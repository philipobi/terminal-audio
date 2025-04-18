#pragma once
#include <stdlib.h>
#include <string>
#include <cmath>
#include "miniaudio.h"
#include "graph.h"
#include "queue.h"
#include "kiss_fftr.h"
#define min(a,b) (a)<(b) ? (a) : (b)
#define max(a,b) (a)>(b) ? (a) : (b)
#define square(a) (a)*(a)

enum AudioStatus {
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_ENGINE,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

class AudioBuffer {
    ma_uint32 bps;
    void* const buf;
public:
    ma_uint64 frameSize, writePos, readPos, channels;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format) :
        format(format),
        frameSize(frameSize),
        writePos(0),
        readPos(0),
        channels(channels),
        bps(ma_get_bytes_per_sample(format)),
        buf((void* const)malloc(frameSize* channels* bps))
    {
        status = buf == NULL ? ERR_INIT_BUF : SUCCESS;
    }
    ~AudioBuffer() { free(buf); }
    void* get_ptr(ma_uint64 framePos) {
        return (char*)buf + framePos * channels * bps;
    }
};

void normalize(const kiss_fft_cpx* pIn, const kiss_fft_cpx* const pIn_, float* pOut);

void reduce_bins(const float* pFreq, const ma_uint64* pBinSize, double* pBin, int nbins);

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

    FFT(int nbins, ma_uint32 duration_ms, const ma_device* pDevice) :
        nbins(nbins)
    {
        auto sampleRate = pDevice->sampleRate;
        frameSizeFFT = ma_calculate_buffer_size_in_frames_from_milliseconds(duration_ms, pDevice->sampleRate);
        frameSizeFFT += frameSizeFFT % 2 == 0 ? 0 : 1;


        FFTcfg = kiss_fftr_alloc(frameSizeFFT, 0, NULL, NULL);
        auto config = ma_data_converter_config_init(
            pDevice->playback.format,
            ma_format_f32,
            pDevice->playback.channels,
            1,
            sampleRate,
            sampleRate
        );

        if (
            (status = ma_data_converter_init(&config, NULL, &converter) == MA_SUCCESS ? SUCCESS : ERR_INIT_CONV) == SUCCESS &&
            (
                pBuffer = new AudioBuffer(
                    2 * frameSizeFFT,
                    1,
                    ma_format_f32
                ),
                status = pBuffer->status
                ) == SUCCESS
            )
        {
            pConverter = &converter;
            freq_cpx = new kiss_fft_cpx[frameSizeFFT];
            ma_uint64 i1 = 20000 * frameSizeFFT / sampleRate;
            i1 = i1 < frameSizeFFT ? i1 : frameSizeFFT;
            ma_uint64 i0 = 20 * frameSizeFFT / sampleRate;
            i0 = i0 < i1 ? i0 : 0;
            freq_cpx_start = freq_cpx + i0;
            freq_cpx_end = freq_cpx + i1;
            ma_uint64 range = i1 - i0;
            freq = new float[range];
            bins = new double[nbins];
            binSizes = new ma_uint64[nbins];

            auto interp = [](float x) { return pow(x, 4); };

            //TODO: FIX BIN SIZE COMPUTATION
            ma_uint64 pos, pos0 = 0;
            for (int n = 1; n <= nbins; n++)
            {
                pos = ma_uint64(interp((float)n / nbins) * range);
                pos = pos == pos0 ? pos + 1 : pos;
                pos = min(pos, range);
                binSizes[n - 1] = pos - pos0;
                pos0 = pos;
            }
        }

    }
    bool update(AudioBuffer* pBufPlayback) {
        ma_uint64 framesRead = min(pBufPlayback->writePos, frameSizeFFT - pBuffer->writePos);
        void
            * pMemPlayback = pBufPlayback->get_ptr(0),
            * pMemFFT = pBuffer->get_ptr(pBuffer->writePos);
        ma_data_converter_process_pcm_frames(pConverter, pMemPlayback, &pBufPlayback->writePos, pMemFFT, &framesRead);
        pBuffer->writePos += framesRead;

        if (pBuffer->writePos != frameSizeFFT) return false;

        kiss_fftr(FFTcfg, (const float*)(pBuffer->get_ptr(0)), freq_cpx);
        pBuffer->writePos = 0;
        normalize(freq_cpx_start, freq_cpx_end, freq);
        reduce_bins(freq, binSizes, bins, nbins);

        int i;
        for (pBin = bins, i = 0; i < nbins; i++, pBin++) {
            if (*pBin > vmax) vmax = *pBin;
            *pBin = *pBin > 0 ? *pBin / vmax : 0;
        };

        return true;
    };
    void cleanup() {
        kiss_fftr_free(FFTcfg);
        ma_data_converter_uninit(pConverter, NULL);
        delete pBuffer;
        delete[] freq_cpx;
        delete[] freq;
        delete[] bins;
        delete[] binSizes;
    }
};

struct ctx {
    ma_engine* pEngine = NULL;
    AudioBuffer* pBufPlayback = NULL;
    graph *pGraph = NULL;
    FFT* pFFT = NULL;
    Queue<std::string>* pMsgQueue = NULL;
    bool init = false;
};



void print_audio_status(AudioStatus status);

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