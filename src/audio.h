#pragma once
#include <stdlib.h>
#include <cmath>
#include <memory>
#include "miniaudio.h"
#include "frontend.h"
#include "kiss_fftr.h"
#include "utils.h"
#include "config.h"
#include <thread>
#include <chrono>

void data_converter_uninit(void *pConv);

template <class T>
void adjust_vol(void *buf, ma_uint64 N, float vol);

void adjust_vol_null(void *buf, ma_uint64 N, float vol);

enum AppStatus
{
    SUCCESS,
    ERR_INIT_DEVICE,
    ERR_INIT_DECODER,
    ERR_INIT_SOUND,
    ERR_INIT_BUF,
    ERR_INIT_CONV,
};

void print_app_status(AppStatus status);

class AudioBuffer
{
public:
    ma_uint64 frameSize;
    ma_uint32 channels;
    ma_format format;
    ma_uint64 writePos, readPos;

private:
    ma_uint32 Bps;
    std::unique_ptr<void, void (*)(void *)> buf;

public:
    void *ptr = NULL;
    AppStatus status;
    explicit AudioBuffer(ma_uint64 frameSize, ma_uint32 channels, ma_format format);
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
    std::unique_ptr<kiss_fftr_state, void (*)(void *)> FFTcfg;
    fft_numeric *timedata;
    std::unique_ptr<kiss_fft_cpx> freqdata;
    double windowSum;
    void reduce_spectrum();

public:
    std::vector<double> amplitudesRaw;
    AppStatus status;
    explicit FFT(ma_uint64 N, fft_numeric *timedata, ma_uint32 sampleRate);
    void compute();
};

class PlaybackHandler
{
    ma_result decoderStatus;

    std::unique_ptr<ma_decoder, ma_result (*)(ma_decoder *)> &pDecoder;
    UI &ui;
    float vol;

    std::unique_ptr<AudioBuffer>
        pBufMain,
        pBufPlayback,
        pBufFFT;

    std::unique_ptr<FFT> pFFT;

    ma_data_converter converter;
    std::unique_ptr<ma_data_converter, void (*)(void *)> pConverter;

    ma_uint64
        framesRead,
        framesWrite,
        framePosSec,
        frameTarget,
        offsetFFT;

    void (*p_adjust_vol)(void *buf, ma_uint64 N, float vol);

    void alloc_playback(ma_uint64 frameCount)
    {
        if (status != SUCCESS)
        {
            allocated = true;
            return;
        }

        int n = 1;
        while (n * frameCount < FFT_BUFFER_FRAMES)
            n++;
        ui.set_animation_frames(n - 1);

        pBufMain = std::unique_ptr<AudioBuffer>(
            new AudioBuffer(
                n * frameCount,
                pDecoder->outputChannels,
                pDecoder->outputFormat));
        pBufPlayback = std::unique_ptr<AudioBuffer>(
            new AudioBuffer(
                n * frameCount,
                pDecoder->outputChannels,
                pDecoder->outputFormat));

        offsetFFT = n * frameCount - FFT_BUFFER_FRAMES;
        allocated = true;

        if (
            (status = pBufMain->status) == SUCCESS &&
            (status = pBufPlayback->status) == SUCCESS)
            status = SUCCESS;
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
        std::swap(pBufMain, pBufPlayback);
        pBufMain->writePos = 0;
        pBufPlayback->readPos = 0;
        if (vol != 1.0)
        {
            pBufPlayback->seek(0);
            (*p_adjust_vol)(pBufPlayback->ptr, pBufPlayback->writePos * pBufPlayback->channels, vol);
        }
    }

public:
    bool allocated = false;
    PlaybackInfo playbackInfo;
    AppStatus status;
    PlaybackHandler(
        std::unique_ptr<ma_decoder, ma_result (*)(ma_decoder *)> &pDecoder,
        UI &ui,
        float vol) : pDecoder(pDecoder),
                     ui(ui),
                     vol(vol),
                     pBufFFT(new AudioBuffer(
                         FFT_BUFFER_FRAMES,
                         1,
                         FFT_FORMAT)),
                     pFFT(new FFT(
                         FFT_BUFFER_FRAMES,
                         (fft_numeric *)pBufFFT->ptr,
                         pDecoder->outputSampleRate)),
                     pConverter(NULL, &data_converter_uninit)
    {
        ma_decoder_get_length_in_pcm_frames(pDecoder.get(), &playbackInfo.audioFrameSize);
        playbackInfo.sampleRate = pDecoder->outputSampleRate;
        compute_time_info(&playbackInfo.audioFrameSize, &playbackInfo.duration);
        ui.set_duration(&playbackInfo.duration);

        auto converterConfig = ma_data_converter_config_init(
            pDecoder->outputFormat,
            FFT_FORMAT,
            pDecoder->outputChannels,
            1,
            playbackInfo.sampleRate,
            playbackInfo.sampleRate);

        if (
            (status = pBufFFT->status) == SUCCESS &&
            (status = pFFT->status) == SUCCESS &&
            (status = ma_data_converter_init(&converterConfig, NULL, &converter) == MA_SUCCESS ? SUCCESS : ERR_INIT_CONV) == SUCCESS)
        {
            pConverter = std::unique_ptr<ma_data_converter, void (*)(void *)>(&converter, &data_converter_uninit);
        }

        switch (pDecoder->outputFormat)
        {
        case ma_format_f32:
            p_adjust_vol = &adjust_vol<float>;
            break;
        case ma_format_s16:
            p_adjust_vol = &adjust_vol<int16_t>;
            break;
        case ma_format_s32:
            p_adjust_vol = &adjust_vol<int32_t>;
            break;
        case ma_format_u8:
            p_adjust_vol = &adjust_vol<uint8_t>;
            break;
        default:
            p_adjust_vol = &adjust_vol_null;
            break;
        }
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
        decoderStatus = ma_decoder_seek_to_pcm_frame(pDecoder.get(), frameTarget);
        if (decoderStatus != MA_SUCCESS)
            return;
        playbackInfo.end = false;
        playbackInfo.audioFrameCursor = frameTarget;
        compute_time_info(&playbackInfo.audioFrameCursor, &playbackInfo.current);
        ui.update_player(&playbackInfo);
        clear_buffers();
        ui.clear_amplitudes();
    }

    void play()
    {
        if (playbackInfo.end)
        {
            clear_buffers();
            ui.clear_amplitudes();
            ma_decoder_seek_to_pcm_frame(pDecoder.get(), 0);
            playbackInfo.end = false;
            playbackInfo.audioFrameCursor = 0;
        }
        playbackInfo.playing = true;
    }

    void pause() { playbackInfo.playing = false; }

    void callback(ma_uint64 frameCount, void *pOutput)
    {

        std::lock_guard<std::mutex> lck(mtx);
        if (!allocated)
        {
            alloc_playback(frameCount);
            cv.notify_one();
            return;
        }

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
            ui.update_player(&playbackInfo);
            ui.animate_amplitudes();
        }
        else if (playbackInfo.end)
        {
            playbackInfo.playing = false;
            ui.clear_amplitudes();
            ui.animate_amplitudes();
        }

        if (playbackInfo.end)
            return;

        framesWrite = frameCount;
        pBufMain->request_write(&framesWrite);
        decoderStatus = ma_decoder_read_pcm_frames(
            pDecoder.get(),
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
                pConverter.get(),
                pBufMain->ptr,
                &framesRead,
                pBufFFT->ptr,
                &framesWrite);
            swap_buffers();

            pFFT->compute();

            ui.set_target_amplitudes(pFFT->amplitudesRaw);
        }
    }
};

class Player
{
    PlaybackHandler *pPlaybackHandler_;

    ma_device device;
    std::unique_ptr<ma_device, void (*)(ma_device *)> pDevice;
    ma_decoder decoder;
    std::unique_ptr<ma_decoder, ma_result (*)(ma_decoder *)> pDecoder;
    std::unique_ptr<PlaybackHandler> pPlaybackHandler;

public:
    enum AppStatus status;
    Player(const char *filePath, UI &ui, float vol = 0.2);
    bool playing();
    void play();
    void pause();
    void move_playback_cursor(ma_uint8 s, bool forward);
    bool allocated();
};
