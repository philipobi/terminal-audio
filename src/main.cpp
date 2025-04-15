#include <ncurses.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include "graph.h"

int main()
{
    initscr();
    start_color();
    cbreak();
    noecho();
    refresh();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    curs_set(0);

    int heights[] = {2, 2, 8};
    int x =5, y = 5;
    std::vector<segment> segs;
    int y_off = 0;
    for(int i=0; i<3; i++){
        segs.push_back(segment(heights[i], 15, y+y_off, x));
        y_off += heights[i];
    }
    getch();
    endwin();
    
    return 0;
    
    auto seg1 = segment(2, 15, 5, 5);
    seg1.set_color(3);
    seg1.clear();
    auto seg2 = segment(2, 15, 7, 5);
    seg2.set_color(2);
    seg2.clear();
    auto seg3 = segment(8, 15, 9, 5);
    seg3.set_color(1);
    seg3.clear();
    
    int i = 0;
    for(auto& seg: segs){
        seg.set_color(++i);
        seg.clear();
    }
    
    
    int N = 15;
    
    graph g = graph(10, N, 5, 5);
    i = 1;
    for(auto& segment : g.segments){
        segment.set_color(i++);
    }
    
    
    std::vector<float> activations = {0.9, 0.99, .8, .7, .2, .4, .1};

    g.update_activations(activations);

    
    
    int t = 0;
    while (true) {
        for(i = 0; i<N; i++)
            activations[i] = pow(sin((0.2+t/100)*M_PI), 2);
        t++;
        g.update_activations(activations);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    endwin();
}