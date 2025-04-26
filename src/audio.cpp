#include "audio.h"
#include "miniaudio.h"
#include "utils.h"
#include "config.h"
#include <cmath>
#include <limits>
#include <mutex>
#include <iostream>

float a0 = 0.53836, a1 = 1 - a0;

double hamming(ma_uint64 n, ma_uint64 N)
{
    return a0 - a1 * std::cos(2 * M_PI * double(n) / N);
}

template <class T>
void apply_windowfunc(T *pData, ma_uint64 N,
                      double (*pFunc)(ma_uint64, ma_uint64))
{
    ma_uint64 n;
    for (n = 0; n < N; n++, pData++)
        *pData *= (*func)(n, N);
}

void playback_data_callback(ma_device *pDevice, void *pOutput,
                            const void *pInput, ma_uint32 frameCount)
{
    /*
    std::lock_guard<std::mutex> lck(syncPlayback);
    auto pCtx = (ctx *)(pDevice->pUserData);
    auto pPlaybackInfo = &pCtx->playbackInfo;
    auto pPlaybackHandler = pCtx->pPlaybackHandler;
    if (pPlaybackHandler && !pPlaybackHandler->allocated)
        pPlaybackHandler->alloc_playback(frameCount);
    if (!pPlaybackInfo->playing)
        return;

    pPlaybackHandler->callback(frameCount, pOutput);

    auto pBuf = pCtx->pBufPlayback;
    frameCount = pBuf->request_write(frameCount);
    pBuf->seek(pBuf->writePos);

    // read pcm frames into buffer
    ma_uint64 framesRead;
    if (
        ma_decoder_read_pcm_frames(
            pCtx->pDecoder,
            pBuf->ptr,
            frameCount,
            &framesRead) == MA_AT_END)
    {
        pPlaybackInfo->playing = false;
        pPlaybackInfo->end = true;
    }

    pBuf->writePos += framesRead;

    // update cursor position
    ma_decoder_get_cursor_in_pcm_frames(pCtx->pDecoder,
                                        &pPlaybackInfo->audioFrameCursor);

    compute_time_info(
        pPlaybackInfo->audioFrameCursor,
        pPlaybackInfo->sampleRate,
        &pPlaybackInfo->current);
    pCtx->pUI->update_player(pPlaybackInfo);

    // write pcm frames to device
    ma_copy_pcm_frames(pOutput, pBuf->ptr, pBuf->writePos, pBuf->format,
                       pBuf->channels);

    // update FFT and graph using new frames
    if (pCtx->pFFT->update(pBuf))
        pCtx->pUI->update_amplitudes(pCtx->pFFT->magnitudes_raw);
    */
}

void print_audio_status(AudioStatus status)
{
    switch (status)
    {
    case SUCCESS:
        std::cout << "Initialization successful\n"
                  << std::endl;
        break;
    case ERR_INIT_DEVICE:
        std::cout << "Failed to initialize device\n"
                  << std::endl;
        break;
    case ERR_INIT_DECODER:
        std::cout << "Failed to initialize decoder\n"
                  << std::endl;
        break;
    case ERR_INIT_SOUND:
        std::cout << "Failed to initialize sound\n"
                  << std::endl;
        break;
    case ERR_INIT_BUF:
        std::cout << "Failed to initialize buffer\n"
                  << std::endl;
        break;
    case ERR_INIT_CONV:
        std::cout << "Failed to initialize converter\n"
                  << std::endl;
        break;
    }
}

AudioBuffer::AudioBuffer(ma_uint64 frameSize, ma_uint32 channels,
                         ma_format format)
    : format(format),
      writePos(0),
      readPos(0),
      channels(channels),
      bps(ma_get_bytes_per_sample(format)),
      buf(malloc(frameSize * channels * bps))
{
    status = buf == NULL ? ERR_INIT_BUF : SUCCESS;
}

AudioBuffer::~AudioBuffer() { free(buf); }

void AudioBuffer::seek(ma_uint64 framePos)
{
    ptr = (char *)buf + framePos * channels * bps;
}

void AudioBuffer::request_write(ma_uint64 *pFrameCount)
{
    if (writePos + *pFrameCount > frameSize)
        *pFrameCount = frameSize - writePos;
    seek(writePos);
}

void AudioBuffer::request_read(ma_uint64 *pFrameCount)
{
    if (readPos + *pFrameCount > writePos)
        *pFrameCount = writePos - readPos;
    seek(readPos);
}

bool AudioBuffer::empty()
{
    return readPos == writePos;
}

bool AudioBuffer::full()
{
    return writePos == frameSize;
}

void AudioBuffer::clear()
{
    readPos = 0;
    writePos = 0;
}

FFT::FFT(ma_uint64 N, kiss_fft_scalar *timedata, ma_uint32 sampleRate) : N(N),
                                                                         timedata(timedata)
{
    windowSum = 0;
    for (ma_uint64 n = 0; n < N; n++)
        windowSum += hamming(n, N);

    FFTcfg = kiss_fftr_alloc(N, 0, NULL, NULL);

    freqdata = new kiss_fft_cpx[N];
    kiss_fft_cpx **ppFreq, *pEnd = freqdata + N / 2;
    int i;
    for (i = 0, ppFreq = frequencyPtrs; i < N_BINS + 1; i++, ppFreq++)
    {
        *ppFreq = freqdata + frequencies[i] * N / sampleRate;
        if (*ppFreq > pEnd)
            *ppFreq = pEnd;
    }
}

void FFT::cleanup()
{
    kiss_fftr_free(FFTcfg);
    delete[] freqdata;
}

void FFT::reduce_spectrum()
{
    int i, n;
    double sum;
    kiss_fft_cpx *pFreq0, *pFreq1;
    for (i = 0, pMag_raw = magnitudes_raw; i < N_BINS; i++, pMag_raw++)
    {
        pFreq0 = frequencyPtrs[i];
        pFreq1 = frequencyPtrs[i + 1];
        n = pFreq1 - pFreq0;
        sum = 0;
        while (pFreq0 < pFreq1)
        {
            sum += 2 * std::hypot(double(pFreq0->r), double(pFreq0->i)) / windowSum;
            pFreq0++;
        }
        if (n != 0)
            sum /= n;
        *pMag_raw = 20 * std::log10(sum + 1e-12);
        if (*pMag_raw < vmin)
            vmin = *pMag_raw;
        if (*pMag_raw > vmax)
            vmax = *pMag_raw;
    }
}

void FFT::compute()
{
    apply_windowfunc<kiss_fft_scalar>(timedata, FFT_BUFFER_FRAMES, &hamming);
    kiss_fftr(FFTcfg, timedata, freqdata);
    reduce_spectrum();
}

Player::Player(ctx *pContext)
    : pContext(pContext),
      pPlaybackInfo(&pContext->playbackInfo)
{
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = playback_data_callback;
    deviceConfig.pUserData = pContext;

    if ((status = ma_device_init(NULL, &deviceConfig, &device) == MA_SUCCESS
                      ? SUCCESS
                      : ERR_INIT_DEVICE) == SUCCESS &&
        (pBuffer =
             new AudioBuffer(0, device.playback.channels, device.playback.format),
         pFFT = new FFT(FFT_BUFFER_FRAMES, device.playback.format,
                        device.playback.channels, device.sampleRate),
         true) &&
        (status = pBuffer->status) == SUCCESS &&
        (status = pFFT->status) == SUCCESS)
    {
        pDevice = &device;
        pContext->pBufPlayback = pBuffer;
        pContext->pFFT = pFFT;
        ma_device_start(pDevice);
    }
    else
        print_audio_status(status);
}

void Player::cleanup()
{
    std::lock_guard<std::mutex> lck(syncPlayback);
    ma_device_uninit(pDevice);
    ma_decoder_uninit(pDecoder);
    if (pFFT)
        pFFT->cleanup();
    delete pFFT;
    delete pBuffer;
}

void Player::play()
{
    if (pPlaybackInfo->end)
    {
        ma_decoder_seek_to_pcm_frame(pDecoder, 0);
        pPlaybackInfo->audioFrameCursor = 0;
        pPlaybackInfo->end = false;
    }
    pPlaybackInfo->playing = true;
}

void Player::pause() { pPlaybackInfo->playing = false; }

AudioStatus Player::load_audio(const char *filePath)
{
    auto decoderConfig = ma_decoder_config_init(
        device.playback.format, device.playback.channels, device.sampleRate);
    if ((status = ma_decoder_init_file(filePath, &decoderConfig, &decoder) ==
                          MA_SUCCESS
                      ? SUCCESS
                      : ERR_INIT_DECODER) == SUCCESS)
    {
        pDecoder = &decoder;
        pContext->pDecoder = pDecoder;
        ma_decoder_get_length_in_pcm_frames(pDecoder,
                                            &pPlaybackInfo->audioFrameSize);
        pPlaybackInfo->sampleRate = pDecoder->outputSampleRate;
        pPlaybackHanlder = new PlaybackHandler(
            pDecoder,
            pDevice,
            pFFT->pConverter);
        pContext->pPlaybackHandler = pPlaybackHanlder;
    }
    return status;
}
