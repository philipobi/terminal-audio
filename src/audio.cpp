#include <stdio.h>
#include <format>
#include "miniaudio.h"
#include "audio.h"
#include "queue.h"



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
    
    ctx* context = (ctx*)(pDevice->pUserData);
    context->pMsgQueue->push("callback");
    ma_audio_buffer* pBuffer = context->pBuffer;
    ma_engine_read_pcm_frames(context->pEngine, pBuffer, frameCount, &context->framesRead);
    context->pFFT->copy_frames(pBuffer, context->framesRead);
    ma_audio_buffer_read_pcm_frames(pBuffer, pOutput, context->framesRead, false);
}

void do_nothing(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){}


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
    ma_audio_buffer_config bufferConfig;
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = do_nothing;
    deviceConfig.pUserData = &pEngine;
    auto engineConfig = ma_engine_config_init();
    if (
        (status = ma_device_init(NULL, &deviceConfig, &device) == MA_SUCCESS ? SUCCESS : ERR_INIT_DEVICE) == SUCCESS &&
        (pDevice = &device, engine.pDevice = pDevice, true) &&
        (status = ma_engine_init(&engineConfig, &engine) == MA_SUCCESS ? SUCCESS : ERR_INIT_ENGINE) == SUCCESS &&
        (pEngine = &engine, ma_engine_set_volume(pEngine, vol), true) &&
        (status = ma_sound_init_from_file(pEngine, path, MA_SOUND_FLAG_STREAM, NULL, NULL, &sound) == MA_SUCCESS ? SUCCESS : ERR_INIT_SOUND) == SUCCESS &&
        (pSound = &sound, bufferConfig = ma_audio_buffer_config_init(
            pDevice->playback.format,
            pDevice->playback.channels,
            200,
            NULL,
            NULL
        ), true) &&
        (status = ma_audio_buffer_alloc_and_init(&bufferConfig, &pBuffer) == MA_SUCCESS ? SUCCESS : ERR_INIT_PBUF) == SUCCESS &&
        (
            context.pEngine = pEngine,
            context.pBuffer = pBuffer,
            context.pMsgQueue = &msg_queue,
            context.pFFT = new fft(200, pDevice),
            true
            ) &&
        (status = context.pFFT->status) == SUCCESS

        ); else print_audio_status(status);
}

void player::cleanup() {
    ma_sound_stop(pSound);
    ma_sound_uninit(pSound);
    ma_engine_uninit(pEngine);
    ma_device_uninit(pDevice);
    ma_audio_buffer_uninit_and_free(pBuffer);
    if (context.pFFT) context.pFFT->cleanup();
    delete context.pFFT;
}

void player::play() {
    ma_sound_start(pSound);
}

bool player::is_playing(){
    return ma_sound_is_playing(pSound);
}