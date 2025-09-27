/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_PVZ_H
#define PUNCH_PVZ_H

#include <ncurses.h>

#define COLOR_GRAY 15
#define COLOR_BROWN 16

class Actor {
  public:
    uint8_t x;
    uint8_t y;
    char sprite;
    
    uint8_t health;

    uint8_t damage;

    short uid;

    Actor( uint8_t x, uint8_t y );
    virtual void Draw( void ) = 0;
};

class Projectile {

};

class Pea : public Projectile {

};

inline void StartNCurses( void ) {
  setlocale(LC_ALL, "");

  initscr();
  keypad(stdscr, TRUE);
  nonl();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  set_escdelay(0);

  if ( has_colors() ) {

    start_color();

    if ( COLORS >= 16 ) {
      init_color( COLOR_GRAY, 200, 200, 200 );
      init_color( COLOR_BROWN, 647, 165, 165 );
    } else {
      printw("No more than 16 colors supported!");
    }

    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_CYAN,    COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    init_pair(8, COLOR_WHITE,   COLOR_GRAY);
    init_pair(9, COLOR_BROWN,   COLOR_BLACK);
  }
}

inline const char* CursorModeString( enum cursorMode_t mode ) {
  switch ( mode ) {
    case NORMAL:
      return "Normal";
    case EX:
      return "Ex";
    case INSERT:
      return "Insert";
    default:
      return "?";
  }
}

#endif /* PUNCH_PVZ_H */
