#pragma once
#include <ncurses.h>
#include <vector>

class segment {
    WINDOW* p_win;
    int width, y, x;
public:
    int height;
    segment(int height, int width, int y, int x);

    void set_color(int i);
    void clear();
    void place_n(int x, int n, char c = '#');
    void refresh();
    void destruct();
};

class graph {
    int height, width, y, x;
public:
    std::vector<segment> segments;
    graph(int height, int width, int y, int x);
    void update_activations(const std::vector<float>& activations);
};