#include "miniaudio.h"
#include "utils.h"

void compute_time_info(
    ma_uint64 frameCursor,
    ma_uint64 sampleRate,
    TimeInfo *pTimeInfo)
{
    ma_uint64 framePosSec = frameCursor / sampleRate;
    pTimeInfo->h = framePosSec / 3600;
    framePosSec %= 3600;
    pTimeInfo->min = framePosSec / 60;
    framePosSec %= 60;
    pTimeInfo->s = framePosSec;
}