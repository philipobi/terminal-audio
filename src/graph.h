#pragma once
#include <ncurses.h>
#include <vector>

class bar
{
    WINDOW **p_win;
    int height, width, y, x, heights[3];
public:
    ~bar();
    bar(int height, int width, int y, int x);
    void draw(int pair);
    void set_activation(float a);
    void make_segments();
};

class graph
{
    int n_bars;
    std::vector<bar> bars;

public:
    graph(int y, int x, int n_bars, int bar_width, int bar_height, int bar_margin);
};