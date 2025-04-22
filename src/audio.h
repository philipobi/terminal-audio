#pragma once
#include <stdlib.h>
#include <string>
#include <cmath>
#include "miniaudio.h"
#include "graph.h"
#include "queue.h"
#include "kiss_fftr.h"
#include "utils.h"
#define BIN_FREQUENCIES { 30, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 }
#define BIN_LABELS { "30", "60", "1h", "2h", "5h", "1k", "2k", "4k", "8k" }
#define N_BINS 9

#define FFT_BUFFER_FRAMES 4096
#define SAMPLE_FORMAT ma_format_f32
typedef float sample_type;

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_DECODER,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

void print_audio_status(AudioStatus status);

class AudioBuffer {
    ma_uint32 bps;
    void* buf = NULL;
    int i = 0, n = 10;
public:
    void* ptr = NULL;
    ma_uint64 frameSize, writePos, readPos, channels;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
    ~AudioBuffer();
    void seek(ma_uint64 framePos);
    void malloc_frames(ma_uint64 n_frames);
    ma_uint64 request_write(ma_uint64 frameCount);
};

class FFT {
    int frequencies[N_BINS + 1] = BIN_FREQUENCIES;
    kiss_fft_cpx* frequencyPtrs[N_BINS + 1];
    ma_data_converter converter, * pConverter = NULL;
    AudioBuffer* pBuffer = NULL;
    ma_uint64 frameSizeFFT, frameCountIn, frameCountOut;
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx* freq_cpx = NULL;
    double windowSum, vmin, vmax;
    double* pMag, * pMag_raw;
    void* ptr0;
public:
    double magnitudes_raw[N_BINS] = { 0 };
    double magnitudes[N_BINS] = { 0 };
    AudioStatus status;

    FFT(
        ma_uint64 frameSize,
        ma_format format,
        ma_uint32 channels,
        ma_uint32 sampleRate
    );
    bool update(AudioBuffer* pBufPlayback);
    void cleanup();
    void reduce_spectrum();
};

struct ctx {
    ma_decoder* pDecoder = NULL;
    AudioBuffer* pBufPlayback = NULL;
    FFT* pFFT = NULL;
    Graph* pGraph = NULL;
    bool
        init = false,
        playing = false,
        end = true;
};

class Player {
    ma_device device, * pDevice = NULL;
    ma_decoder decoder, * pDecoder = NULL;
    AudioBuffer* pBuffer = NULL;
    FFT* pFFT = NULL;
public:
    ctx context;
    enum AudioStatus status;
    Player(const char* path);
    void play();
    void cleanup();
    bool is_playing();
};