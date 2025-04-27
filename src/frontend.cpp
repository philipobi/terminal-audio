#include "frontend.h"
#include <ncurses.h>
#include <algorithm>
#include <string>
#include "config.h"
#include "partition.h"
#include "utils.h"

WINDOW *newwin_rel(const Window *pParent, int nlines, int ncols, int begin_y, int begin_x)
{
    int begin_y_p, begin_x_p;
    getbegyx(pParent->p_win.get(), begin_y_p, begin_x_p);
    return newwin(nlines, ncols, begin_y_p + begin_y, begin_x_p + begin_x);
}

Window::Window(int height, int width, int y, int x, const std::unique_ptr<Window> &pParent)
    : height(height), width(width), y(y), x(x),
      p_win(
          pParent ? newwin_rel(pParent.get(), height, width, y, x) : newwin(height, width, y, x),
          &delwin)
{
}

void Window::set_color(int i) { wattron(p_win.get(), COLOR_PAIR(i)); }
void Window::clear() { werase(p_win.get()); }

void Window::refresh() { wrefresh(p_win.get()); }

void Window::stack_n_vertical(int x, int n, char c, int offset)
{
    for (int i = 0, y = height - (1 + offset); i < n; i++, y--)
        mvwaddch(p_win.get(), y, x, c);
}
void Window::stack_n_horizontal(int y, int n, char c, int offset)
{
    for (int x = 0 + offset; x < n + offset; x++)
        mvwaddch(p_win.get(), y, x, c);
}

void Bar::set_segments(std::vector<Window> &segments_) { segments = segments_; }

Bar::Bar(int x) : x(x) {}

void Bar::draw(int n)
{
    int i, j, n1;
    for (auto &segment : segments)
    {
        n1 = std::min(n, segment.height);
        for (i = 1; i <= n1; i++)
            for (j = 0; j < width; j++)
                mvwaddch(
                    segment.p_win.get(),
                    segment.height - i,
                    x + j,
                    '#');
        n -= n1;
        if (n == 0)
            break;
    }
}

void Bar::set_target_amplitude(int a)
{
    i_anim = 1;
    amp0 = amp1;
    amp1 = a;
}

void Bar::animate()
{
    draw(
        std::round(
            amp0 + (amp1 - amp0) *
                       std::min<float>(float(i_anim) / frameCount, 1)));
    ++i_anim;
}

void Bar::clear()
{
    amp1 = 0;
    set_target_amplitude(0);
}

UI::UI(int y, int x)
    : y(y), x(x), amplitudes(new double[nbars]{0})
{

    pContainer = std::unique_ptr<Window>(
        new Window(
            height,
            width,
            y,
            x));

    box(pContainer->p_win.get(), 0, 0);
    mvwprintw(pContainer->p_win.get(), 0, 2, "Audio Player");
    pContainer->refresh();

    pBarsContainer = std::unique_ptr<Window>(
        new Window(
            win_bars_height,
            win_bars_width,
            window_margin,
            window_margin,
            pContainer));
    pPlayerContainer = std::unique_ptr<Window>(
        new Window(
            win_bars_height,
            win_player_width,
            window_margin,
            window_margin + win_bars_width + window_margin,
            pContainer));

    // create bars
    partition_fractions<int>(
        segment_ratios.size(),
        segment_ratios.data(),
        bar_height,
        segment_heights.data());

    for (int i = 0, y_off = 0; i < nsegments; i++)
    {
        barSegments.emplace_back(
            segment_heights[i],
            win_bars_width,
            y_off,
            0,
            pBarsContainer);
        y_off += segment_heights[i];
    }
    std::reverse(barSegments.begin(), barSegments.end());

    Bar::height = bar_height;
    Bar::width = bar_width;
    Bar::set_segments(barSegments);
    for (int i = 0; i < nbars; i++)
        bars.emplace_back(i * (bar_width + bar_margin));

    pFooter = std::unique_ptr<Window>(
        new Window(
            1,
            win_bars_width,
            bar_height,
            0,
            pBarsContainer));

    for (int i = 0, j; i < nbars; i++)
        for (j = 0; j < bar_width; j++)
            pFooter->stack_n_vertical(i * (bar_width + bar_margin) + j, 1);
    pFooter->refresh();

    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);

    int i = 2;
    for (auto &segment : barSegments)
        segment.set_color(i++);

    // create playback status elements
    pProgressBar = std::unique_ptr<Window>(
        new Window(
            1,
            win_player_width,
            pPlayerContainer->height / 2,
            0,
            pPlayerContainer));

    int time_width = 8;
    pTimeCurrent = std::unique_ptr<Window>(
        new Window(
            1,
            time_width,
            pPlayerContainer->height / 2 + 1,
            0,
            pPlayerContainer));
    pTimeTotal = std::unique_ptr<Window>(
        new Window(
            1,
            time_width,
            pPlayerContainer->height / 2 + 1,
            pPlayerContainer->width - time_width,
            pPlayerContainer));
}

void UI::set_animation_frames(int n) { Bar::frameCount = n; }

void UI::set_target_amplitudes(const std::array<double, nbars> &amplitudesRaw)
{
    auto pAmp = amplitudesRaw.begin();
    auto pBar = bars.begin();
    for (; pAmp != amplitudesRaw.end() && pBar != bars.end(); pAmp++, pBar++)
        pBar->set_target_amplitude(
            std::round(
                std::max<double>(
                    0, std::min<double>(1, *pAmp / 80 + 1)) *
                bar_height));
}

void UI::animate_amplitudes()
{
    for (auto &segment : barSegments)
        segment.clear();
    for (auto &bar : bars)
        bar.animate();
    for (auto &segment : barSegments)
        segment.refresh();
}

void UI::update_player(const PlaybackInfo *pPlaybackInfo)
{
    pProgressBar->clear();
    mvwaddch(pProgressBar->p_win.get(), 0, 0, '[');
    mvwaddch(pProgressBar->p_win.get(), 0, pProgressBar->width - 1, ']');
    pProgressBar->stack_n_horizontal(
        0,
        round((pProgressBar->width - 2) * pPlaybackInfo->audioFrameCursor / double(pPlaybackInfo->audioFrameSize)),
        '#', 1);
    pProgressBar->refresh();

    update_time(pTimeCurrent, &pPlaybackInfo->current);
}

void UI::clear_amplitudes()
{
    for (auto &bar : bars)
        bar.clear();
}

void UI::update_time(std::unique_ptr<Window> &pWin, const TimeInfo *pTime)
{
    mvwprintw(pWin->p_win.get(), 0, 0, "%02u:%02u:%02u", pTime->h % 100, pTime->min, pTime->s);
    pWin->refresh();
}