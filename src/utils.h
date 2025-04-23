#pragma once
#include <mutex>

#define min(a,b) (a)<(b) ? (a) : (b)
#define max(a,b) (a)>(b) ? (a) : (b)
#define square(a) (a)*(a)
#define PLAYER_DEBUG false

extern std::mutex syncPlayback;