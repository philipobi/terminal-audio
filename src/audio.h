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
    ma_uint32 Bps;
    void *const buf = NULL;

public:
    void *ptr = NULL;
    ma_uint32 channels;
    ma_uint64 frameSize, writePos, readPos;
    ma_format format;
    AudioStatus status;
    AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
    ~AudioBuffer();
    void seek(ma_uint64 framePos);
    bool empty();
    bool full();
    void request_write(ma_uint64 *pFrameCount);
    void request_read(ma_uint64 *pFrameCount);
    void clear();
};

class FFT
{
    static constexpr int nbins = N_BINS;
    int frequencies[nbins + 1] = BIN_FREQUENCIES;
    kiss_fft_cpx *frequencyPtrs[nbins + 1];
    kiss_fftr_cfg FFTcfg;
    kiss_fft_cpx *freqdata = NULL;
    fft_numeric *timedata;
    const ma_uint64 N;
    double windowSum;
    void reduce_spectrum();

public:
    AudioStatus status;
    std::vector<double> amplitudesRaw;

    FFT(ma_uint64 N, fft_numeric *timedata, ma_uint32 sampleRate);
    void compute();
    void cleanup();
};

class PlaybackHandler
{
    AudioBuffer
        *pBufMain,
        *pBufPlayback,
        *pBufFFT,
        *tmp;
    ma_decoder *pDecoder;
    ma_data_converter converter, *pConverter;
    ma_uint64
        framesRead,
        framesWrite,
        framePosSec,
        frameTarget,
        offsetFFT;
    FFT *pFFT;
    ma_result decoderStatus;
    UI *pUI;
    PlaybackInfo playbackInfo;
    bool allocated = false;

    void alloc_playback(ma_uint64 frameCount)
    {
        int n = 1;
        while (n * frameCount < FFT_BUFFER_FRAMES)
            n++;
        pUI->set_animation_frames(n-1);

        pBufMain = new AudioBuffer(
            n * frameCount,
            pDecoder->outputChannels,
            pDecoder->outputFormat);
        pBufPlayback = new AudioBuffer(
            n * frameCount,
            pDecoder->outputChannels,
            pDecoder->outputFormat);

        offsetFFT = n * frameCount - FFT_BUFFER_FRAMES;
        allocated = true;
    }

    void compute_time_info(ma_uint64 *pFrameCursor, TimeInfo *pTimeInfo)
    {
        framePosSec = *pFrameCursor / playbackInfo.sampleRate;
        pTimeInfo->h = framePosSec / 3600;
        framePosSec %= 3600;
        pTimeInfo->min = framePosSec / 60;
        framePosSec %= 60;
        pTimeInfo->s = framePosSec;
    }

    void clear_buffers()
    {
        pBufMain->clear();
        pBufPlayback->clear();
    }

    void swap_buffers()
    {
        pBufMain->readPos = 0;
        pBufPlayback->writePos = 0;
        swap_ptr(pBufMain, pBufPlayback, tmp);
    }

public:
    PlaybackHandler(ma_decoder *pDecoder, UI *pUI) : pDecoder(pDecoder), pUI(pUI)
    {
        ma_decoder_get_length_in_pcm_frames(pDecoder, &playbackInfo.audioFrameSize);
        playbackInfo.sampleRate = pDecoder->outputSampleRate;
        compute_time_info(&playbackInfo.audioFrameSize, &playbackInfo.duration);

        pBufFFT = new AudioBuffer(
            FFT_BUFFER_FRAMES,
            1,
            FFT_FORMAT);
        pBufFFT->seek(0);

        auto converterConfig = ma_data_converter_config_init(
            pDecoder->outputFormat,
            FFT_FORMAT,
            pDecoder->outputChannels,
            1,
            playbackInfo.sampleRate,
            playbackInfo.sampleRate);

        ma_data_converter_init(&converterConfig, NULL, &converter);
        pConverter = &converter;

        pFFT = new FFT(
            FFT_BUFFER_FRAMES,
            (fft_numeric *)pBufFFT->ptr,
            pDecoder->outputSampleRate);
    }

    void move_playback_cursor(ma_uint8 s, bool forward)
    {
        frameTarget = s * playbackInfo.sampleRate;
        if (forward)
        {
            frameTarget += playbackInfo.audioFrameCursor;
            if (frameTarget >= playbackInfo.audioFrameSize)
                frameTarget = playbackInfo.audioFrameSize - 1;
        }
        else
        {
            if (frameTarget > playbackInfo.audioFrameCursor)
                frameTarget = 0;
            else
                frameTarget = playbackInfo.audioFrameCursor - frameTarget;
        }
        decoderStatus = ma_decoder_seek_to_pcm_frame(pDecoder, frameTarget);
        if (decoderStatus != MA_SUCCESS)
            return;
        playbackInfo.end = false;
        playbackInfo.audioFrameCursor = frameTarget;
        compute_time_info(&playbackInfo.audioFrameCursor, &playbackInfo.current);
        clear_buffers();
        pUI->clear_amplitudes();
    }

    void play()
    {
        if (playbackInfo.end)
        {
            clear_buffers();
            pUI->clear_amplitudes();
            ma_decoder_seek_to_pcm_frame(pDecoder, 0);
            playbackInfo.end = false;
        }
        playbackInfo.playing = true;
    }

    void pause() { playbackInfo.playing = false; }

    void toggle_play_pause()
    {
        if (playbackInfo.playing)
            pause();
        else
            play();
    }

    void callback(ma_uint64 frameCount, void *pOutput)
    {
        if (!allocated)
            alloc_playback(frameCount);
        if (!playbackInfo.playing)
            return;

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

            playbackInfo.audioFrameCursor += framesRead;
            compute_time_info(&playbackInfo.audioFrameCursor, &playbackInfo.current);
            pUI->animate_amplitudes();
        }
        else if (pBufPlayback->empty() && playbackInfo.end)
        {
            playbackInfo.playing = false;
        }

        if (playbackInfo.end)
            return;

        framesWrite = frameCount;
        pBufMain->request_write(&framesWrite);
        decoderStatus = ma_decoder_read_pcm_frames(
            pDecoder,
            pBufMain->ptr,
            framesWrite,
            &framesRead);
        pBufMain->writePos += framesRead;

        if (decoderStatus == MA_AT_END)
        {
            playbackInfo.end = true;
            swap_buffers();
            return;
        }

        if (pBufMain->full())
        {
            framesRead = framesWrite = FFT_BUFFER_FRAMES;
            pBufMain->seek(offsetFFT);
            ma_data_converter_process_pcm_frames(
                pConverter,
                pBufMain->ptr,
                &framesRead,
                pBufFFT->ptr,
                &framesWrite);
            swap_buffers();

            pFFT->compute();

            // for (int i = 0; i < pFFT->magnitudesRaw.size(); i++)
            //     mvwprintw(
            //         pUI->pContainer->p_win.get(),
            //         0, 4 * i, "%.0f", pFFT->magnitudesRaw[i]);
            // pUI->pContainer->refresh();

            pUI->set_target_amplitudes(pFFT->amplitudesRaw);
        }
    }
};

class Player
{
    ma_device device, *pDevice = NULL;
    ma_decoder decoder, *pDecoder = NULL;
    PlaybackHandler
        *pPlaybackHandler = NULL,
        **ppPlaybackHandler = &pPlaybackHandler;

public:
    enum AudioStatus status;
    Player(const char *filePath, UI *pUI);
    void toggle_play_pause();
    void play();
    void move_playback_cursor(ma_uint8 s, bool forward);
    void cleanup();
};