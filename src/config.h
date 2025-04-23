#pragma once
#include "miniaudio.h"

#define BIN_FREQUENCIES {30, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000}
#define BIN_LABELS {"30", "60", "1h", "2h", "5h", "1k", "2k", "4k", "8k"}
#define N_BINS 9

#define FFT_BUFFER_FRAMES 4096
#define SAMPLE_FORMAT ma_format_f32
typedef float sample_type;
