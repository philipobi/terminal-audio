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

    std::unique_lock<std::mutex> initLock(syncPlayback);
    ctx context;
    context.pUI = &ui;
    context.playbackInfo.fname = fname.empty() ? argv[1] : fname;
    auto p = Player(&context);

    ma_uint64 frameCount10s;

    auto p_win = ui.pContainer->p_win;
    keypad(p_win, true);
    if (
        p.status == SUCCESS &&
        p.load_audio(argv[1]) == SUCCESS)
    {
        frameCount10s = 10 * context.playbackInfo.sampleRate;
        compute_time_info(
            context.playbackInfo.audioFrameSize,
            context.playbackInfo.sampleRate,
            &context.playbackInfo.duration);
        ui.update_time(ui.pTimeTotal, &context.playbackInfo.duration);

        p.play();
        initLock.unlock();

        char c;
        bool run = true;
        while (run)
        {
            c = wgetch(p_win);
            if (c == 'p')
            {
                std::unique_lock<std::mutex> lck(syncPlayback);
                if (context.playbackInfo.playing)
                    p.pause();
                else
                    p.play();
                lck.unlock();
            }
            else if (c == 'q')
            {
                run = false;
            }
            else if (c == 'm')
            {
                move_playback_cursor(&context, frameCount10s, true);
                ui.update_player(&context.playbackInfo);
            }
            else if (c == 'n')
            {
                move_playback_cursor(&context, frameCount10s, false);
                ui.update_player(&context.playbackInfo);
            }
        }
    }

    p.cleanup();
    endwin();
    return 0;
}