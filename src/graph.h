#pragma once
#include <ncurses.h>
#include <vector>

class segment {
    int width, y, x;
public:
    WINDOW* p_win;
    int height;
    segment(int height, int width, int y, int x);

    void set_color(int i);
    void clear();
    void place_n(int x, int n, char c = '#');
};

class graph {
    int height, width, y, x;
public:
    std::vector<segment> segments;
    graph(int height, int width, int y, int x);
    ~graph();
    void update_activations(const std::vector<float>& activations);
};