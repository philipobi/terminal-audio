#include <stdio.h>
#include <mutex>
#include <cmath>
#include "miniaudio.h"
#include "audio.h"
#include "queue.h"
#include "partition.h"

float
a0 = 0.53836,
a1 = 1 - a0;

float hamming(ma_uint64 n, ma_uint64 N) {
    return a0 - a1 * std::cos(2 * M_PI * double(n) / N);
}

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto pContext = (ctx*)(pDevice->pUserData);
    if (!pContext->init) return;
    auto pBufPlayback = pContext->pBufPlayback;
    auto pMemPlayback = pBufPlayback->get_ptr(0);
    ma_engine_read_pcm_frames(pContext->pEngine, pMemPlayback, min(frameCount, pBufPlayback->frameSize), &pBufPlayback->writePos);
    ma_copy_pcm_frames(pOutput, pMemPlayback, pBufPlayback->writePos, pBufPlayback->format, pBufPlayback->channels);
    if (pContext->pFFT->update(pBufPlayback))
        pContext->pGraph->update_activations(pContext->pFFT->bins);
}

void print_audio_status(AudioStatus status) {
    switch (status) {
    case SUCCESS:
        printf("Initialization successful\n");
        break;
    case ERR_INIT_DEVICE:
        printf("Failed to initialize device\n");
        break;
    case ERR_INIT_ENGINE:
        printf("Failed to initialize engine\n");
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

void normalize(const kiss_fft_cpx* pIn, const kiss_fft_cpx* const pIn_, float* pOut) {
    while (pIn != pIn_) {
        *pOut++ = std::hypot(pIn->r, pIn->i);
        pIn++;
    }
}

void reduce_bins(const float* pFreq, const ma_uint64* pBinSize, double* pBin, int nbins) {
    ma_uint64 j, n;
    for (int i = 0; i < nbins; i++, pBin++) {
        *pBin = 0;
        n = *pBinSize++;
        for (j = 0; j < n; j++) *pBin += *pFreq++;
        *pBin /= n;
        *pBin = 20 * std::log10(*pBin + 1e-12);
    }
}

Player::Player(int nbins, float vol, const char* path) {
    ma_audio_buffer_config bufferConfig;
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = playback_data_callback;
    deviceConfig.pUserData = &context;
    auto engineConfig = ma_engine_config_init();
    if (
        (status = ma_device_init(NULL, &deviceConfig, &device) == MA_SUCCESS ? SUCCESS : ERR_INIT_DEVICE) == SUCCESS &&
        (pDevice = &device, engineConfig.pDevice = pDevice, true) &&
        (status = ma_engine_init(&engineConfig, &engine) == MA_SUCCESS ? SUCCESS : ERR_INIT_ENGINE) == SUCCESS &&
        (pEngine = &engine, ma_engine_set_volume(pEngine, vol), true) &&
        (status = ma_sound_init_from_file(pEngine, path, MA_SOUND_FLAG_STREAM, NULL, NULL, &sound) == MA_SUCCESS ? SUCCESS : ERR_INIT_SOUND) == SUCCESS &&
        (
            pSound = &sound,
            pBuffer = new AudioBuffer(500, pDevice->playback.channels, pDevice->playback.format),
            pFFT = new FFT(nbins, FFT_BUFFER_MS, pDevice),
            true) &&
        (status = pFFT->status) == SUCCESS &&
        (status = pBuffer->status) == SUCCESS
        ) {
        context.pEngine = pEngine;
        context.pBufPlayback = pBuffer;
        //context.pMsgQueue = &msg_queue;
        context.pFFT = pFFT;
        context.init = true;
    }
    else print_audio_status(status);
}

void Player::cleanup() {
    ma_sound_stop(pSound);
    ma_sound_uninit(pSound);
    ma_engine_uninit(pEngine);
    ma_device_uninit(pDevice);
    if (context.pFFT) context.pFFT->cleanup();
    delete context.pFFT;
    delete pBuffer;
}

void Player::play() {
    ma_sound_start(pSound);
}

bool Player::is_playing() {
    return ma_sound_is_playing(pSound);
}

FFT::FFT(int nbins, ma_uint32 duration_ms, const ma_device* pDevice) :
    nbins(nbins)
{
    auto sampleRate = pDevice->sampleRate;
    frameSizeFFT = ma_calculate_buffer_size_in_frames_from_milliseconds(duration_ms, pDevice->sampleRate);
    frameSizeFFT += frameSizeFFT % 2 == 0 ? 0 : 1;


    FFTcfg = kiss_fftr_alloc(frameSizeFFT, 0, NULL, NULL);
    auto config = ma_data_converter_config_init(
        pDevice->playback.format,
        ma_format_f32,
        pDevice->playback.channels,
        1,
        sampleRate,
        sampleRate
    );

    if (
        (status = ma_data_converter_init(&config, NULL, &converter) == MA_SUCCESS ? SUCCESS : ERR_INIT_CONV) == SUCCESS &&
        (
            pBuffer = new AudioBuffer(
                2 * frameSizeFFT,
                1,
                ma_format_f32
            ),
            status = pBuffer->status
            ) == SUCCESS
        )
    {
        pConverter = &converter;
        freq_cpx = new kiss_fft_cpx[frameSizeFFT];
        ma_uint64 i1 = FFT_FREQUENCY_MAX * frameSizeFFT / sampleRate;
        i1 = i1 < frameSizeFFT ? i1 : frameSizeFFT;
        ma_uint64 i0 = FFT_FREQUENCY_MIN * frameSizeFFT / sampleRate;
        i0 = i0 < i1 ? i0 : 0;
        freq_cpx_start = freq_cpx + i0;
        freq_cpx_end = freq_cpx + i1;
        ma_uint64 range = i1 - i0;
        freq = new float[range];
        bins = new double[nbins];
        binSizes = new ma_uint64[nbins];

        partition_exp<ma_uint64>(nbins, range, binSizes);
    }

}

void FFT::cleanup() {
    kiss_fftr_free(FFTcfg);
    ma_data_converter_uninit(pConverter, NULL);
    delete pBuffer;
    delete[] freq_cpx;
    delete[] freq;
    delete[] bins;
    delete[] binSizes;
}

bool FFT::update(AudioBuffer* pBufPlayback) {
    ma_uint64 framesRead = min(pBufPlayback->writePos, frameSizeFFT - pBuffer->writePos);
    void
        * pMemPlayback = pBufPlayback->get_ptr(0),
        * pMemFFT = pBuffer->get_ptr(pBuffer->writePos);
    ma_data_converter_process_pcm_frames(pConverter, pMemPlayback, &pBufPlayback->writePos, pMemFFT, &framesRead);
    pBuffer->writePos += framesRead;

    if (pBuffer->writePos != frameSizeFFT) return false;

    apply_windowfunc((float*)pBuffer->get_ptr(0), frameSizeFFT, &hamming);
    kiss_fftr(FFTcfg, (const float*)(pBuffer->get_ptr(0)), freq_cpx);
    pBuffer->writePos = 0;
    normalize(freq_cpx_start, freq_cpx_end, freq);
    reduce_bins(freq, binSizes, bins, nbins);

    int i;
    for (pBin = bins, i = 0; i < nbins; i++, pBin++) {
        if (*pBin > vmax) vmax = *pBin;
        *pBin = *pBin > 0 ? *pBin / vmax : 0;
    };

    return true;
};

AudioBuffer::AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format) :
    format(format),
    frameSize(frameSize),
    writePos(0),
    readPos(0),
    channels(channels),
    bps(ma_get_bytes_per_sample(format)),
    buf((void* const)malloc(frameSize* channels* bps))
{
    status = buf == NULL ? ERR_INIT_BUF : SUCCESS;
}

AudioBuffer::~AudioBuffer() { free(buf); }

void* AudioBuffer::get_ptr(ma_uint64 framePos) {
    return (char*)buf + framePos * channels * bps;
}

void apply_windowfunc(float* pData, ma_uint64 N, float (*func)(ma_uint64, ma_uint64)) {
    ma_uint64 n;
    for (n = 0; n < N; n++) *pData++ *= (*func)(n, N);
}