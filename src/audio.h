#pragma once
#include <stdlib.h>
#include <cmath>
#include "miniaudio.h"
#include "frontend.h"
#include "kiss_fftr.h"
#include "utils.h"
#include "config.h"

enum AudioStatus
{
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_DECODER,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

void print_audio_status(AudioStatus status);

class AudioBuffer
{
    ma_uint32 bps;
    void *buf = NULL;
    int i = 0, n = 10;

public:
    void *ptr = NULL;
    ma_uint64 frameSize, writePos, readPos, channels;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
    ~AudioBuffer();
    void seek(ma_uint64 framePos);
    void malloc_frames(ma_uint64 n_frames);
    ma_uint64 request_write(ma_uint64 frameCount);
};

class FFT
{
    int frequencies[N_BINS + 1] = BIN_FREQUENCIES;
    kiss_fft_cpx *frequencyPtrs[N_BINS + 1];
    ma_data_converter converter, *pConverter = NULL;
    AudioBuffer *pBuffer = NULL;
    ma_uint64 frameSizeFFT, frameCountIn, frameCountOut;
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx *freq_cpx = NULL;
    double windowSum, vmin, vmax;
    double *pMag, *pMag_raw;
    void *ptr0;

public:
    double magnitudes_raw[N_BINS] = {0};
    double magnitudes[N_BINS] = {0};
    AudioStatus status;

    FFT(
        ma_uint64 frameSize,
        ma_format format,
        ma_uint32 channels,
        ma_uint32 sampleRate);
    bool update(AudioBuffer *pBufPlayback);
    void cleanup();
    void reduce_spectrum();
};

struct ctx
{
    ma_decoder *pDecoder = NULL;
    AudioBuffer *pBufPlayback = NULL;
    FFT *pFFT = NULL;
    UI *pUI = NULL;
    PlaybackInfo playbackInfo;
};

class Player
{
    ma_device device, *pDevice = NULL;
    ma_decoder decoder, *pDecoder = NULL;
    AudioBuffer *pBuffer = NULL;
    FFT *pFFT = NULL;
    PlaybackInfo *pPlaybackInfo = NULL;
    ctx *const pContext = NULL;

public:
    enum AudioStatus status;
    Player(ctx *pContext);
    void play();
    void pause();
    AudioStatus load_audio(const char *filePath);
    void cleanup();
};