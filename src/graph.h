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
        x,
        *activations = NULL;
    int nsegments = 3;
    float segment_ratios[3] = { 0.2, 0.1, 0.7 };
    float activation_decay = 0.5;
    int segment_heights[3];
    Segment *pFooter = NULL;
    Segment *pLabels = NULL;
public:
    std::vector<Segment> segments;
    Graph(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x);
    ~Graph();
    void update_activations(const double* activations);
};