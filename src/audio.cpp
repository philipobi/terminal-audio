#include "audio.h"
#include "miniaudio.h"
#include "utils.h"
#include "config.h"
#include <cmath>
#include <limits>
#include <mutex>
#include <iostream>

float a0 = 0.53836, a1 = 1 - a0;

double hamming(ma_uint64 n, ma_uint64 N)
{
    return a0 - a1 * std::cos(2 * M_PI * double(n) / N);
}

template <class T>
void apply_windowfunc(T *pData, ma_uint64 N,
                      double (*pFunc)(ma_uint64, ma_uint64))
{
    ma_uint64 n;
    for (n = 0; n < N; n++, pData++)
        *pData *= (*pFunc)(n, N);
}

void playback_data_callback(ma_device *pDevice, void *pOutput,
                            const void *pInput, ma_uint32 frameCount)
{
    std::lock_guard<std::mutex> lck(syncPlayback);
    (*((PlaybackHandler **)(pDevice->pUserData)))->callback(frameCount, pOutput);
}

void print_audio_status(AudioStatus status)
{
    switch (status)
    {
    case SUCCESS:
        std::cout << "Initialization successful\n"
                  << std::endl;
        break;
    case ERR_INIT_DEVICE:
        std::cout << "Failed to initialize device\n"
                  << std::endl;
        break;
    case ERR_INIT_DECODER:
        std::cout << "Failed to initialize decoder\n"
                  << std::endl;
        break;
    case ERR_INIT_SOUND:
        std::cout << "Failed to initialize sound\n"
                  << std::endl;
        break;
    case ERR_INIT_BUF:
        std::cout << "Failed to initialize buffer\n"
                  << std::endl;
        break;
    case ERR_INIT_CONV:
        std::cout << "Failed to initialize converter\n"
                  << std::endl;
        break;
    }
}

AudioBuffer::AudioBuffer(ma_uint64 frameSize, ma_uint32 channels,
                         ma_format format)
    : frameSize(frameSize),
      channels(channels),
      format(format),
      writePos(0),
      readPos(0),
      Bps(ma_get_bytes_per_sample(format)),
      buf(malloc(frameSize * channels * Bps))
{
    status = buf == NULL ? ERR_INIT_BUF : SUCCESS;
}

AudioBuffer::~AudioBuffer() { free(buf); }

void AudioBuffer::seek(ma_uint64 framePos)
{
    ptr = (char *)buf + framePos * channels * Bps;
}

void AudioBuffer::request_write(ma_uint64 *pFrameCount)
{
    if (writePos + *pFrameCount > frameSize)
        *pFrameCount = frameSize - writePos;
    seek(writePos);
}

void AudioBuffer::request_read(ma_uint64 *pFrameCount)
{
    if (readPos + *pFrameCount > writePos)
        *pFrameCount = writePos - readPos;
    seek(readPos);
}

bool AudioBuffer::empty()
{
    return readPos == writePos;
}

bool AudioBuffer::full()
{
    return writePos == frameSize;
}

void AudioBuffer::clear()
{
    readPos = 0;
    writePos = 0;
}

FFT::FFT(ma_uint64 N, kiss_fft_scalar *timedata, ma_uint32 sampleRate) : N(N),
                                                                         timedata(timedata)
{
    windowSum = 0;
    for (ma_uint64 n = 0; n < N; n++)
        windowSum += hamming(n, N);

    FFTcfg = kiss_fftr_alloc(N, 0, NULL, NULL);

    freqdata = new kiss_fft_cpx[N];
    kiss_fft_cpx **ppFreq, *pEnd = freqdata + N / 2;
    int i;
    for (i = 0, ppFreq = frequencyPtrs; i < N_BINS + 1; i++, ppFreq++)
    {
        *ppFreq = freqdata + frequencies[i] * N / sampleRate;
        if (*ppFreq > pEnd)
            *ppFreq = pEnd;
    }
}

void FFT::cleanup()
{
    kiss_fftr_free(FFTcfg);
    delete[] freqdata;
}

void FFT::reduce_spectrum()
{
    int i, n;
    double sum;
    kiss_fft_cpx *pFreq0, *pFreq1;
    for (i = 0, pMag_raw = magnitudes_raw; i < N_BINS; i++, pMag_raw++)
    {
        pFreq0 = frequencyPtrs[i];
        pFreq1 = frequencyPtrs[i + 1];
        n = pFreq1 - pFreq0;
        sum = 0;
        while (pFreq0 < pFreq1)
        {
            sum += 2 * std::hypot(double(pFreq0->r), double(pFreq0->i)) / windowSum;
            pFreq0++;
        }
        if (n != 0)
            sum /= n;
        *pMag_raw = 20 * std::log10(sum + 1e-12);
        if (*pMag_raw < vmin)
            vmin = *pMag_raw;
        if (*pMag_raw > vmax)
            vmax = *pMag_raw;
    }
}

void FFT::compute()
{
    apply_windowfunc<kiss_fft_scalar>(timedata, FFT_BUFFER_FRAMES, &hamming);
    kiss_fftr(FFTcfg, timedata, freqdata);
    reduce_spectrum();
}

Player::Player(const char *filePath, const UI *pUI)
{
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = playback_data_callback;
    deviceConfig.pUserData = ppPlaybackHandler;

    ma_decoder_config decoderConfig;

    // clang-format off
    if (
        (status = ma_device_init(NULL, &deviceConfig, &device) == MA_SUCCESS ? SUCCESS : ERR_INIT_DEVICE) == SUCCESS &&
        (
            decoderConfig = ma_decoder_config_init(
                device.playback.format,
                device.playback.channels,
                device.sampleRate
            ),
            true
        ) &&
        (status = ma_decoder_init_file(filePath, &decoderConfig, &decoder) == MA_SUCCESS ? SUCCESS : ERR_INIT_DECODER) == SUCCESS &&
        (
            pPlaybackHandler = new PlaybackHandler(&decoder, pUI),
            true
        )
    ) {
        // clang-format on
        pDevice = &device;
        pDecoder = &decoder;
        ma_device_start(pDevice);
    }
}

void Player::toggle_play_pause() { pPlaybackHandler->toggle_play_pause(); }

void Player::play() { pPlaybackHandler->play(); }

void Player::move_playback_cursor(ma_uint8 s, bool forward) { pPlaybackHandler->move_playback_cursor(s, forward); }

void Player::cleanup()
{
    std::lock_guard<std::mutex> lck(syncPlayback);
    pPlaybackHandler->pause();
    ma_device_uninit(pDevice);
    ma_decoder_uninit(pDecoder);
    delete pPlaybackHandler;
}
