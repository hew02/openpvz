/**
 * @file np.hpp
 * @author hew02
 * @brief A light abstraction over NCurses, primarily for game development.
 * @version 0.1
 * @date 2025-09-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef NP_HPP
#define NP_HPP

#include <chrono>
#include <clocale>
#include <cstdlib>
#include <ncurses.h>
#include <unistd.h>

// More colors
#define COLOR_GRAY 15
#define COLOR_BROWN 16
#define COLOR_LIGHTGRAY 17
#define COLOR_DARKBLUE 18
#define COLOR_ORANGE 19

/* Macros for default color pairs */
#define PAIR_RED_BLACK 1
#define PAIR_GREEN_BLACK 2
#define PAIR_YELLOW_BLACK 3
#define PAIR_BLUE_BLACK 4
#define PAIR_CYAN_BLACK 5
#define PAIR_MAGENTA_BLACK 6
#define PAIR_WHITE_BLACK 7
#define PAIR_BROWN_BLACK 9
#define PAIR_GRAY_BLACK 10
#define PAIR_YELLOW_DARKBLUE 11

#define ONE_SECOND 1000000

#define TOTAL_ANIMATION_TICKS 4

/* Config */
#define ANIMATION_TICK_SPEED 500

namespace np {

struct np {
  std::chrono::microseconds startTime;
  long long currentTime;
  long long lastTickTime = 0;

  int winW;
  int winH;

  uint8_t animationTick;
};

/**
 * @brief Get current time in micro seconds.
 *
 * @return std::chrono::microseconds
 */
inline std::chrono::microseconds Now() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch());
}

inline void Init(np &np) {
  setlocale(LC_ALL, "");

  initscr();
  getmaxyx(stdscr, np.winH, np.winW);
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  cbreak();
  noecho();
  set_escdelay(0);

  if (has_colors()) {
    start_color();

    if (COLORS >= 16) {
      init_color(COLOR_GRAY, 200, 200, 200);
      init_color(COLOR_LIGHTGRAY, 500, 500, 500);
      init_color(COLOR_BROWN, 647, 400, 165);
      init_color(COLOR_DARKBLUE, 0, 100, 300);
      init_color(COLOR_ORANGE, 1000, 502, 0);
    } else {
      printw("No more than 16 colors supported!");
      usleep(ONE_SECOND * 2);
    }

    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(PAIR_BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(PAIR_WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_WHITE, COLOR_GRAY);
    init_pair(PAIR_BROWN_BLACK, COLOR_BROWN, COLOR_BLACK);
    init_pair(COLOR_LIGHTGRAY, COLOR_LIGHTGRAY, COLOR_BLACK);
    init_pair(PAIR_YELLOW_DARKBLUE, COLOR_YELLOW, COLOR_DARKBLUE);
    init_pair(COLOR_ORANGE, COLOR_ORANGE, COLOR_BLACK);

  } else {
    printw("[np] this terminal does not support colors, sorry");
    usleep(ONE_SECOND * 2);
  }

  np.startTime = Now();
}

inline void Terminate(void) {
  endwin();
  system("stty sane");
}

inline void Clear(np np) { erase(); }

inline void BeginLoop(np &np) {
  Clear(np);

  getmaxyx(stdscr, np.winH, np.winW);

  auto currentTime = Now();
  np.currentTime =
    std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - np.startTime)
        .count();

  if (np.lastTickTime + ANIMATION_TICK_SPEED < np.currentTime) {
    if (np.animationTick + 1 >= TOTAL_ANIMATION_TICKS)
      np.animationTick = 0;
    else
      np.animationTick++;

    np.lastTickTime = np.currentTime;
  }
};

inline void EndLoop() {
  refresh();
  usleep(40000);
}

}; // namespace np

#endif