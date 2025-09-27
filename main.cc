#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <list>
#include <vector>
#include <algorithm>
#include <chrono>

#include "ecs.hh"
#include "types.hh"
#include "pvz.hh"
#include "zombie.hh"
#include "global.hh"

using namespace std;

#define KEY_ESC 27

struct userStats_t {
  short waveNumber = 0; // -1 if lost
} userStats;

const char* UserStatsString( userStats_t stats, char *buf ) {
  snprintf( buf, sizeof(buf), "Wave: %d", stats.waveNumber );
  buf[strlen(buf)] = '\0';
  return buf;
}

list<vec2> activeProjectilePositions;

cursorMode_t cursorMode;

void Shoot( long currentTime, uint8_t lane, uint8_t x ) {
  activeProjectilePositions.push_back( 
    (vec2){static_cast<uint8_t>(x + 1), lane} 
  ); 
}

int main( int argc, char **argv ) {

  StartNCurses();

  InitECS();

  cursorMode = NORMAL;
  bool shouldQuit = false;

  auto startTime = chrono::steady_clock::now();

  int y, x;
  getyx( stdscr, y, x );

  char exBuf[256] = { '\0' };

  while ( !shouldQuit ) {
    // TODO: Use smaller denomination of time.
    auto currentTime = chrono::steady_clock::now();
    long long msecs = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();

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
          if ( y + 1 > winY - 2 ) break;
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

        case 'x': {
          uint8_t id = laneEntities.spaces[x];

          RemoveEntity( id, 0 );
          break;
        }
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
          AddNewPlant( y, x, PEASHOOTER );
          break;
        case 'n':
          AddNewPlant( y, x, NUT );
          break;
        case 'c':
          AddNewPlant( y, x, CHOMPER );
          break;
        case 'z':
          AddNewZombie( y, x, ZOMBIE );
          break;
        case 's':
          AddNewZombie( y, x, ZOMBIE_SPRINTER );
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
        case KEY_ENTER: {
          if ( strcmp(exBuf, "q") == 0 ) {
            endwin();
            exit( 0 );
          } else {
            strncpy( exBuf, "Command not available", 20 );
          }
          cursorMode = NORMAL;
          break;
        }
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
            if ( laneEntities.health[i] <= 0 ) {
              RemoveEntity( i, 0 );
              break;
            }

            vec2 pos = laneEntities.positions[i];
            if ( laneEntities.actionDelays[i] + PEASHOOTER_DELAY < msecs ) {
              Shoot( msecs, pos.y, pos.x );
              laneEntities.actionDelays[i] = msecs;
            }
          }
          break;

        case NUT: {
          if ( laneEntities.health[i] <= 0 ) {
            RemoveEntity( i, 0 );
            break;
          }
          break;
        }

        case CHOMPER: {
            uint8_t _x = laneEntities.positions[i].x;
            if ( laneEntities.spaces[_x+1] != -1 
                 && laneEntities.types[laneEntities.spaces[_x+1]] == ZOMBIE ) {
              laneEntities.states[i] = EATING;
              laneEntities.health[laneEntities.spaces[_x+1]] = 0;
              laneEntities.sprites[i].sprite = 'c';
              break;
            }
          }
          break;
        
        case ZOMBIE_SPRINTER:
        case ZOMBIE:
          UpdateZombieAI( i, laneEntities.types[i], msecs );
          break;

        default:
          break;
      }
    }

        
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
        laneEntities.health[i] -= 1; // TODO: projectile damage
        --proj;
        proj = activeProjectilePositions.erase( proj );
        break;
      }
    }


  }


    attron( COLOR_PAIR(8) );
    for ( int i = 1; i < 6; i += 2 ) {
      mvhline( i, 0, ' ', 48 );
    }
    attroff( COLOR_PAIR(8) );

    mvaddstr( winY - 2, 0, CursorModeString(cursorMode) );
    char userStatsStr[256];
    UserStatsString( userStats, userStatsStr );
    mvaddstr( winY - 4, 0, userStatsStr );
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
