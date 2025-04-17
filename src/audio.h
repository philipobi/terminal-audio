#pragma once
#include <string>
#include "miniaudio.h"
#include "queue.h"

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_ENGINE,
    ERR_INIT_SOUND,
    ERR_INIT_PBUF,
    ERR_INIT_RBUF,
    ERR_INIT_CONV,
};

class fft {
    ma_channel_converter  converter, * pConverter = NULL;
    void* buf_handle;
public:
    ma_pcm_rb buf, * pBuf = NULL;
    AudioStatus status;
    fft(int ms, const ma_device* pDevice);
    void copy_frames(void* pFramesIn, ma_uint32 nframes);
    void cleanup();
};

struct ctx {
    ma_engine* pEngine = NULL;
    ma_audio_buffer* pBuffer = NULL;
    fft* pFFT = NULL;
    Queue<std::string> *pMsgQueue = NULL;
    ma_uint64 framesRead;
};



void print_audio_status(AudioStatus status);




class player {
    ma_device device, * pDevice = NULL;
    ma_engine engine, * pEngine = NULL;
    ma_sound sound, * pSound = NULL;
    ma_audio_buffer* pBuffer = NULL;
public:
    Queue<std::string> msg_queue;
    ctx context;
    enum AudioStatus status;
    player(float vol, const char* path);
    void play();
    void cleanup();
    bool is_playing();
};