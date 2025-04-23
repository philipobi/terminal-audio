#include "frontend.h"

#include <ncurses.h>

#include <algorithm>
#include <vector>

#include "config.h"
#include "partition.h"
#include "utils.h"

Window::Window(int height, int width, int y, int x)
    : height(height), width(width), y(y), x(x)
{
  p_win = newwin(height, width, y, x);
}

void Window::set_color(int i)
{
  wattron(p_win, COLOR_PAIR(i));
}
void Window::clear()
{
  werase(p_win);
}
void Window::stack_n_vertical(int x, int n, char c, int offset)
{
  for (int i = 0, y = height - (1 + offset); i < n; i++, y--)
    mvwaddch(p_win, y, x, c);
}
void Window::stack_n_horizontal(int y, int n, char c, int offset)
{
  for (int x = 0 + offset; x < n + offset; x++)
    mvwaddch(p_win, y, x, c);
}

UI::UI(int nbars, int bar_height, int bar_width, int bar_margin, int y, int x)
    : nbars(nbars), bar_height(bar_height), bar_width(bar_width), bar_margin(bar_margin), height(0), width(0), y(y), x(x), amplitudes(new double[nbars]{0})
{
  int i, j, win_bars_width, win_playback_width;

  // create bar segments
  partition_fractions<int>(
      nsegments, segment_ratios, bar_height, segment_heights);

  barSegments.reserve(3);
  win_bars_width = nbars * bar_width + (nbars - 1) * bar_margin;
  height = window_margin;
  for (i = 0; i < nsegments; i++)
  {
    barSegments.emplace_back(segment_heights[i],
                             win_bars_width,
                             y + height,
                             x + window_margin);
    height += segment_heights[i];
  }

  init_pair(2, COLOR_GREEN, -1);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_RED, -1);
  std::reverse(barSegments.begin(), barSegments.end());
  i = 2;
  for (auto &barSegment : barSegments)
    barSegment.set_color(i++);

  // create footer
  pFooter = new Window(1, win_bars_width, y + height, x + window_margin);
  height++;
  for (i = 0; i < nbars; i++)
    for (j = 0; j < bar_width; j++)
      pFooter->stack_n_vertical(i * (bar_width + bar_margin) + j, 1);

  width = window_margin + win_bars_width + window_margin;

  // create playback status elements
  win_playback_width = player_width + 2;
  pProgressBar = new Window(1, win_playback_width, y + height - 2, x + width);
  pPlaybackStatus = new Window(1, win_playback_width, y + height - 1, x + width);

  width += win_playback_width + window_margin;

  height += window_margin;

  pContainer = new Window(height, width, y, x);
  box(pContainer->p_win, 0, 0);
  mvwprintw(pContainer->p_win, 0, 2, "Terminal Audio");
  wrefresh(pContainer->p_win);
  wrefresh(pFooter->p_win);
}
UI::~UI()
{
  for (auto &barSegment : barSegments)
    delwin(barSegment.p_win);
  delwin(pFooter->p_win);
  delete[] amplitudes;
}

void UI::update_amplitudes(const double *amplitudes_raw)
{
  double *pAmp;
  const double *pAmp_raw;
  for (auto &barSegment : barSegments)
    barSegment.clear();
  int n, i, j, a;
  for (i = 0, pAmp = amplitudes, pAmp_raw = amplitudes_raw; i < nbars;
       i++, pAmp++, pAmp_raw++)
  {
    *pAmp = std::max<double>(*pAmp_raw / 80 + 1, *pAmp * amplitude_decay);
    a = round(std::max<double>(0, std::min<double>(*pAmp, 1)) * bar_height);
    for (auto &barSegment : barSegments)
    {
      n = std::min(a, barSegment.height);
      for (j = 0; j < bar_width; j++)
      {
        barSegment.stack_n_vertical(i * (bar_width + bar_margin) + j, n);
      }
      a -= n;
      if (a == 0)
        break;
    }
  }

  for (auto &barSegment : barSegments)
    wrefresh(barSegment.p_win);
}

void UI::update_player(const PlaybackInfo *pPlaybackInfo)
{
  // todo: try background color as border

  pProgressBar->clear();
  mvwaddch(pProgressBar->p_win, 0, 0, '[');
  mvwaddch(pProgressBar->p_win, 0, pProgressBar->width - 1, ']');
  pProgressBar->stack_n_horizontal(
      0,
      round(
          player_width * pPlaybackInfo->audioFrameCursor /
          double(pPlaybackInfo->audioFrameSize)),
      '#',
      1);
    
  wrefresh(pProgressBar->p_win);
}