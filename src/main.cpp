#include <ncurses.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <stdio.h>
#include "graph.h"
#include "miniaudio.h"

ma_engine g_engine;

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    printf("callback\n");
    ma_engine_read_pcm_frames(&g_engine, pOutput, frameCount, NULL);
}

int main(int argc, const char** argv)
{
    if (argc < 2) {
        printf("No filename provided\n");
        return -1;
    }
    
    
    ma_device device;
    ma_sound sound;
    
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.dataCallback = playback_data_callback;
    ma_device_init(NULL, &deviceConfig, &device);
    
    auto engineConfig = ma_engine_config_init();
    engineConfig.pDevice = &device;
    ma_engine_init(&engineConfig, &g_engine);
    ma_engine_set_volume(&g_engine, 0.1);
    ma_sound_init_from_file(&g_engine, argv[1], MA_SOUND_FLAG_STREAM, NULL, NULL, &sound);
    ma_sound_start(&sound);

    printf("Press enter to quit...");
    getchar();

    ma_sound_uninit(&sound);
    ma_engine_uninit(&g_engine);
    ma_device_uninit(&device);

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