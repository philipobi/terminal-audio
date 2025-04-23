#pragma once
#include "miniaudio.h"

struct PlaybackInfo {
    bool
        playing = false,
        end = true;
    ma_uint64
        sampleRate,
        audioFrameCursor,
        audioFrameSize,
        audioDuration;
};