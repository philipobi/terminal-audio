#pragma once

#include "miniaudio.h"
#include <string>
struct TimeInfo
{
    unsigned int
        h,
        min,
        s;
};

struct PlaybackInfo
{
    std::string fname;
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

void compute_time_info(ma_uint64 frameCursor, ma_uint64 sampleRate, TimeInfo *pTimeInfo);

#include <mutex>
extern std::mutex syncPlayback;