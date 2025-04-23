#pragma once
#define PLAYER_DEBUG false

#include "miniaudio.h"
struct PlaybackInfo
{
    bool
        playing = false,
        end = true;
    ma_uint64
        sampleRate,
        audioFrameCursor,
        audioFrameSize,
        audioDuration;
};

#include <mutex>
extern std::mutex syncPlayback;