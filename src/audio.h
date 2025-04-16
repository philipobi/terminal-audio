#pragma once
#include "miniaudio.h"

struct ctx {
    ma_engine* pEngine;
    ma_audio_buffer* pBuffer;
    ma_uint64 framesRead;
};

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_ENGINE,
    ERR_INIT_SOUND,
    ERR_INIT_PBUF
};

void print_audio_status(AudioStatus status);

class player {
    ma_device device, * pDevice = NULL;
    ma_engine engine, * pEngine = NULL;
    ma_sound sound, * pSound = NULL;
    ma_audio_buffer* pBuffer = NULL;
    ctx context;
public:
    enum AudioStatus status;
    player(float vol, const char* path);
    void play();
    void cleanup();
};