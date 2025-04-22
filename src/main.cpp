#include <ncurses.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdio.h>
#include "graph.h"
#include "audio.h"


int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("No filename provided\n");
        return -1;
    }

    initscr();
    start_color();
    use_default_colors();
    cbreak();
    noecho();
    refresh();
    init_pair(1, COLOR_WHITE, -1);
    attron(COLOR_PAIR(1));
    curs_set(0);

    int N = N_BINS;

    auto g = Graph(N, 20, 2, 2, 5, 5);
    auto p = Player(N, .3, argv[1]);

    p.context.pGraph = &g;

    if (p.status == SUCCESS) {
        p.play();
    }

    getchar();
    p.cleanup();
    endwin();
    return 0;
}