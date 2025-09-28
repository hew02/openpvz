#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <iterator>
#include <list>
#include <ncurses.h>
#include <unistd.h>
#include <vector>
#include <stack>

#include <entt/entt.hpp>

#include "include/global.hh"
#include "include/pvz.hh"
#include "include/types.hh"

using namespace std;

struct userStats_t {
  short waveNumber = 0; // -1 if lost
  uint16_t amountOfSun = 10000;
} userStats;

const char *UserStatsString(userStats_t stats, char *buf) {
  snprintf(buf,
           (sizeof(char) * 19) + sizeof(stats.waveNumber) +
               sizeof(stats.amountOfSun),
           "Wave: %d | Sun: %d", stats.waveNumber, stats.amountOfSun);
  buf[strlen(buf)] = '\0';
  return buf;
}

// list<vec2> activeProjectilePositions;

entt::entity rowPositions[ROW_LENGTH];

cursorMode_t cursorMode;

stack<entt::entity> zombieOrder = {};


void Draw(entt::registry &registry) {
  auto view = registry.view<const position_t, sprite_t>();

  view.each([](const auto &pos, const auto &sprite) {
    attron(COLOR_PAIR(sprite.colorNum));
    mvaddch(pos.y, pos.x, sprite.sprite);
    attroff(COLOR_PAIR(sprite.colorNum));
  });

  for (auto [entity, pos, sprite] : view.each()) {
  }
}

void DamageSystem(entt::registry &registry, long long msecs) {
  auto view = registry.view<damage_t, targetedEntity_t>();

  vector<entt::entity> toDestroy;


  view.each([&](const auto entity, const auto &damage, const auto &targeting) {

    if (targeting.reachedDest) {
      toDestroy.push_back(entity);

    }

  });


  for (auto entity : toDestroy) {
    registry.destroy(entity);
  }

}

void ProjectileSystem(entt::registry &registry, long long msecs) {
  auto view = registry.view<position_t, targetedEntity_t>();

  view.each([&](const auto entity, const auto &pos, const auto &targeting) {
    position_t targetPos = registry.get<position_t>(targeting.target);
    if (pos.x >= targetPos.x) {
      targetedEntity_t targeting =
          registry.get<targetedEntity_t>(entity);
      targeting.target = targeting.target;
      targeting.reachedDest = true;
      registry.replace<targetedEntity_t>(entity, targeting);
    }
  });
}

// TODO this could be decoupled a lot, just being lazy now
void PositionSystem(entt::registry &registry, long long msecs) {
  auto view = registry.view<position_t, velocity_t>();

  vector<entt::entity> toDestroy;

  view.each([&registry, &toDestroy](const auto entity, const auto &pos,
                                    const auto &vel) {
    int8_t nextPos = pos.x + vel.dx;

    if (nextPos < 0 || nextPos > ROW_LENGTH - 1) {
      toDestroy.push_back(entity);
      rowPositions[pos.x] = entt::null;
      return;
    }

    entityKind_t kind = registry.get<entityKind_t>(entity);

    if (kind.side == T_proj) {
      registry.replace<position_t>(entity, pos.x + vel.dx);
    } else if (rowPositions[nextPos] == entt::null) {
      rowPositions[pos.x] = entt::null;
      rowPositions[nextPos] = entity;
      registry.replace<position_t>(entity, pos.x + vel.dx);
    } else {
      entityKind_t kindOfTarget =
          registry.get<entityKind_t>(rowPositions[nextPos]);

      switch (kindOfTarget.side) {
      case T_plant:
        registry.replace<state_t>(entity, EATING);
        return;
      case T_zombie:
        // registry.replace<state_t>(entity, EATING);
        //  registry.destroy(entity);
        break;
      case T_proj:
        break;
      }
    }
  });

  for (auto entity : toDestroy) {
    registry.destroy(entity);
  }

  /*for (auto [entity, pos, vel] : view.each()) {
  }*/
}

void DamageEntity(entt::registry &registry, entt::entity attacker,
                  entt::entity target, uint8_t damage) {
  stats_t updatedStats = registry.get<stats_t>(target);
  updatedStats.health -= damage;
  registry.replace<stats_t>(target, updatedStats);
}

void StateSystem(entt::registry &registry, long long msecs) {
  auto view = registry.view<state_t, position_t, stats_t, entityKind_t>();

  vector<entt::entity> toDestroy;

  view.each([&registry, &msecs,
             &toDestroy](const auto entity, const auto &state, const auto &pos,
                         const auto &stats, const auto &kind) {
    if (stats.health == 0) {
      toDestroy.push_back(entity);
      rowPositions[pos.x] = entt::null;
    }

    if (stats.cooldown + NORMAL_ZOMBIE_SPEED > msecs) {
      return;
    }

    stats_t updatedStats = registry.get<stats_t>(entity);
    updatedStats.cooldown = msecs;
    registry.replace<stats_t>(entity, updatedStats);

    switch (state) {
    case WALKING:
      PositionSystem(registry, msecs);
      break;
    case EATING:
      if (kind.side == T_zombie) {
        // Have a plant track its attacker then set that to null?
        entt::entity target = rowPositions[pos.x - 1];
        if (target == entt::null) {
          registry.replace<state_t>(entity, WALKING);
        } else {
          DamageEntity(registry, entity, target, stats.damage);
        }
      }
      break;
    case SHOOTING: {
      if (!zombieOrder.empty()) {
        const auto bullet = registry.create();
        registry.emplace<position_t>(bullet, pos.x + 1, pos.y);
        registry.emplace<sprite_t>(bullet, PEA_SPRITE, PAIR_GREEN_BLACK);
        registry.emplace<velocity_t>(bullet, 1, 0);
        registry.emplace<entityKind_t>(bullet, T_proj);
        registry.emplace<damage_t>(bullet, 1);
        registry.emplace<targetedEntity_t>(bullet, zombieOrder.top(), false);
      }
    }

    break;
    case IDLE:
      break;
    }
  });

  for (auto entity : toDestroy) {
    registry.destroy(entity);
  }
}

int main(int argc, char **argv) {
  entt::registry registry;

  StartNCurses();

  fill(begin(rowPositions), end(rowPositions), entt::null);

  cursorMode = NORMAL;
  bool shouldQuit = false;

  auto startTime = chrono::steady_clock::now();

  int y, x;
  getyx(stdscr, y, x);

  char exBuf[256] = {};
  char cmdBuf[256] = {};

  while (!shouldQuit) {
    // TODO: Use smaller denomination of time.
    auto currentTime = chrono::steady_clock::now();
    long long msecs =
        chrono::duration_cast<chrono::milliseconds>(currentTime - startTime)
            .count();

    int winY, winX;
    getmaxyx(stdscr, winY, winX);

    int c = getch();

    if (cursorMode == NORMAL) {
      switch (c) {

      // Movement
      case KEY_UP:
      case 'k':
        if (y - 1 < 0)
          break;
        y -= 1;
        break;
      case KEY_DOWN:
      case 'j':
        if (y + 1 > NUM_ROWS - 1)
          break;
        y += 1;
        break;
      case KEY_LEFT:
      case 'h':
        if (x - 1 < 0)
          break;
        x -= 1;
        break;
      case KEY_RIGHT:
      case 'l':
        if (x + 1 > ROW_LENGTH - 1)
          break;
        x += 1;
        break;

      case '0':
        x = 0;
        break;
      case '$':
        x = ROW_LENGTH - 1;
        break;
      case 'G':
        y = NUM_ROWS - 1;
        break;

      case 'x': {
        /*uint8_t id = laneEntities.spaces[x];
        RemoveEntity( id, 0 );*/
      } break;
      case ':':
      case ';':
        cursorMode = EX;
        break;
      case 'i':
        cursorMode = INSERT;
        break;
      case KEY_ESC:
        cursorMode = NORMAL;
        cmdBuf[0] = '\0';
        break;
      case -1:
        break;
      default:
        if (c == 'g' && strcmp(cmdBuf, "g") == 0) {
          cmdBuf[0] = '\0';
          y = 0;
        } else {
          strncat(cmdBuf, (char *)&c, 1);
        }
        break;
      }
    } else if (cursorMode == INSERT) {
      switch (c) {
      case KEY_ESC:
        cursorMode = NORMAL;
        break;
      case 'p': {
        if (userStats.amountOfSun - PEASHOOTER_COST < 0) {
          break;
        }
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, PEASHOOTER_SPRITE, PAIR_GREEN_BLACK);
        registry.emplace<entityKind_t>(entity, T_plant, P_peashooter);
        registry.emplace<stats_t>(entity, 4, 1, 0);
        registry.emplace<state_t>(entity, SHOOTING);
        rowPositions[x] = entity;
        userStats.amountOfSun -= PEASHOOTER_COST;
      } break;
      case 'n': {
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, NUT_SPRITE, PAIR_BROWN_BLACK);
        registry.emplace<entityKind_t>(entity, T_plant, P_nut);
        registry.emplace<stats_t>(entity, 14, 0, 0);
        registry.emplace<state_t>(entity, IDLE);
        rowPositions[x] = entity;
      } break;
      case 'c': {
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, CHOMPER_SPRITE_READY,
                                   PAIR_MAGENTA_BLACK);
      } break;
#ifdef ENABLE_DEBUG_TOOLS
      case 'z': {
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, ZOMBIE_SPRITE, PAIR_RED_BLACK);
        registry.emplace<velocity_t>(entity, -1, 0);
        entityKind_t kind = {T_zombie, {.zom = Z_default}};
        registry.emplace<entityKind_t>(entity, kind);
        registry.emplace<stats_t>(entity, 8, 1, 0);
        registry.emplace<state_t>(entity, WALKING);
        rowPositions[x] = entity;
        
        zombieOrder.push(entity);

      } break;
      case 's': {
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, SPRINTER_SPRITE, PAIR_CYAN_BLACK);
      } break;
#endif

      case -1:
        break;
      default:
        strcpy(exBuf, "No plant by that name");
      }
    } else if (cursorMode == EX) {
      switch (c) {
      case KEY_ESC:
        cursorMode = NORMAL;
        exBuf[0] = '\0';
        break;
      case KEY_ENTER: {
        if (strcmp(exBuf, "q") == 0) {
          endwin();
          exit(0);
        } else {
          strncpy(exBuf, "Command not available", 20);
        }
        cursorMode = NORMAL;
        break;
      }
      case -1:
        break;
      default:
        strncat(exBuf, (char *)&c, 1);
      }
    }

    /*for ( int i = 0; i < laneEntities.types.size(); i++ ) {
      if ( i > nextUid[0] - 1 ) {
        break;
      }

      // Actor AI
      switch ( laneEntities.types[i].variant.pl ) {
        case P_PEASHOOTER: {
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

        case P_NUT: {
          if ( laneEntities.health[i] <= 0 ) {
            RemoveEntity( i, 0 );
            break;
          }
          break;
        }

        case P_CHOMPER: {
            uint8_t _x = laneEntities.positions[i].x;
            if ( laneEntities.spaces[_x+1] != -1
                 && laneEntities.types[laneEntities.spaces[_x+1]].side ==
    T_ZOMBIE ) { laneEntities.states[i] = EATING;
              laneEntities.health[laneEntities.spaces[_x+1]] = 0;
              laneEntities.sprites[i].sprite = 'c';
              break;
            }
          }
          break;
      }

      switch ( laneEntities.types[i].variant.zom ) {
        case Z_SPRINTER:
        case Z_DEFAULT:
          UpdateZombieAI( i, laneEntities.types[i], msecs );
          break;

        default:
          break;
      }
    }*/

    // Update
    /*auto proj = activeProjectilePositions.begin();
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

      if ( ecs_GetType(i).side == T_ZOMBIE
           && x == laneEntities.positions[i].x ) {
        laneEntities.health[i] -= 1; // TODO: projectile damage
        --proj;
        proj = activeProjectilePositions.erase( proj );
        break;
      }
    }


  }*/

    curs_set(0);
    attron(COLOR_PAIR(8));
    for (int i = 1; i < 6; i += 2) {
      mvhline(i, 0, ' ', 48);
    }
    attroff(COLOR_PAIR(8));

    mvaddstr(winY - 2, 0, CursorModeString(cursorMode));
    char userStatsStr[256];
    UserStatsString(userStats, userStatsStr);
    mvaddstr(winY - 4, 0, userStatsStr);
    mvaddstr(winY - 1, 0, exBuf);
    mvaddstr(winY - 1, winX - strlen(cmdBuf) - 2, cmdBuf);

    StateSystem(registry, msecs);
    PositionSystem(registry, msecs);
    ProjectileSystem(registry, msecs);
    DamageSystem(registry, msecs);

    Draw(registry);

    /*for ( int i = 0; i < laneEntities.sprites.size(); i++ ) {
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
    }*/

    move(y, x);
    curs_set(1);

    refresh();
    clear();

    usleep(40000);
  }

  endwin();
  system("stty sane");

  return EXIT_SUCCESS;
}
