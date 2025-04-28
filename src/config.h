#pragma once
#include "miniaudio.h"
#define APP_DEBUG false

#define BIN_FREQUENCIES {20, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000}
#define BIN_LABELS {"20", "60", "1h", "2h", "5h", "1k", "2k", "4k", "8k"}
#define N_BINS 9

#define N_SEGMENTS 3

#include <limits.h>
#define FFT_BUFFER_FRAMES 4096
#define FFT_FORMAT ma_format_f32
#define SAMPLE_NORM 1
typedef float fft_numeric;
