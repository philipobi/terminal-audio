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
#define FFT_FREQUENCY_MAX 8000
#define FFT_FREQUENCY_MIN 20

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_ENGINE,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

void print_audio_status(AudioStatus status);

void normalize(const kiss_fft_cpx* pIn, const kiss_fft_cpx* const pIn_, float* pOut);

void reduce_bins(const float* pFreq, const ma_uint64* pBinSize, double* pBin, int nbins);

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
    ma_data_converter converter, * pConverter = NULL;
    AudioBuffer* pBuffer = NULL;
    ma_uint64 frameSizeFFT, * binSizes;
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx
        * freq_cpx = NULL,
        * freq_cpx_start = NULL,
        * freq_cpx_end = NULL;
    float* freq = NULL;
    double vmax = 1;
public:
    int nbins;
    double* bins = NULL, * pBin;
    AudioStatus status;

    FFT(int nbins, ma_uint32 duration_ms, const ma_device* pDevice);
    bool update(AudioBuffer* pBufPlayback);
    void cleanup();
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