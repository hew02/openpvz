#include <cstdlib>
#include <ncurses.h>
#include <csignal>
#include <unistd.h>
#include <list>
#include <ctime>

#define PEASHOOTER_DELAY 4

typedef struct {
  uint8_t x;
  uint8_t y;
} vec2;

std::list<vec2> activeProjectilePositions;

class Projectile {
};

class Pea : public Projectile {

};

class Actor {
  public:
    uint8_t x;
    uint8_t y;
    char sprite;
    
    uint8_t health;

    uint8_t damage;
    
    virtual void Draw( void ) = 0;
};

class Zombie : public Actor {
  public:
    Zombie( uint8_t x, uint8_t y ) {
      this->y = y;
      this->x = x;
      this->sprite = 'z';
    }

    void Draw( void ) override {
      attron( COLOR_PAIR(1) );
      mvaddch( this->y, this->x, this->sprite );
      attroff( COLOR_PAIR(1) );
    }
};

class Peashooter : public Actor {
  public:
    long timeSinceLastShot;

    Peashooter( uint8_t x, uint8_t y ) {
      this->y = y;
      this->x = x;
      this->sprite = 'p';
      this->timeSinceLastShot = -PEASHOOTER_DELAY;
    }

    void Draw( void ) override {
      attron( COLOR_PAIR(2) );
      mvaddch( this->y, this->x, this->sprite );
      attroff( COLOR_PAIR(2) );
    }

    void Shoot( long currentTime ) {
      
      if ( timeSinceLastShot + PEASHOOTER_DELAY < currentTime ) {
        activeProjectilePositions.push_back( (vec2){this->x + 1, this->y} ); 
        timeSinceLastShot = currentTime;
      }
    }
    
};

int main( int argc, char **argv ) {

  initscr();
  keypad(stdscr, TRUE);
  nonl();
  cbreak();
  echo();
  nodelay(stdscr, TRUE);

  if ( has_colors() ) {
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_CYAN,    COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
  }

  bool shouldQuit = false;

  Peashooter p = Peashooter( 10, 5 );
  Zombie z = Zombie( 25, 5 );

  struct timespec startTime;
  clock_gettime( CLOCK_MONOTONIC, &startTime );

  while ( !shouldQuit ) {
    struct timespec currentTime;
    clock_gettime( CLOCK_MONOTONIC, &currentTime );
    long seconds = currentTime.tv_sec - startTime.tv_sec;


    int winY, winX;
    getmaxyx( stdscr, winY, winX );
    
    // Update
    auto proj = activeProjectilePositions.begin();
    while ( proj != activeProjectilePositions.end() ) {
      uint8_t x = proj->x;
      if ( x > winX ) {
        proj = activeProjectilePositions.erase( proj );
        continue;
      }

      proj->x += 1;
      ++proj;
    }

    int y, x;
    getyx( stdscr, y, x );
    int c = getch();

    p.Draw();
    z.Draw();
    p.Shoot( seconds );

    // Draw projectiles
    proj = activeProjectilePositions.begin();
    while ( proj != activeProjectilePositions.end() ) {
      mvaddch( proj->y, proj->x, '.' );
      ++proj;
    }

    move( y, x );

    refresh();
    clear();
    
    usleep( 400000 );
  }
  
  endwin();
  exit( 0 );

  return EXIT_SUCCESS;
}
