#include <ncurses.h>
#include <vector>
#include <algorithm>
#include "graph.h"
#include "utils.h"
#include "partition.h"
#include "audio.h"

Segment::Segment(int height, int width, int y, int x) : height(height), width(width), y(y), x(x) {
    //try shared_ptr
    p_win = newwin(height, width, y, x);
}

void Segment::set_color(int i) { wattron(p_win, COLOR_PAIR(i)); }
void Segment::clear() { werase(p_win); }
void Segment::place_n(int x, int n, char c) {
    for (int i = 0, y = height - 1; i < n; i++, y--)
        mvwaddch(p_win, y, x, c);
}

Graph::Graph(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x) :
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

    partition_fractions<int>(nsegments, segment_ratios, bar_height, segment_heights);

    segments.reserve(3);
    int y_off = bar_margin;
    for (i = 0; i < nsegments; i++) {
        segments.emplace_back(
            segment_heights[i],
            width - 2 * bar_margin,
            y + y_off,
            x + bar_margin
        );
        y_off += segment_heights[i];
    }

    Segment** pSegments[4] = {
        &pFooter,
        &pLabels,
        &pRaw,
        &pNorm
    };
    for (i = 0; i < 4; i++) {
        *pSegments[i] = new Segment(
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


    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);
    std::reverse(segments.begin(), segments.end());
    i = 2;
    for (auto& segment : segments) segment.set_color(i++);
    pFooter->set_color(2);
    for (i = 0; i < nbars; i++)
        for (j = 0; j < bar_width; j++)
            pFooter->place_n(i * (bar_width + bar_margin) + j, 1);

    wrefresh(pFooter->p_win);
    wrefresh(pLabels->p_win);
}
Graph::~Graph() {
    for (auto& segment : segments) delwin(segment.p_win);
    delwin(pFooter->p_win);
    delete[] amplitudes;
}

void Graph::update(const double* amplitudes_raw) {
    double* pAmp;
    const double* pAmp_raw;
    for (auto& segment : segments) segment.clear();
    int n, i, j, a;
    for (
        i = 0, pAmp = amplitudes, pAmp_raw = amplitudes_raw;
        i < nbars;
        i++, pAmp++, pAmp_raw++)
    {
        *pAmp = max(*pAmp_raw / 80 + 1, *pAmp * amplitude_decay);
        a = round(max(0, min(*pAmp, 1)) * bar_height);
        for (auto& segment : segments) {
            n = min(a, segment.height);
            for (j = 0; j < bar_width; j++) {
                segment.place_n(i * (bar_width + bar_margin) + j, n);
            }
            a -= n;
            if (a == 0) break;
        }
    }
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
    for (auto& segment : segments) wrefresh(segment.p_win);
}