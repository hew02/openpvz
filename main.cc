#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <list>
#include <vector>
#include <ctime>

#include "pvz.hh"
#include "config.hh"

using namespace std;

#define KEY_ESC 27

typedef struct {
  uint8_t x;
  uint8_t y;
} vec2;

std::list<vec2> activeProjectilePositions;

vector<short> nextUid = { 0 }; // Per row uid

void PeashooterAI( long currentTime ) {
    /*if ( !(*currentLane).empty() ) {
      Shoot( currentTime );
    }*/
}

typedef struct {
  char sprite;
  int colorNum;
} sprite_t;

enum entityType_t {
  PEASHOOTER,
  ZOMBIE
};

void Shoot( long currentTime, uint8_t lane, uint8_t x ) {
  activeProjectilePositions.push_back( 
    (vec2){static_cast<uint8_t>(x + 1), lane} 
  ); 
}

struct laneEntities_t {
  vector<vec2>         positions;
  vector<sprite_t>     sprites;
  vector<long>         actionDelays;
  vector<entityType_t> types;
  vector<int8_t>      health;
} laneEntities;

void AddNewZombie( uint8_t lane, uint8_t x ) {
  short uid = nextUid[lane]++; 
  laneEntities.sprites[uid] = { 'z', 1 };
  laneEntities.positions[uid] = { x, lane };
  laneEntities.actionDelays[uid] = -NORMAL_ZOMBIE_DELAY;
  laneEntities.types[uid] = ZOMBIE;
  laneEntities.health[uid] = 4;
}

void AddNewPlant( uint8_t lane, uint8_t x ) {
  short uid = nextUid[lane]++; 
  laneEntities.sprites[uid] = { 'p', 2 };
  laneEntities.positions[uid] = { x, lane };
  laneEntities.actionDelays[uid] = -PEASHOOTER_DELAY;
  laneEntities.types[uid] = PEASHOOTER;
  laneEntities.health[uid] = 2;
  // damage 1
}

void RemoveEntity( short uid ) {
  laneEntities.sprites[uid] = { 'd', 2};
}


enum cursorMode_t {
  NORMAL,
  EX,
  INSERT
} cursorMode;

const char* CursorModeString( enum cursorMode_t mode ) {
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

int main( int argc, char **argv ) {

  StartNCurses(); 

  laneEntities.positions.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.sprites.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.actionDelays.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.types.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.health.resize(MAX_ENTITIES_PER_ROW);

  cursorMode = NORMAL;
  bool shouldQuit = false;

  AddNewZombie( 0, 25 );
  AddNewPlant( 0, 4 );

  struct timespec startTime;
  clock_gettime( CLOCK_MONOTONIC, &startTime );


  int y, x;
  getyx( stdscr, y, x );

  char exBuf[256] = { '\0' };

  while ( !shouldQuit ) {
    // TODO: Use smaller denomination of time.
    struct timespec currentTime;
    clock_gettime( CLOCK_MONOTONIC, &currentTime );
    long seconds = currentTime.tv_sec - startTime.tv_sec;

    int winY, winX;
    getmaxyx( stdscr, winY, winX );

    int c = getch();

    if ( cursorMode == NORMAL ) {
      switch ( c ) {
        
        // Movement
        case KEY_UP:
        case 'k':
          if ( y - 1 < 0 ) break;
          y -= 1;
          break;
        case KEY_DOWN:
        case 'j':
          if ( y + 1 > winY ) break;
          y += 1;
          break;
        case KEY_LEFT:
        case 'h':
          if ( x - 1 < 0 ) break;
          x -= 1;
          break;
        case KEY_RIGHT:
        case 'l':
          if ( x + 1 > winX ) break;
          x += 1;
          break;

        case 'x': // Dig up plant?
          break;
        case ':':
        case ';':
          cursorMode = EX;
          break;
        case 'i':
          cursorMode = INSERT;
          break;
        case KEY_ESC:
          cursorMode = NORMAL;
          break;
        default:
          break;
      }
    } else if ( cursorMode == INSERT ) {
      switch ( c ) {
        case KEY_ESC:
          cursorMode = NORMAL;
          break;
        case 'p':
          AddNewPlant( y, x );
          break;
        case -1:
          break;
        default:
          strcpy(exBuf, "No plant by that name");
      }
    } else if ( cursorMode == EX ) {
      switch ( c ) {
        case KEY_ESC:
          cursorMode = NORMAL;
          exBuf[0] = '\0';
          break;
        case -1:
          break;
        default:
          strncat( exBuf, (char*)&c, 1 );
      }
    }


    move( y, x );

    for ( int i = 0; i < laneEntities.types.size(); i++ ) {
      if ( i > nextUid[0] - 1 ) {
        break;
      }
      
      // Actor AI
      switch ( laneEntities.types[i] ) {
        case PEASHOOTER: {
            vec2 pos = laneEntities.positions[i];
            if ( laneEntities.actionDelays[i] + PEASHOOTER_DELAY < seconds ) {
              Shoot( seconds, pos.y, pos.x );
              laneEntities.actionDelays[i] = seconds;
            }
          }
          break;
        case ZOMBIE: {
          if ( laneEntities.health[i] <= 0 ) {
            RemoveEntity( i );
            break;
          }
          if ( laneEntities.actionDelays[i] + NORMAL_ZOMBIE_DELAY < seconds ) {
            laneEntities.positions[i].x -= NORMAL_ZOMBIE_SPEED;
            laneEntities.actionDelays[i] = seconds;
          }
          break;
        }
        default:
          break;
      }
    }


    /*laneEntities..erase(
        remove_if(
          laneEntities.zombies.begin(),
          laneEntities.zombies.end(),
          [](Zombie z) { 
            if ( z.health <= 0 ) {
              return true;
            } else {
              return false;
            }
          }
        ),
        laneEntities.zombies.end()
      );*/
    
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

    for ( int i = 0; i < laneEntities.types.size(); i++ ) {
      if ( i > nextUid[0] - 1 ) {
        break;
      }
      
      if ( laneEntities.types[i] == ZOMBIE
           && x == laneEntities.positions[i].x ) {
        laneEntities.health[i] = 0; // TODO: projectile damage
        --proj;
        proj = activeProjectilePositions.erase( proj );
        break;
      }
    }


  }


    /*attron( COLOR_PAIR(8) );
    for ( int i = 1; i < 12; i += 2 ) {
      mvhline( i, 0, ' ', winX );
    }
    attroff( COLOR_PAIR(8) );*/

    mvaddstr( winY - 2, 0, CursorModeString(cursorMode) );
    mvaddstr( winY - 1, 0, exBuf );


    for ( int i = 0; i < laneEntities.sprites.size(); i++ ) {
      if ( i > nextUid[0] - 1 ) {
        break;
      }

      sprite_t s = laneEntities.sprites[i];
      vec2 pos = laneEntities.positions[i];
      attron( COLOR_PAIR(s.colorNum) );
      mvaddch( pos.y, pos.x, s.sprite );
      attroff( COLOR_PAIR(s.colorNum) );
    }

    // Draw projectiles
    proj = activeProjectilePositions.begin();
    while ( proj != activeProjectilePositions.end() ) {
      mvaddstr( proj->y, proj->x, "•" );
      ++proj;
    }

    move( y, x );
    
    refresh();
    clear();
    
    usleep( 40000 );
  }
  
  endwin();
  exit( 0 );

  return EXIT_SUCCESS;
}
