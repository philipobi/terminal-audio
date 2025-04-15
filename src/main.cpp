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
    
    int N = 15;
    
    graph g = graph(10, N, 5, 5);
    int i = 1;
    for(auto& segment : g.segments){
        segment.set_color(i++);
    }
    std::vector<float> activations = {0.9, 0.99, .8, .7, .2, .4, .1};
    
    g.update_activations(activations);
    
    
    getch();
    endwin();

    return 0;
    
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