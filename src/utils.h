#pragma once

#include "miniaudio.h"
struct TimeInfo
{
    uint8_t
        h,
        min,
        s;
};

struct PlaybackInfo
{
    bool
        playing = false,
        end = true;
    ma_uint64
        sampleRate,
        audioFrameCursor,
        audioFrameSize;
    TimeInfo
        current,
        duration;
};

void compute_time_info(
    ma_uint64 frameCursor,
    ma_uint64 sampleRate,
    TimeInfo *pTimeInfo
){
    ma_uint64 framePosSec = frameCursor/sampleRate;
    pTimeInfo->h = framePosSec/3600;
}

#include <mutex>
extern std::mutex syncPlayback;