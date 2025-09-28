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
#include <queue>

#include <entt/entt.hpp>

#include "include/global.hpp"
#include "include/pvz.hpp"
#include "include/types.hpp"

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

entt::entity rowPositions[ROW_LENGTH];

cursorMode_t cursorMode;

queue<entt::entity> zombieOrder = {};


void Draw(entt::registry &registry) {
  auto view = registry.view<const position_t, sprite_t>();

  view.each([](const auto &pos, const auto &sprite) {
    attron(COLOR_PAIR(sprite.colorNum));
    mvaddch(pos.y, pos.x, sprite.sprite);
    attroff(COLOR_PAIR(sprite.colorNum));
  });
}

void DamageEntity(entt::registry &registry, entt::entity target, uint8_t damage) {
  stats_t updatedStats = registry.get<stats_t>(target);
  if (updatedStats.health - damage <= 0) {
    updatedStats.health = 0;
  } else {
    updatedStats.health -= damage;
  }
  registry.replace<stats_t>(target, updatedStats);
}

void ProjectileSystem(entt::registry &registry, vector<entt::entity> &toDestroy) {
  auto view = registry.view<projectile_t, damage_t, position_t, targetedEntity_t>();

  view.each([&](const auto entity,
                const auto &projType, 
                const auto &damage,
                const auto &pos, 
                const auto &targeting) {
    if (targeting.target == entt::null) {
      return;
    }

    if (zombieOrder.empty()) {
      targetedEntity_t _targeting =
        registry.get<targetedEntity_t>(entity);
      _targeting.target = entt::null;
      _targeting.reachedDest = false;
      registry.replace<targetedEntity_t>(entity, _targeting);
      return;
    } else if (targeting.target != zombieOrder.front()) {
      targetedEntity_t targeting =
        registry.get<targetedEntity_t>(entity);
      targeting.target = zombieOrder.front();
      targeting.reachedDest = false;
      registry.replace<targetedEntity_t>(entity, targeting);
    }

    position_t targetPos = registry.get<position_t>(targeting.target);
    if (pos.x >= targetPos.x) {
      targetedEntity_t targeting =
        registry.get<targetedEntity_t>(entity);
      targeting.target = targeting.target;
      targeting.reachedDest = true;
      toDestroy.push_back(entity);
      registry.replace<targetedEntity_t>(entity, targeting);

      DamageEntity(registry, targeting.target, damage);
    }
  });
}

// TODO this could be decoupled a lot, just being lazy now
void PositionSystem(entt::registry &registry, 
                    vector<entt::entity> &toDestroy) {
  auto view = registry.view<position_t, velocity_t>();

  view.each([&](const auto entity, 
                const auto &pos,
                const auto &vel) {
    int8_t nextPos = pos.x + vel.dx;

    if (nextPos < 0 || nextPos > ROW_LENGTH - 1) {
      toDestroy.push_back(entity);
      return;
    }

    entityKind_t kind = registry.get<entityKind_t>(entity);

    if (kind.side == T_proj) {
      registry.replace<position_t>(entity, static_cast<uint8_t>(pos.x + vel.dx));
    } else if (rowPositions[nextPos] == entt::null) {
      rowPositions[pos.x] = entt::null;
      rowPositions[nextPos] = entity;
      registry.replace<position_t>(entity, static_cast<uint8_t>(pos.x + vel.dx));
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

  /*for (auto [entity, pos, vel] : view.each()) {
  }*/
}


void StateSystem(entt::registry &registry, 
                 vector<entt::entity> &toDestroy, 
                 long long msecs) {
  auto view = registry.view<state_t, position_t, stats_t, entityKind_t>();

  view.each([&](const auto entity, const auto &state, const auto &pos,
                const auto &stats, const auto &kind) {
    if (stats.health == 0) {
      toDestroy.push_back(entity);
      return;
    }

    switch (state) {
    case WALKING: {
      if (stats.cooldown + NORMAL_ZOMBIE_SPEED < msecs) {
        registry.replace<velocity_t>(entity, -1, 0);

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);
      } else {
        registry.replace<velocity_t>(entity, 0, 0);
      }
    } break;
    case EATING: {
      if (kind.side == T_zombie) {
        if (stats.cooldown + NORMAL_ZOMBIE_EATING_SPEED < msecs) break;
        // Have a plant track its attacker then set that to null?
        entt::entity target = rowPositions[pos.x - 1];

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);

        if (target == entt::null) {
          registry.replace<state_t>(entity, WALKING);
        } else {
          //entityKind_t targetKind = registry.get<entityKind_t>(target);
          DamageEntity(registry, target, stats.damage);
        }
      } else {
        // HACK: assumes a chomper as that's the only plant that 'eats'
        if (stats.cooldown + CHOMPER_DELAY < msecs) {
          registry.replace<state_t>(entity, IDLE);
          registry.replace<sprite_t>(entity,
                                     CHOMPER_SPRITE_READY, 
                                     PAIR_MAGENTA_BLACK);
        } 
#ifdef ALLOW_ANIMATIONS 
        else {
          char frames[TOTAL_ANIMATION_TICKS] = { 'C', 'c', 'C', 'c' };
          registry.replace<sprite_t>(entity,
                                     frames[animationTick], 
                                     PAIR_GRAY_BLACK);
        }
#endif
      }
    } break;
    case SHOOTING: {
      if (!zombieOrder.empty()
          && stats.cooldown + PEASHOOTER_DELAY < msecs) {
        const auto bullet = registry.create();
        registry.emplace<position_t>(bullet, 
                                     static_cast<uint8_t>(pos.x + 1),
                                     pos.y);
        registry.emplace<sprite_t>(bullet, PEA_SPRITE, PAIR_GREEN_BLACK);
        registry.emplace<velocity_t>(bullet, 
                                     static_cast<int8_t>(1), 
                                     static_cast<int8_t>(0));
        registry.emplace<entityKind_t>(bullet, T_proj);
        registry.emplace<damage_t>(bullet, 1u);
        registry.emplace<projectile_t>(bullet, B_default);
        registry.emplace<targetedEntity_t>(bullet, zombieOrder.front(), false);

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);
      }
    } break;
    case IDLE: {
      if (kind.pl == P_chomper) {
        entt::entity maybeEnemy = rowPositions[pos.x+1];
        if (maybeEnemy != entt::null
            && registry.get<entityKind_t>(maybeEnemy).side == T_zombie ) {
          DamageEntity(registry, maybeEnemy, 231u);
          registry.replace<state_t>(entity, EATING);
          registry.replace<sprite_t>(entity,
                                     CHOMPER_SPRITE_EATING, 
                                     PAIR_GRAY_BLACK);
          // Start cooldown
          stats_t updatedStats = registry.get<stats_t>(entity);
          updatedStats.cooldown = msecs;
          registry.replace<stats_t>(entity, updatedStats);
        }
      }
    } break;
    }
  });
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

  long long lastTickTime = 0;

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
        } else if (rowPositions[x] != entt::null) {
          strcpy(exBuf, "A plant is already there!");
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
        registry.emplace<entityKind_t>(entity, T_plant, P_chomper);
        registry.emplace<stats_t>(entity, 8, 999, 0);
        registry.emplace<damage_t>(entity, 999);
        registry.emplace<state_t>(entity, IDLE);
        registry.emplace<targetedEntity_t>(entity, entt::null, false);
        rowPositions[x] = entity;
      } break;
#ifdef ENABLE_DEBUG_TOOLS
      case 'z': {
        const auto entity = registry.create();
        registry.emplace<position_t>(entity, x, y);
        registry.emplace<sprite_t>(entity, ZOMBIE_SPRITE, PAIR_RED_BLACK);
        registry.emplace<velocity_t>(entity, 0, 0);
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

    
    if ( lastTickTime + ANIMATION_TICK_SPEED < msecs ) {
      if (animationTick + 1 >= TOTAL_ANIMATION_TICKS)
        animationTick = 0;
      else 
        animationTick++;
      
      lastTickTime = msecs;
    }

    vector<entt::entity> toDestroy;

    StateSystem(registry, toDestroy, msecs);
    PositionSystem(registry, toDestroy);
    ProjectileSystem(registry, toDestroy);

    clear();
    
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

    Draw(registry);

    move(y, x);

    refresh();
    usleep(60000);

    for (auto entity : toDestroy) {
      position_t pos = registry.get<position_t>(entity);
      entityKind_t kind = registry.get<entityKind_t>(entity);
      rowPositions[pos.x] = entt::null;
      if (kind.side == T_zombie) {
        zombieOrder.pop();
      }
      registry.destroy(entity);
    }
  }

  endwin();
  system("stty sane");

  return EXIT_SUCCESS;
}
