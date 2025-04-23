#pragma once
#include <ncurses.h>
#include <vector>
#include "utils.h"
#include "config.h"

class Window
{
    int y, x;

public:
    WINDOW *p_win;
    int width, height;
    Window(int height, int width, int y, int x);

    void set_color(int i);
    void clear();
    void stack_n_vertical(int x, int n, char c = '#', int offset = 0);
    void stack_n_horizontal(int y, int n, char c = '#', int offset = 0);
};

class UI
{
    int
        nbars,
        bar_height,
        bar_width,
        bar_margin,
        player_width = 20,
        window_margin = 2,
        width,
        height,
        y,
        x;
    double *amplitudes = NULL;
    int nsegments = 3;
    float segment_ratios[3] = {0.2, 0.1, 0.7};
    int segment_heights[3];
    float amplitude_decay = 0.8;
    bool
        playerInit = false,
        playerPlaying = false;

    Window
        *pFooter = NULL,
        *pProgressBar = NULL,
        *pPlaybackStatus = NULL,
        *pContainer = NULL;

public:
    std::vector<Window> barSegments;
    UI(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x);
    ~UI();
    void update_amplitudes(const double *amplitudes_raw);
    void update_player(const PlaybackInfo *pPlaybackInfo);
};