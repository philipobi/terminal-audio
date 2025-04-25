#include "frontend.h"
#include <ncurses.h>
#include <algorithm>
#include <vector>
#include <string>
#include "config.h"
#include "partition.h"
#include "utils.h"

WINDOW *newwin_rel(const Window *pParent, int nlines, int ncols, int begin_y, int begin_x)
{
    int begin_y_p, begin_x_p;
    getbegyx(pParent->p_win, begin_y_p, begin_x_p);
    return newwin(nlines, ncols, begin_y_p + begin_y, begin_x_p + begin_x);
}

Window::Window(int height, int width, int y, int x, const Window *pParent)
    : height(height), width(width), y(y), x(x)
{
    p_win = pParent == NULL ? newwin(height, width, y, x) : newwin_rel(pParent, height, width, y, x);
}

void Window::set_color(int i)
{
    wattron(p_win, COLOR_PAIR(i));
}
void Window::clear()
{
    werase(p_win);
}

void Window::refresh()
{
    wrefresh(p_win);
}

void Window::stack_n_vertical(int x, int n, char c, int offset)
{
    for (int i = 0, y = height - (1 + offset); i < n; i++, y--)
        mvwaddch(p_win, y, x, c);
}
void Window::stack_n_horizontal(int y, int n, char c, int offset)
{
    for (int x = 0 + offset; x < n + offset; x++)
        mvwaddch(p_win, y, x, c);
}

UI::UI(int y, int x)
    : y(y), x(x), amplitudes(new double[nbars]{0})
{

    pContainer = new Window(height, width, y, x);
    box(pContainer->p_win, 0, 0);
    mvwprintw(pContainer->p_win, 0, 2, "Audio Player");
    pContainer->refresh();

    pBarsContainer = new Window(
        win_bars_height,
        win_bars_width,
        window_margin,
        window_margin,
        pContainer);
    pPlayerContainer = new Window(
        win_bars_height,
        win_player_width,
        window_margin,
        window_margin + win_bars_width + window_margin,
        pContainer);

    // create bars
    partition_fractions<int>(
        nsegments,
        segment_ratios,
        bar_height,
        segment_heights);

    barSegments.reserve(3);

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
    pFooter = new Window(
        1,
        win_bars_width,
        bar_height,
        0,
        pBarsContainer);
    for (int i = 0, j; i < nbars; i++)
        for (j = 0; j < bar_width; j++)
            pFooter->stack_n_vertical(i * (bar_width + bar_margin) + j, 1);
    pFooter->refresh();

    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);
    std::reverse(barSegments.begin(), barSegments.end());
    for (int i = 0, j = 2; i < nsegments; i++, j++)
        barSegments[i].set_color(j);

    // create playback status elements
    pProgressBar = new Window(
        1,
        win_player_width,
        pPlayerContainer->height / 2,
        0,
        pPlayerContainer);
    
    int time_width = 8;
    pTimeCurrent = new Window(
        1,
        time_width,
        pPlayerContainer->height / 2 + 1,
        0,
        pPlayerContainer
    );
    pTimeTotal = new Window(
        1,
        time_width,
        pPlayerContainer->height / 2 + 1,
        pPlayerContainer->width - time_width,
        pPlayerContainer
    );
}

UI::~UI()
{
    for (auto &barSegment : barSegments)
        delwin(barSegment.p_win);
    delwin(pFooter->p_win);
    delete[] amplitudes;
}

void UI::update_amplitudes(const double *amplitudes_raw)
{
    for (auto &barSegment : barSegments)
        barSegment.clear();
    double *pAmp;
    const double *pAmp_raw;
    int n, i, j, a;
    for (i = 0, pAmp = amplitudes, pAmp_raw = amplitudes_raw; i < nbars;
         i++, pAmp++, pAmp_raw++)
    {
        *pAmp = std::max<double>(*pAmp_raw / 80 + 1, *pAmp * amplitude_decay);
        a = round(std::max<double>(0, std::min<double>(*pAmp, 1)) * bar_height);
        for (auto &barSegment : barSegments)
        {
            n = std::min(a, barSegment.height);
            for (j = 0; j < bar_width; j++)
            {
                barSegment.stack_n_vertical(
                    i * (bar_width + bar_margin) + j,
                    n);
            }
            a -= n;
            if (a == 0)
                break;
        }
    }

    for (auto &barSegment : barSegments)
        barSegment.refresh();
}

void UI::update_player(const PlaybackInfo *pPlaybackInfo)
{
    pProgressBar->clear();
    mvwaddch(pProgressBar->p_win, 0, 0, '[');
    mvwaddch(pProgressBar->p_win, 0, pProgressBar->width - 1, ']');
    pProgressBar->stack_n_horizontal(
        0,
        round((pProgressBar->width - 2) * pPlaybackInfo->audioFrameCursor / double(pPlaybackInfo->audioFrameSize)),
        '#', 1);
    pProgressBar->refresh();

    update_time(pTimeCurrent, &pPlaybackInfo->current);
}

void UI::update_time(Window* pWin, const TimeInfo *pTime){
    mvwprintw(pWin->p_win, 0, 0, "%02u:%02u:%02u", pTime->h % 100, pTime->min, pTime->s);
    pWin->refresh();
}