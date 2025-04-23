#include <ncurses.h>
#include <vector>
#include <algorithm>
#include "frontend.h"
#include "utils.h"
#include "partition.h"
#include "audio.h"

Window::Window(int height, int width, int y, int x) : height(height), width(width), y(y), x(x) {
    //try shared_ptr
    p_win = newwin(height, width, y, x);
}

void Window::set_color(int i) { wattron(p_win, COLOR_PAIR(i)); }
void Window::clear() { werase(p_win); }
void Window::stack_n_vertical(int x, int n, char c, int offset) {
    for (int i = 0, y = height - (1 + offset); i < n; i++, y--)
        mvwaddch(p_win, y, x, c);
}
void Window::stack_n_horizontal(int y, int n, char c, int offset){
    for (int x = 0 + offset; x < n + offset; x++)
        mvwaddch(p_win, y, x, c);
}

UI::UI(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x) :
    nbars(nbars),
    bar_height(bar_height),
    bar_width(bar_width),
    bar_margin(bar_margin),
    height(bar_height + 2 * bar_margin + 1),
    width(nbars* bar_width + (nbars + 1) * bar_margin),
    y(y),
    x(x),
    amplitudes(new double[nbars] { 0 })
{
    int i, j;

    //create bar segments
    partition_fractions<int>(nsegments, segment_ratios, bar_height, segment_heights);

    barSegments.reserve(3);
    int y_off = bar_margin;
    for (i = 0; i < nsegments; i++) {
        barSegments.emplace_back(
            segment_heights[i],
            width - 2 * bar_margin,
            y + y_off,
            x + bar_margin
        );
        y_off += segment_heights[i];
    }
    
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);
    std::reverse(barSegments.begin(), barSegments.end());
    i = 2;
    for (auto& barSegment : barSegments) barSegment.set_color(i++);

    //create footers
    Window** pWindows[4] = {
        &pFooter,
        &pLabels,
        &pRaw,
        &pNorm
    };
    for (i = 0; i < 4; i++) {
        *pWindows[i] = new Window(
            1,
            width - 2 * bar_margin,
            y + y_off,
            x + bar_margin
        );
        y_off++;
    }

    const char* strings[N_BINS] = BIN_LABELS;
    for (i = 0; i < N_BINS; i++) {
        mvwprintw(pLabels->p_win, 0, i * (bar_margin + bar_width), strings[i]);
    }
    wrefresh(pLabels->p_win);
    
    for (i = 0; i < nbars; i++)
        for (j = 0; j < bar_width; j++)
            pFooter->stack_n_vertical(i * (bar_width + bar_margin) + j, 1);
    wrefresh(pFooter->p_win);

    //create progress bar
    pProgressBar = new Window(
        3, player_width + 2, y, x + width + 2
    );
}
UI::~UI() {
    for (auto& barSegment : barSegments) delwin(barSegment.p_win);
    delwin(pFooter->p_win);
    delete[] amplitudes;
}

void UI::update_amplitudes(const double* amplitudes_raw) {
    double* pAmp;
    const double* pAmp_raw;
    for (auto& barSegment : barSegments) barSegment.clear();
    int n, i, j, a;
    for (
        i = 0, pAmp = amplitudes, pAmp_raw = amplitudes_raw;
        i < nbars;
        i++, pAmp++, pAmp_raw++)
    {
        *pAmp = max(*pAmp_raw / 80 + 1, *pAmp * amplitude_decay);
        a = round(max(0, min(*pAmp, 1)) * bar_height);
        for (auto& barSegment : barSegments) {
            n = min(a, barSegment.height);
            for (j = 0; j < bar_width; j++) {
                barSegment.stack_n_vertical(i * (bar_width + bar_margin) + j, n);
            }
            a -= n;
            if (a == 0) break;
        }
    }
    
    if (PLAYER_DEBUG)
    {
        for (
            i = 0, pAmp = amplitudes, pAmp_raw = amplitudes_raw;
            i < N_BINS;
            i++, pAmp++, pAmp_raw++)
        {
            mvwprintw(pNorm->p_win, 0, i * (bar_margin + bar_width), "%.2f", *pAmp);
            mvwprintw(pRaw->p_win, 0, i * (bar_margin + bar_width), "%.2f", *pAmp_raw);
        }
        wrefresh(pRaw->p_win);
        wrefresh(pNorm->p_win);
    }
    
    for (auto& barSegment : barSegments) wrefresh(barSegment.p_win);
}

void UI::update_player(const PlaybackInfo* pPlaybackInfo){
    //todo: try background color as border
    double width = round(player_width * pPlaybackInfo->audioFrameCursor / double(pPlaybackInfo->audioFrameSize));
    mvwprintw(pRaw->p_win, 0, 0, "%f", width);
    pProgressBar->clear();
    pProgressBar->stack_n_horizontal(
        1, 
        width,
        '#',
        1
    );
    box(pProgressBar->p_win, 0, 0);
    wrefresh(pProgressBar->p_win);
}