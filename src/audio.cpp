#include <stdio.h>
#include <mutex>
#include <cmath>
#include <limits>
#include "miniaudio.h"
#include "audio.h"
#include "queue.h"
#include "partition.h"

float
a0 = 0.53836,
a1 = 1 - a0;

double hamming(ma_uint64 n, ma_uint64 N) {
    return a0 - a1 * std::cos(2 * M_PI * double(n) / N);
}

template <class T>
void apply_windowfunc(T* pData, ma_uint64 N, double (*func)(ma_uint64, ma_uint64)) {
    ma_uint64 n;
    for (n = 0; n < N; n++, pData++) *pData *= (*func)(n, N);
}

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto pCtx = (ctx*)(pDevice->pUserData);
    if (!pCtx->playing || !pCtx->init) {
        return;
    }

    auto pBuf = pCtx->pBufPlayback;
    frameCount = pBuf->request_write(frameCount);

    pBuf->seek(0);

    // read pcm frames into buffer
    if (
        ma_decoder_read_pcm_frames(
            pCtx->pDecoder,
            pBuf->ptr,
            frameCount,
            &pBuf->writePos
        ) == MA_AT_END
        ) {
        pCtx->playing = false;
        pCtx->end = true;
    }

    // write pcm frames to device
    ma_copy_pcm_frames(
        pOutput,
        pBuf->ptr,
        pBuf->writePos,
        pBuf->format,
        pBuf->channels
    );

    // update FFT and graph using new frames
    if (pCtx->pFFT->update(pBuf))
        pCtx->pGraph->update(
            pCtx->pFFT->magnitudes_raw
        );
}

void print_audio_status(AudioStatus status) {
    switch (status) {
    case SUCCESS:
        printf("Initialization successful\n");
        break;
    case ERR_INIT_DEVICE:
        printf("Failed to initialize device\n");
        break;
    case ERR_INIT_DECODER:
        printf("Failed to initialize decoder\n");
        break;
    case ERR_INIT_SOUND:
        printf("Failed to initialize sound\n");
        break;
    case ERR_INIT_BUF:
        printf("Failed to initialize buffer\n");
        break;
    case ERR_INIT_CONV:
        printf("Failed to initialize converter\n");
        break;
    }
}


AudioBuffer::AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format) :
    format(format),
    writePos(0),
    readPos(0),
    channels(channels),
    bps(ma_get_bytes_per_sample(format))
{
    if (frameSize != 0) {
        malloc_frames(frameSize);
        status = buf == NULL ? ERR_INIT_BUF : SUCCESS;
    }
}

AudioBuffer::~AudioBuffer() { free(buf); }

void AudioBuffer::seek(ma_uint64 framePos) {
    ptr = (char*)buf + framePos * channels * bps;
}

ma_uint64 AudioBuffer::request_write(ma_uint64 frameCount) {
    if (buf == NULL) malloc_frames(frameCount);
    if (frameSize < frameCount) {
        if (++i == n) {
            malloc_frames(1.5 * frameCount);
            i = 0;
            return frameCount;
        }
        return frameSize;
    }
    return frameCount;
}

void AudioBuffer::malloc_frames(ma_uint64 n_frames) {
    free(buf);
    buf = malloc(n_frames * channels * bps);
    frameSize = buf == NULL ? 0 : n_frames;
}


FFT::FFT(
    ma_uint64 frameSize,
    ma_format format,
    ma_uint32 channels,
    ma_uint32 sampleRate
) :
    frameSizeFFT(frameSize)
{
    if (frameSizeFFT % 2 == 1) frameSizeFFT += 1;
    windowSum = 0;
    for (ma_uint64 n = 0; n < frameSizeFFT; n++) windowSum += hamming(n, frameSizeFFT);

    FFTcfg = kiss_fftr_alloc(frameSizeFFT, 0, NULL, NULL);
    auto converterConfig = ma_data_converter_config_init(
        format,
        SAMPLE_FORMAT,
        channels,
        1,
        sampleRate,
        sampleRate
    );

    if (
        (status = ma_data_converter_init(&converterConfig, NULL, &converter) == MA_SUCCESS ? SUCCESS : ERR_INIT_CONV) == SUCCESS &&
        (
            pBuffer = new AudioBuffer(
                frameSizeFFT,
                1,
                SAMPLE_FORMAT
            ),
            status = pBuffer->status
            ) == SUCCESS
        )
    {
        pConverter = &converter;
        freq_cpx = new kiss_fft_cpx[frameSizeFFT];
        kiss_fft_cpx
            ** ppFreq,
            * pEnd = freq_cpx + frameSizeFFT / 2;
        int i;
        for (i = 0, ppFreq = frequencyPtrs; i < N_BINS + 1; i++, ppFreq++) {
            *ppFreq = freq_cpx + frequencies[i] * frameSizeFFT / sampleRate;
            if (*ppFreq > pEnd) *ppFreq = pEnd;

            vmin = std::numeric_limits<double>::infinity();
            vmax = -std::numeric_limits<double>::infinity();
        }
    }

}

void FFT::cleanup() {
    kiss_fftr_free(FFTcfg);
    ma_data_converter_uninit(pConverter, NULL);
    delete pBuffer;
    delete[] freq_cpx;
}

void FFT::reduce_spectrum() {
    int i, n;
    double sum;
    kiss_fft_cpx* pFreq0, * pFreq1;
    for (i = 0, pMag_raw = magnitudes_raw; i < N_BINS; i++, pMag_raw++) {
        pFreq0 = frequencyPtrs[i];
        pFreq1 = frequencyPtrs[i + 1];
        n = pFreq1 - pFreq0;
        sum = 0;
        while (pFreq0 < pFreq1) {
            sum += 2 * std::hypot(double(pFreq0->r), double(pFreq0->i)) / windowSum;
            pFreq0++;
        }
        if (n != 0) sum /= n;
        *pMag_raw = 20 * std::log10(sum + 1e-12);
        if (*pMag_raw < vmin) vmin = *pMag_raw;
        if (*pMag_raw > vmax) vmax = *pMag_raw;
    }
}

bool FFT::update(AudioBuffer* pBufPlayback) {
    frameCountIn = min(pBufPlayback->writePos, frameSizeFFT - pBuffer->writePos);
    frameCountOut = frameCountIn;
    ptr0 = pBufPlayback->ptr;

    pBuffer->seek(pBuffer->writePos);
    pBufPlayback->seek(pBufPlayback->writePos - frameCountIn);
    ma_data_converter_process_pcm_frames(
        pConverter,
        pBufPlayback->ptr,
        &frameCountIn,
        pBuffer->ptr,
        &frameCountOut
    );
    pBuffer->writePos += frameCountOut;

    pBufPlayback->ptr = ptr0;

    if (pBuffer->writePos != frameSizeFFT) return false;

    pBuffer->seek(0);
    apply_windowfunc<sample_type>((sample_type*)pBuffer->ptr, frameSizeFFT, &hamming);
    kiss_fftr(FFTcfg, (const kiss_fft_scalar*)(pBuffer->ptr), freq_cpx);
    pBuffer->writePos = 0;
    reduce_spectrum();
    
    return true;
}


Player::Player(const char* path) {
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = playback_data_callback;
    deviceConfig.pUserData = &context;
    ma_decoder_config decoderConfig;

    if (
        (status = ma_device_init(NULL, &deviceConfig, &device) == MA_SUCCESS ? SUCCESS : ERR_INIT_DEVICE) == SUCCESS &&
        (
            decoderConfig = ma_decoder_config_init(
                device.playback.format,
                device.playback.channels,
                device.sampleRate
            ),
            pBuffer = new AudioBuffer(
                0,
                device.playback.channels,
                device.playback.format
            ),
            pFFT = new FFT(
                FFT_BUFFER_FRAMES,
                device.playback.format,
                device.playback.channels,
                device.sampleRate
            ),
            true) &&
        (status = ma_decoder_init_file(path, &decoderConfig, &decoder) == MA_SUCCESS ? SUCCESS : ERR_INIT_DECODER) == SUCCESS &&
        (status = pBuffer->status) == SUCCESS &&
        (status = pFFT->status) == SUCCESS
        ) {
        pDevice = &device;
        pDecoder = &decoder;
        context.pDecoder = pDecoder;
        context.pBufPlayback = pBuffer;
        context.pFFT = pFFT;
        context.init = true;
        ma_device_start(pDevice);
    }
    else print_audio_status(status);
}

void Player::cleanup() {
    ma_device_uninit(pDevice);
    ma_decoder_uninit(pDecoder);
    if (pFFT) pFFT->cleanup();
    delete pFFT;
    delete pBuffer;
}

void Player::play() {
    if (context.end) {
        ma_decoder_seek_to_pcm_frame(pDecoder, 0);
        context.end = false;
        context.playing = true;
        return;
    }
    context.playing = !context.playing;
}

bool Player::is_playing() {
    return context.playing;
}
