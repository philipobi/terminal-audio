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
    ma_uint32 channels;
    ma_uint64 frameSize, writePos, readPos;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
    ~AudioBuffer();
    void seek(ma_uint64 framePos);
    void malloc_frames(ma_uint64 n_frames);
    bool empty();
    bool full();
    void request_write(ma_uint64 *pFrameCount);
    void request_read(ma_uint64 *pFrameCount);
};

class PlaybackHandler
{
    AudioBuffer
        *pBufMain,
        *pBufPlayback,
        *pBufFFT,
        *tmp;
    ma_decoder *pDecoder;
    ma_device *pDevice;
    ma_data_converter *pConverter;
    ma_uint64
        frameCountBuf,
        framesRead,
        framesWrite,
        offsetFFT;

public:
    bool allocated = false;

    PlaybackHandler(ma_decoder *pDecoder, ma_device *pDevice, ma_data_converter *pConverter) : pDecoder(pDecoder),
                                                                                               pDevice(pDevice),
                                                                                               pConverter(pConverter)
    {
        pBufFFT = new AudioBuffer(
            FFT_BUFFER_FRAMES,
            1,
            SAMPLE_FORMAT);
        pBufFFT->seek(0);
    }

    void alloc_playback(ma_uint64 frameCount)
    {
        frameCountBuf = frameCount;
        int n = 1;
        while (n * frameCountBuf < FFT_BUFFER_FRAMES)
            n++;

        pBufMain = new AudioBuffer(
            n * frameCountBuf,
            pDevice->playback.channels,
            pDevice->playback.format);
        pBufPlayback = new AudioBuffer(
            n * frameCountBuf,
            pDevice->playback.channels,
            pDevice->playback.format);

        offsetFFT = n * frameCountBuf - FFT_BUFFER_FRAMES;
        allocated = true;
    }

    void callback(ma_uint64 frameCount, void *pOutput)
    {
        if (!pBufPlayback->empty())
        {
            framesRead = frameCount;
            pBufPlayback->request_read(&framesRead);
            ma_copy_pcm_frames(
                pOutput,
                pBufPlayback->ptr,
                framesRead,
                pBufPlayback->format,
                pBufPlayback->channels);
            pBufPlayback->readPos += framesRead;
        }

        framesWrite = frameCount;
        pBufMain->request_write(&framesWrite);
        ma_decoder_read_pcm_frames(
            pDecoder,
            pBufMain->ptr,
            framesWrite,
            &framesRead);
        pBufMain->writePos += framesRead;

        if (!pBufMain->full())
            return;

        framesRead = framesWrite = FFT_BUFFER_FRAMES;
        pBufMain->seek(offsetFFT);
        ma_data_converter_process_pcm_frames(
            pConverter,
            pBufMain->ptr,
            &framesRead,
            pBufFFT->ptr,
            &framesWrite);

        pBufMain->readPos = 0;
        pBufPlayback->writePos = 0;
        swap_ptr(pBufMain, pBufPlayback, tmp);
    }
};

class FFT
{
    int frequencies[N_BINS + 1] = BIN_FREQUENCIES;
    kiss_fft_cpx *frequencyPtrs[N_BINS + 1];
    AudioBuffer *pBuffer = NULL;
    ma_uint64 frameSizeFFT, frameCountIn, frameCountOut;
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx *freq_cpx = NULL;
    double windowSum, vmin, vmax;
    double *pMag, *pMag_raw;
    void *ptr0;

public:
    ma_data_converter converter, *pConverter = NULL;
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
    PlaybackHandler *pPlaybackHandler = NULL;
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
    PlaybackHandler *pPlaybackHanlder = NULL;

public:
    enum AudioStatus status;
    Player(ctx *pContext);
    void play();
    void pause();
    AudioStatus load_audio(const char *filePath);
    void cleanup();
};