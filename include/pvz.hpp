/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_PVZ_H
#define PUNCH_PVZ_H

#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <queue>

#include <entt/entt.hpp>
#include <ncurses.h>
#include <vector>

#include "global.hpp"
#include "np.hpp"
#include "types.hpp"

#define COLOR_DEFAULT_ZOMBIE COLOR_RED
#define COLOR_DEFAULT_ZOMBIE_frozen

struct PvZ_t {
  cursorMode_t mode;

  char exBuf[256];
  char cmdBuf[256];

  short waveNumber = 0; // -1 if lost
  uint16_t amountOfSun = 10000;

  entt::entity rowPositions[NUM_ROWS][ROW_LENGTH];
};

void AddPlant(PvZ_t *pvz, entt::registry &registry, uint8_t cost, char sprite,
              uint8_t colorPair, uint8_t health, uint16_t damage,
              damageKind_t damageKind, plant_t type, int x, int y,
              long long msecs);

void AddZombie(PvZ_t *pvz, entt::registry &registry,
               std::queue<entt::entity> *zombieOrder, char sprite,
               uint8_t colorPair, uint8_t health, uint16_t damage,
               damageKind_t damageKind, zombie_t type, int x, int y,
               long long msecs);

inline const char *CursorModeString(enum cursorMode_t mode) {
  switch (mode) {
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

inline void KeyboardInteraction(PvZ_t *pvz, entt::registry &registry,
                                std::queue<entt::entity> *zombieOrder,
                                std::vector<entt::entity> &toDestroy, int &cy,
                                int &cx, int &c, long long msecs) {
  if (pvz->mode == NORMAL) {

    switch (c) {

    // Movement
    case KEY_UP:
    case 'k':
      if (cy - 1 < 0)
        break;
      cy -= 1;
      break;
    case KEY_DOWN:
    case 'j':
      if (cy + 1 > NUM_ROWS - 1)
        break;
      cy += 1;
      break;
    case KEY_LEFT:
    case 'h':
      if (cx - 1 < 0)
        break;
      cx -= 1;
      break;
    case KEY_RIGHT:
    case 'l':
      if (cx + 1 > ROW_LENGTH - 1)
        break;
      cx += 1;
      break;

    case '0':
      cx = 0;
      break;
    case '$':
      cx = ROW_LENGTH - 1;
      break;
    case 'G':
      cy = NUM_ROWS - 1;
      break;

    case 'x': {
      if (pvz->rowPositions[cy][cx] == entt::null)
        break;
#ifdef ENABLE_DEBUG_TOOLS
      toDestroy.push_back(pvz->rowPositions[cy][cx]);
#else
      switch (registry.get<entityKind_t>(pvz->rowPositions[cx]).side) {
      case T_zombie:
        strcpy(pvz->exBuf, "You can't delete the undead!");
        break;
      case T_plant:
        toDestroy.push_back(pvz->rowPositions[cx]);
      case T_proj:
        break;
      }
#endif
    } break;
    case ':':
    case ';':
      pvz->mode = EX;
      pvz->exBuf[0] = '\0';
      break;
    case 'i':
      pvz->mode = INSERT;
      break;
    case KEY_ESC:
      pvz->mode = NORMAL;
      pvz->cmdBuf[0] = '\0';
      break;
    case '\n':
      if (pvz->rowPositions[cy][cx] != entt::null &&
          registry.get<entityKind_t>(pvz->rowPositions[cy][cx]).pl ==
              P_sunflower &&
          registry.get<state_t>(pvz->rowPositions[cy][cx]) == GROWN) {
        pvz->amountOfSun += 50;

        sprite_t updatedSprite =
            registry.get<sprite_t>(pvz->rowPositions[cy][cx]);
        stats_t updatedStats = registry.get<stats_t>(pvz->rowPositions[cy][cx]);
        updatedStats.cooldown = msecs;
        updatedSprite.sprite = SUNFLOWER_SPRITE;

        registry.replace<sprite_t>(pvz->rowPositions[cy][cx], updatedSprite);
        registry.replace<state_t>(pvz->rowPositions[cy][cx], IDLE);
        registry.replace<stats_t>(pvz->rowPositions[cy][cx], updatedStats);
      }
      break;
    case -1:
      break;
    default:
      if (c == 'g' && strcmp(pvz->cmdBuf, "g") == 0) {
        pvz->cmdBuf[0] = '\0';
        cy = 0;
      } else {
        strncat(pvz->cmdBuf, (char *)&c, 1);
      }
      break;
    }
  } else if (pvz->mode == INSERT) {

    if (strlen(pvz->cmdBuf) > 3) {
      strcpy(pvz->exBuf, "No plant by that name");
      pvz->cmdBuf[0] = '\0';
      return;
    }

    switch (c) {
    case KEY_ESC:
      pvz->mode = NORMAL;
      pvz->cmdBuf[0] = '\0';
      return;
    case KEY_BACKSPACE: {
      pvz->cmdBuf[strlen(pvz->cmdBuf) - 1] = '\0';
    } break;
    case '\n': {
      if (pvz->rowPositions[cy][cx] != entt::null) {
        strcpy(pvz->exBuf, "There is already a plant there!");
        pvz->cmdBuf[0] = '\0';
        break;
      }

      if (pvz->cmdBuf[0] == 'p' && pvz->cmdBuf[1] == '\0') {
        AddPlant(pvz, registry, PEASHOOTER_COST, PEASHOOTER_SPRITE, COLOR_GREEN,
                 3, 1, ranged, P_peashooter, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Peashooter");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (pvz->cmdBuf[0] == 'n' && pvz->cmdBuf[1] == '\0') {
        AddPlant(pvz, registry, NUT_COST, NUT_SPRITE, PAIR_BROWN_BLACK, 16, 0,
                 none, P_nut, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Nut");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (pvz->cmdBuf[0] == 's' && pvz->cmdBuf[1] == '\0') {
        AddPlant(pvz, registry, SUNFLOWER_COST, SUNFLOWER_SPRITE, COLOR_YELLOW,
                 2, 0, none, P_sunflower, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Sunflower");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (pvz->cmdBuf[0] == 'c' && pvz->cmdBuf[1] == '\0') {
        AddPlant(pvz, registry, CHOMPER_COST, CHOMPER_SPRITE_READY,
                 COLOR_MAGENTA, 8, 999, melee, P_chomper, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Chomper");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (pvz->cmdBuf[0] == 'r' && pvz->cmdBuf[1] == '\0') {
        AddPlant(pvz, registry, REPEATER_COST, REPEATER_SPRITE, COLOR_GREEN, 3,
                 1, ranged, P_repeater, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Repeater");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (strcmp(pvz->cmdBuf, "sp") == 0) {
        AddPlant(pvz, registry, SNOWPEA_COST, SNOWPEA_SPRITE, COLOR_CYAN, 3, 1,
                 ranged, P_snowpea, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Snowpea");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (strcmp(pvz->cmdBuf, "c") == 0) {
        AddPlant(pvz, registry, CHERRYBOMB_COST, CHERRYBOMB_SPRITE, COLOR_RED,
                 6, 999, ranged, P_snowpea, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Snowpea");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      } else if (strcmp(pvz->cmdBuf, "cb") == 0) {
        AddPlant(pvz, registry, CHERRYBOMB_COST, CHERRYBOMB_SPRITE, COLOR_RED,
                 6, 999, ranged, P_cherrybomb, cx, cy, msecs);
        strcpy(pvz->exBuf, "You planted a Cherrybomb");
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      }
#ifdef ENABLE_DEBUG_TOOLS
      else if (pvz->cmdBuf[0] == 'z' && pvz->cmdBuf[1] == '\0') {
        AddZombie(pvz, registry, zombieOrder, ZOMBIE_SPRITE, COLOR_RED, 8, 1,
                  melee, Z_default, cx, cy, msecs);
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;

      } else if (strcmp(pvz->cmdBuf, "zc") == 0) {
        AddZombie(pvz, registry, zombieOrder, ZOMBIE_SPRITE, COLOR_ORANGE, 16,
                  1, melee, Z_conehead, cx, cy, msecs);
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;

      } else if (strcmp(pvz->cmdBuf, "zs") == 0) {
        AddZombie(pvz, registry, zombieOrder, ZOMBIE_SPRITE, COLOR_CYAN, 8, 1,
                  melee, Z_sprinter, cx, cy, msecs);
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;

      } else if (strcmp(pvz->cmdBuf, "zb") == 0) {
        AddZombie(pvz, registry, zombieOrder, ZOMBIE_SPRITE, COLOR_LIGHTGRAY,
                  32, 1, melee, Z_conehead, cx, cy, msecs);
        pvz->cmdBuf[0] = '\0';
        pvz->mode = NORMAL;
        break;
      }
#endif

      strcpy(pvz->exBuf, "No such plant exists");
      pvz->cmdBuf[0] = '\0';
    } break;
    case -1:
      break;
    default:
      strncat(pvz->cmdBuf, (char *)&c, 1);
    }
  } else if (pvz->mode == EX) {
    switch (c) {
    case KEY_ESC:
      pvz->mode = NORMAL;
      pvz->exBuf[0] = '\0';
      break;
    case '\n': {
      if (strcmp(pvz->exBuf, "q") == 0) {
        strncpy(pvz->exBuf, "Save first!", 12);
      } else if (strcmp(pvz->exBuf, "q!") == 0) {
        np::Terminate();
        exit(EXIT_SUCCESS);
      } else if (strcmp(pvz->exBuf, "night") == 0) {
        bkgdset(COLOR_PAIR(COLOR_PAIR_NIGHT_BACKGROUND));
      } else if (strcmp(pvz->exBuf, "day") == 0) {
        bkgdset(COLOR_PAIR(PAIR_WHITE_BLACK));
      } else {
        strncpy(pvz->exBuf, "Command not available", 22);
      }
      pvz->mode = NORMAL;
      break;
    }
    case KEY_BACKSPACE: {
      pvz->exBuf[strlen(pvz->exBuf) - 1] = '\0';
    }
    case -1:
      break;
    default:
      strncat(pvz->exBuf, (char *)&c, 1);
    }
  }
}

inline void DrawBackyard(int x, int y) {
  attron(COLOR_PAIR(8));
  for (int i = 1; i < 6; i += 2) {
    mvhline(i + y, x, ' ', 48);
  }
  attroff(COLOR_PAIR(8));
}

#endif /* PUNCH_PVZ_H */
