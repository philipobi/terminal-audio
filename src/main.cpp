#include <ncurses.h>
#include <vector>
#include <mutex>
#include <filesystem>
#include <string>
#include <condition_variable>
#include <iostream>
#include "frontend.h"
#include "audio.h"
#include "utils.h"

std::mutex mtx;
std::condition_variable cv;

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

    std::string fname = std::filesystem::path(argv[1]).replace_extension("").filename().string();

    auto ui = UI(2, 2, fname);
    std::unique_ptr<Window> &pContainer = ui.pContainer;
    keypad(pContainer->p_win.get(), true);

    std::unique_lock<std::mutex> lck(mtx);
    lck.unlock();

    auto p = Player(argv[1], ui);

    if (p.status == SUCCESS)
    {
        lck.lock();
        p.play();
        lck.unlock();

        bool run_loop = true;
        while (run_loop)
        {
            switch (wgetch(pContainer->p_win.get()))
            {
            case 'p':
                lck.lock();
                if (p.playing())
                    p.pause();
                else
                    p.play();
                lck.unlock();
                break;
            case 'q':
                lck.lock();
                p.pause();
                run_loop = false;
                lck.unlock();
                break;
            case 'n':
                lck.lock();
                p.move_playback_cursor(10, false);
                lck.unlock();
                break;
            case 'm':
                lck.lock();
                p.move_playback_cursor(10, true);
                lck.unlock();
                break;
            }
        }
    }

    endwin();

    if (p.status != SUCCESS) print_app_status(p.status);

    return 0;
}