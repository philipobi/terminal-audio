#include <ncurses.h>
#include <vector>
#include <cmath>
#include <mutex>
#include <stdio.h>
#include "frontend.h"
#include "audio.h"
#include "utils.h"

std::mutex syncPlayback;

void move_playback_cursor(ctx *pContext, ma_uint64 frameCount, bool forward){
    std::lock_guard<std::mutex> lck(syncPlayback);
    if(forward){
        frameCount = std::min(
            pContext->playbackInfo.audioFrameCursor + frameCount,
            pContext->playbackInfo.audioFrameSize - 1
        );
    } else {
        if (frameCount > pContext->playbackInfo.audioFrameCursor) frameCount = 0;
        else frameCount = pContext->playbackInfo.audioFrameCursor - frameCount;
    }
    if (ma_decoder_seek_to_pcm_frame(
        pContext->pDecoder,
        frameCount
    ) == MA_SUCCESS) {
        pContext->playbackInfo.audioFrameCursor = frameCount;
        compute_time_info(
            pContext->playbackInfo.audioFrameCursor,
            pContext->playbackInfo.sampleRate,
            &pContext->playbackInfo.current
        );
        pContext->playbackInfo.end = false;
    }
}

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

    auto ui = UI(N_BINS, 10, 2, 1, 5, 5);

    std::unique_lock<std::mutex> initLock(syncPlayback);
    ctx context;
    context.pUI = &ui;
    auto p = Player(&context);

    ma_uint64 frameCount10s;

    auto p_win = ui.barSegments[0].p_win;
    keypad(p_win, true);
    if (
        p.status == SUCCESS &&
        p.load_audio(argv[1]) == SUCCESS
        ) {
        frameCount10s = 10 * context.playbackInfo.sampleRate;

        p.play();
        initLock.unlock();
        
        char c;
        bool run = true;
        while (run) {
            c = wgetch(p_win);
            if(c == 'p'){
                std::unique_lock<std::mutex> lck(syncPlayback);
                if(context.playbackInfo.playing) p.pause();
                else p.play();
                lck.unlock();
            } else if (c == 'q') {
                run = false;
            } else if (c == 'm') {
                move_playback_cursor(&context, frameCount10s, true);
                ui.update_player(&context.playbackInfo);
            } else if (c == 'n') {
                move_playback_cursor(&context, frameCount10s, false);
                ui.update_player(&context.playbackInfo);
            }
        }
    }

    p.cleanup();
    endwin();
    return 0;
}