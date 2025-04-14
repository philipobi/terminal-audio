#include <ncurses.h>
#include "graph.h"

int main()
{
    initscr();
    start_color();
    cbreak();
    noecho();
    refresh();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_BLUE);

    graph g = graph(0, 0, 5, 2, 8, 1);

    getch();
    endwin();
}