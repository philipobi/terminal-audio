#include "graph.h"
#include <ncurses.h>

bar::bar(int height, int width, int y, int x) : height(height), width(width), y(y), x(x)
{
    //p_win = newwin(height, width, y_pos, x_pos);
}

bar::~bar()
{
    delwin(p_win);
}

void bar::draw(int pair)
{
    wbkgd(p_win, COLOR_PAIR(pair));
    wrefresh(p_win);
}

void bar::make_segments(){
    int h1 = int(0.2 * height) || 2;
    int h2 = int(0.1 * height) || 1;
    int h3 = height - (h1 + h2);
    int heights[] = {h1, h2, h3};
    p_win = (WINDOW**)malloc(3*sizeof(WINDOW *));
    for(int i=0, y_s = y; i<3; i++){
        p_win[i] = newwin()
    }
}




graph::graph(int y, int x, int n_bars, int bar_width, int bar_height, int bar_margin) : n_bars(n_bars)
{
    bars.reserve(n_bars);
    for (int i = 0, x_bar = x; i < n_bars; i++, x_bar += (bar_width + bar_margin))
    {
        bars.emplace_back(bar_height, bar_width, y, x_bar);
        bars[i].draw(i % 2 + 1);
    }
}