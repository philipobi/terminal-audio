#include <ncurses.h>
#include <vector>
#include <cmath>
#include <mutex>
#include <filesystem>
#include <string>
#include <iostream>
#include "frontend.h"
#include "audio.h"
#include "utils.h"

std::mutex syncPlayback;

int main(int argc, const char **argv)
{
    if (argc < 2)
    {
        std::cout << "No filename provided\n"
                  << std::endl;
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

    auto ui = UI(5, 5);

    std::string fname = std::filesystem::path(argv[1]).replace_extension("").filename().string();

    std::unique_lock<std::mutex> lock(syncPlayback);
    auto p = Player(argv[1], &ui);

    auto p_win = ui.pContainer->p_win.get();
    keypad(p_win, true);
    if (p.status == SUCCESS)
    {
        p.play();
        lock.unlock();

        char c;
        bool run = true;
        while (run)
        {
            c = wgetch(p_win);
            if (c == 'p')
            {
                lock.lock();
                p.toggle_play_pause();
                lock.unlock();
            }
            else if (c == 'q')
            {
                run = false;
            }
            else if (c == 'm')
            {
                lock.lock();
                p.move_playback_cursor(10, true);
                lock.unlock();
            }
            else if (c == 'n')
            {
                lock.lock();
                p.move_playback_cursor(10, false);
                lock.unlock();
            }
        }
    }

    p.cleanup();
    endwin();
    return 0;
}