#pragma once

#include "miniaudio.h"
#include <string>

#define swap_ptr(p1, p2, tmp) (tmp) = (p1); (p1) = (p2); (p2) = (tmp)

struct TimeInfo
{
    unsigned int
        h,
        min,
        s;
};

struct PlaybackInfo
{
    bool
        playing = false,
        end = false;
    ma_uint64
        sampleRate,
        audioFrameCursor = 0,
        audioFrameSize;
    TimeInfo
        current,
        duration;
};

#include <mutex>
#include <condition_variable>
extern std::mutex mtx;
extern std::condition_variable cv;