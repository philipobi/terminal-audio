#pragma once
#include <stdlib.h>
#include <string>
#include <cmath>
#include "miniaudio.h"
#include "graph.h"
#include "queue.h"
#include "kiss_fftr.h"
#include "utils.h"
#define FFT_BUFFER_MS 100
#define BIN_FREQUENCIES { 15, 30, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 }
#define BIN_LABELS { "15", "30", "60", "1h", "2h", "5h", "1k", "2k", "4k", "8k" }
#define N_BINS 10

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_ENGINE,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

void print_audio_status(AudioStatus status);

class AudioBuffer {
    ma_uint32 bps;
    void* const buf;
public:
    ma_uint64 frameSize, writePos, readPos, channels;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
    ~AudioBuffer();
    void* get_ptr(ma_uint64 framePos);
};

class FFT {
    int frequencies[N_BINS + 1] = BIN_FREQUENCIES;
    kiss_fft_cpx* frequencyPtrs[N_BINS + 1];
    ma_data_converter converter, * pConverter = NULL;
    AudioBuffer* pBuffer = NULL;
    ma_uint64 frameSizeFFT;
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx* freq_cpx = NULL;
    double windowSum, vmin, vmax;
    double* pMag;
public:
    double magnitudes[N_BINS] = { 0 };
    AudioStatus status;

    FFT(ma_uint32 duration_ms, const ma_device* pDevice);
    bool update(AudioBuffer* pBufPlayback);
    void cleanup();
    void reduce_spectrum();
};

struct ctx {
    ma_engine* pEngine = NULL;
    AudioBuffer* pBufPlayback = NULL;
    Graph* pGraph = NULL;
    FFT* pFFT = NULL;
    Queue<std::string>* pMsgQueue = NULL;
    bool init = false;
};

class Player {
    ma_device device, * pDevice = NULL;
    ma_engine engine, * pEngine = NULL;
    ma_sound sound, * pSound = NULL;
    AudioBuffer* pBuffer = NULL;
    FFT* pFFT = NULL;
public:
    ctx context;
    enum AudioStatus status;
    Player(int nbins, float vol, const char* path);
    void play();
    void cleanup();
    bool is_playing();
};