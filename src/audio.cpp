#include <stdio.h>
#include <format>
#include <mutex>
#include "miniaudio.h"
#include "audio.h"
#include "queue.h"
#define min(a,b) (a)<(b) ? (a) : (b)

std::mutex m;

fft::fft(int ms, const ma_device* pDevice) {
    int frames = ma_calculate_buffer_size_in_frames_from_milliseconds(ms, pDevice->sampleRate);
    printf("ring buffer size: %i\n", frames);
    auto config = ma_channel_converter_config_init(
        pDevice->playback.format,
        pDevice->playback.channels,
        NULL,
        1,
        NULL,
        ma_channel_mix_mode_default
    );
    if (
        (status = ma_channel_converter_init(&config, NULL, &converter) == MA_SUCCESS ? SUCCESS : ERR_INIT_CONV) == SUCCESS &&
        (status = ma_pcm_rb_init(pDevice->playback.format, 1, frames, NULL, NULL, &buf) == MA_SUCCESS ? SUCCESS : ERR_INIT_RBUF) == SUCCESS
        ) {
        pConverter = &converter;
        pBuf = &buf;
    }
}

void fft::cleanup() {
    ma_pcm_rb_uninit(pBuf);
    ma_channel_converter_uninit(pConverter, NULL);
}

void fft::copy_frames(void* pFramesIn, ma_uint32 nframes) {
    ma_pcm_rb_acquire_write(&buf, &nframes, &buf_handle);
    ma_channel_converter_process_pcm_frames(&converter, buf_handle, pFramesIn, nframes);
    ma_pcm_rb_commit_write((ma_pcm_rb*)buf_handle, nframes);
}

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_uint64 framesRead;
    std::lock_guard<std::mutex> lock_context(m);
    ctx* pContext = (ctx*)(pDevice->pUserData);
    pContext->pMsgQueue->push("callback");
    AudioBuffer* pBuffer = pContext->pBuffer;
    ma_engine_read_pcm_frames(pContext->pEngine, pBuffer->pData, min(frameCount, pBuffer->frameCount), &framesRead);
    ma_copy_pcm_frames(pOutput, pBuffer->pData, framesRead, ma_format_f32, pBuffer->channels);
    //pContext->pFFT->copy_frames(pBuffer->pData, framesRead);
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
    case ERR_INIT_PBUF:
        printf("Failed to initialize playback buffer\n");
        break;
    case ERR_INIT_RBUF:
        printf("Failed to initialize ring buffer\n");
        break;
    case ERR_INIT_CONV:
        printf("Failed to initialize converter\n");
        break;
    }
}

player::player(float vol, const char* path) {
    std::lock_guard<std::mutex> lock_context(m);
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
            pFFT = new fft(200, pDevice),
            true) &&
        (status = pFFT->status) == SUCCESS &&
        (status = pBuffer->status) == SUCCESS
        ) {
        context.pEngine = pEngine;
        context.pBuffer = pBuffer;
        context.pMsgQueue = &msg_queue;
        context.pFFT = pFFT;
    }
    else print_audio_status(status);
}

void player::cleanup() {
    ma_sound_stop(pSound);
    ma_sound_uninit(pSound);
    ma_engine_uninit(pEngine);
    ma_device_uninit(pDevice);
    if (context.pFFT) context.pFFT->cleanup();
    delete context.pFFT;
    delete pBuffer;
}

void player::play() {
    ma_sound_start(pSound);
}

bool player::is_playing() {
    return ma_sound_is_playing(pSound);
}