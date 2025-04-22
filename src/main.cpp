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

    auto g = Graph(N_BINS, 20, 5, 4, 5, 5);
    auto p = Player(argv[1]);

    p.context.pGraph = &g;

    auto p_win = g.segments[0].p_win;

    if (p.status == SUCCESS) {
        p.play();
        char c;
        bool run = true;
        while (run) {
            c = wgetch(p_win);
            switch (c)
            {
            case 'p':
                p.play();
                break;
            case 'q':
                run = false;
                break;
            default:
                break;
            }
        }
    }

    p.cleanup();
    endwin();
    return 0;
}