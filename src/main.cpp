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

    auto p = Player(0.1, argv[1]);

    if (p.status == SUCCESS) {
        p.play();
    }

    getchar();
    p.cleanup();
    return 0;

    initscr();
    start_color();
    use_default_colors();
    cbreak();
    noecho();
    refresh();
    init_pair(1, COLOR_WHITE, -1);
    attron(COLOR_PAIR(1));
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);
    curs_set(0);

    int N = 15;

    graph g = graph(10, N, 5, 5);
    int i = 2;
    for (auto& segment : g.segments) {
        segment.set_color(i++);
    }

    std::vector<float> activations(15);

    int t = 0;
    float x;
    while (true) {
        for (i = 0; i < N; i++) {
            x = activations[i] = (1 + sin((i + t) * M_PI / 10)) / 2;
        }
        refresh();
        g.update_activations(activations);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        t++;
    }

    endwin();
    return 0;
}