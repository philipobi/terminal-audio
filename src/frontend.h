#pragma once
#include <ncurses.h>

#include <vector>

#include "config.h"
#include "utils.h"

class Window
{
    int y, x;

public:
    WINDOW *p_win;
    int width, height;
    Window(int height, int width, int y, int x, const Window *pParent = NULL);

    void set_color(int i);
    void clear();
    void refresh();
    void stack_n_vertical(int x, int n, char c = '#', int offset = 0);
    void stack_n_horizontal(int y, int n, char c = '#', int offset = 0);
};

WINDOW *newwin_rel(const Window *pParent, int nlines, int ncols, int begin_y, int begin_x);

class UI
{
    const int
        y,
        x,
        window_margin = 2,
        nbars = N_BINS,
        bar_height = 10,
        bar_width = 2,
        bar_margin = 1,
        win_bars_width = nbars * bar_width + (nbars - 1) * bar_margin,
        win_bars_height = bar_height + 1,
        win_player_width = 20;

    const int width =
        window_margin + win_bars_width + window_margin + win_player_width + window_margin;

    const int height =
        window_margin + win_bars_height + window_margin;

    double *amplitudes = NULL;
    int nsegments = 3;
    float segment_ratios[3] = {0.2, 0.1, 0.7};
    int segment_heights[3];
    float amplitude_decay = 0.8;
    bool playerInit = false, playerPlaying = false;

    Window
        *pBarsContainer = NULL,
        *pFooter = NULL,
        *pPlayerContainer = NULL,
        *pProgressBar = NULL;

    std::vector<Window> barSegments;

public:
    Window
        *pContainer = NULL,
        *pTimeCurrent = NULL,
        *pTimeTotal = NULL;
    UI(int y, int x);
    ~UI();
    void update_amplitudes(const double *amplitudes_raw);
    void update_player(const PlaybackInfo *pPlaybackInfo);
    void update_time(Window* pWin, const TimeInfo *pTime);
};