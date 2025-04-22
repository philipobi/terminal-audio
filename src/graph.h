#pragma once
#include <ncurses.h>
#include <vector>

class Segment {
    int width, y, x;
public:
    WINDOW* p_win;
    int height;
    Segment(int height, int width, int y, int x);

    void set_color(int i);
    void clear();
    void place_n(int x, int n, char c = '#');
};

class Graph {
    int
        nbars,
        bar_height,
        bar_width,
        bar_margin,
        width,
        height,
        y,
        x;
    double *amplitudes = NULL;
    int nsegments = 3;
    float segment_ratios[3] = { 0.2, 0.1, 0.7 };
    int segment_heights[3];
    float amplitude_decay = 0.8;
    Segment 
        *pFooter = NULL,
        *pLabels = NULL,
        *pRaw = NULL,
        *pNorm = NULL;
public:
    std::vector<Segment> segments;
    Graph(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x);
    ~Graph();
    void update(const double* amplitudes_raw);
};