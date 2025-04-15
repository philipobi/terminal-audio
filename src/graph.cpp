#include <ncurses.h>
#include <vector>
#include <algorithm>
#include "graph.h"

segment::segment(int height, int width, int y, int x) : height(height), width(width), y(y), x(x) {
    p_win = newwin(height, width, y, x);
}

void segment::set_color(int i) { wattron(p_win, COLOR_PAIR(i)); }
void segment::clear() { werase(p_win); }
void segment::place_n(int x, int n, char c) {
    for (int i = 0, y = height - 1; i < n; i++, y--)
        mvwaddch(p_win, y, x, c);
}

graph::graph(int height, int width, int y, int x) : height(height), width(width), y(y), x(x) {

    std::vector<int> heights = { 2, 1, 7 };

    segments.reserve(3);
    int y_off = 0;
    for (int h : heights) {
        segments.emplace_back(h, width, y + y_off, x);
        y_off += h;
    }
    std::reverse(segments.begin(), segments.end());
}
graph::~graph(){
    for(auto& segment : segments) delwin(segment.p_win);
}

void graph::update_activations(const std::vector<float>& activations) {
    for (auto& segment : segments) segment.clear();
    int act, n;
    auto p_act = activations.begin();
    for (int i = 0; i < width && p_act != activations.end(); i++, p_act++) {
        act = *p_act * height;
        for (auto& segment : segments) {
            n = std::min(act, segment.height);
            segment.place_n(i, n);
            act -= n;
            if (act == 0) break;
        }
    }
    for (auto& segment : segments) wrefresh(segment.p_win);
}