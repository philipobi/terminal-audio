#include <stdio.h>
#include <mutex>
#include "miniaudio.h"
#include "audio.h"
#include "queue.h"


std::mutex m;

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    //printf("callback\n");
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
            pFFT = new FFT(nbins, 100, pDevice),
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

void normalize(const kiss_fft_cpx* pIn, const kiss_fft_cpx* const pIn_, float* pOut) {
    while (pIn != pIn_) {
        *pOut++ = 20 * std::sqrt(square(pIn->r) + square(pIn->i));
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