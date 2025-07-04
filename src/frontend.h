#pragma once
#include <ncurses.h>
#include <utility>
#include <memory>
#include <vector>

#include "config.h"
#include "utils.h"

class Window
{
public:
    int height, width;
    std::unique_ptr<WINDOW, int (*)(WINDOW *)> p_win;

    explicit Window(int height, int width, int y, int x, const std::unique_ptr<Window> &pParent = NULL);

    void set_color(int i);
    void clear();
    void refresh();
    void stack_n_vertical(int x, int n, char c = '#', int offset = 0);
    void stack_n_horizontal(int y, int n, char c = '#', int offset = 0);
};

WINDOW *newwin_rel(const Window *pParent, int nlines, int ncols, int begin_y, int begin_x);

class Bar
{
    int x,
        i_anim = 1,
        amp0 = 0, amp1 = 0,
        diff = amp1 - amp0;
    void draw(int n);

public:
    static std::vector<Window> *pSegments;
    static int height, width, frameCount;

    explicit Bar(int x);
    void set_target_amplitude(int a);
    void animate();
    void clear();
};

class UI
{
    static constexpr int
        nbars = N_BINS,
        nsegments = N_SEGMENTS,
        window_margin = 2,
        bar_height = 12,
        bar_width = 2,
        bar_margin = 1,
        win_bars_width = nbars * bar_width + (nbars - 1) * bar_margin,
        win_bars_height = bar_height + 1,
        win_player_width = 20,
        width = window_margin + win_bars_width + 2 * window_margin + win_player_width + window_margin,
        height = window_margin + win_bars_height + window_margin;

    std::vector<float> segment_ratios = {0.3, 0.2, 0.5};
    std::vector<int> segment_heights;
    std::vector<Window> barSegments;
    std::vector<Bar> bars;

    std::unique_ptr<Window>
        pBarsContainer,
        pFooter,
        pPlayerContainer,
        pProgressBar,
        pTimeCurrent,
        pTimeDuration,
        pFilenameDisplay;

    void update_time(std::unique_ptr<Window> &pWin, const TimeInfo *pTime);

public:
    std::unique_ptr<Window> pContainer;
        
    explicit UI(int y, int x, const std::string &fname);
    void set_animation_frames(int n);
    void set_target_amplitudes(const std::vector<double> &amplitudesRaw);
    void animate_amplitudes();
    void clear_amplitudes();
    void update_player(const PlaybackInfo *pPlaybackInfo);
    void set_duration(const TimeInfo *pTime);
};