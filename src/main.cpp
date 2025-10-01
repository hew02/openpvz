#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <iterator>
#include <list>
#include <ncurses.h>
#include <queue>
#include <unistd.h>
#include <vector>

#include <entt/entt.hpp>

#include "include/global.hpp"
#include "include/pvz.hpp"
#include "include/types.hpp"

#define ALLOW_ANIMATIONS
#include "np.hpp"

using namespace std;

const char *UserStatsString(PvZ_t pvz, char *buf, size_t sizeOfBuf) {
  snprintf(buf, sizeOfBuf, "Wave: %d | Sun: %d", pvz.waveNumber,
           pvz.amountOfSun);
  buf[strlen(buf)] = '\0';
  return buf;
}

queue<entt::entity> zombieOrder = {};

void Draw(entt::registry &registry) {
  auto view = registry.view<const position_t, sprite_t>();

  view.each([](const auto &pos, const auto &sprite) {
    attron(COLOR_PAIR(sprite.colorNum));
    mvaddch(pos.y, pos.x, sprite.sprite);
    attroff(COLOR_PAIR(sprite.colorNum));
  });
}

void DamageEntity(entt::registry &registry, entt::entity target, uint8_t damage,
                  effect_t projType) {
  stats_t updatedStats = registry.get<stats_t>(target);
  if (updatedStats.health - damage <= 0) {
    updatedStats.health = 0;
  } else {
    updatedStats.health -= damage;
  }

  if (!(updatedStats.effects & projType)) {
    switch (projType) {
    case E_freeze: {
      updatedStats.effects |= projType;
      sprite_t updatedSprite = registry.get<sprite_t>(target);
      updatedSprite.colorNum = ZOMBIE_DEFAULT_FROZEN;
      registry.replace<sprite_t>(target, updatedSprite);
    } break;
    default:
      break;
    }
  }

  registry.replace<stats_t>(target, updatedStats);
}

void ProjectileSystem(entt::registry &registry,
                      vector<entt::entity> &toDestroy) {
  auto view = registry.view<effect_t, damage_t, position_t, targetedEntity_t>();

  view.each([&](const auto entity, const auto &projType, const auto &damage,
                const auto &pos, const auto &targeting) {
    if (targeting.target == entt::null) {
      return;
    }

    if (zombieOrder.empty()) {
      targetedEntity_t _targeting = registry.get<targetedEntity_t>(entity);
      _targeting.target = entt::null;
      _targeting.reachedDest = false;
      registry.replace<targetedEntity_t>(entity, _targeting);
      return;
    } else if (targeting.target != zombieOrder.front()) {
      targetedEntity_t targeting = registry.get<targetedEntity_t>(entity);
      targeting.target = zombieOrder.front();
      targeting.reachedDest = false;
      registry.replace<targetedEntity_t>(entity, targeting);
    }

    position_t targetPos = registry.get<position_t>(targeting.target);
    if (pos.x >= targetPos.x) {
      targetedEntity_t targeting = registry.get<targetedEntity_t>(entity);
      targeting.target = targeting.target;
      targeting.reachedDest = true;
      toDestroy.push_back(entity);
      registry.replace<targetedEntity_t>(entity, targeting);

      DamageEntity(registry, targeting.target, damage, projType);
    }
  });
}

// TODO this could be decoupled a lot, just being lazy now
void PositionSystem(entt::registry &registry, vector<entt::entity> &toDestroy,
                    PvZ_t *pvz) {
  auto view = registry.view<position_t, velocity_t>();

  view.each([&](const auto entity, const auto &pos, const auto &vel) {
    int8_t nextPos = pos.x + vel.dx;

    if (nextPos < 0 || nextPos > ROW_LENGTH - 1) {
      toDestroy.push_back(entity);
      return;
    }

    entityKind_t kind = registry.get<entityKind_t>(entity);

    if (kind.side == T_proj) {
      registry.replace<position_t>(entity, nextPos, pos.y);
    } else if (pvz->rowPositions[pos.y][nextPos] == entt::null) {
      pvz->rowPositions[pos.y][pos.x] = entt::null;
      pvz->rowPositions[pos.y][nextPos] = entity;
      registry.replace<position_t>(entity, nextPos, pos.y);
    } else {
      entityKind_t kindOfTarget =
          registry.get<entityKind_t>(pvz->rowPositions[pos.y][nextPos]);

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

void SpawnNewBullet(entt::registry &registry, long long msecs, uint8_t x,
                    uint8_t y, effect_t projType) {
  const auto bullet = registry.create();
  registry.emplace<position_t>(bullet, x, y);
  switch (projType) {
  case E_freeze:
    registry.emplace<sprite_t>(bullet, PEA_SPRITE, COLOR_CYAN);
    break;
  default:
    registry.emplace<sprite_t>(bullet, PEA_SPRITE, COLOR_GREEN);
  }
  registry.emplace<velocity_t>(bullet, 1u, 0u);
  registry.emplace<entityKind_t>(bullet, T_proj);
  registry.emplace<damage_t>(bullet, 1u);
  registry.emplace<effect_t>(bullet, projType);
  registry.emplace<targetedEntity_t>(bullet, zombieOrder.front(), false);
}

void StateSystem(entt::registry &registry, vector<entt::entity> &toDestroy,
                 np::np &np, PvZ_t *pvz, long long msecs) {
  auto view = registry.view<state_t, position_t, stats_t, entityKind_t>();

  view.each([&](const auto entity, const auto &state, const auto &pos,
                const auto &stats, const auto &kind) {
    if (stats.health == 0) {
      toDestroy.push_back(entity);
      return;
    }

    switch (state) {
    // TODO: perhaps have speed be an effect?
    case WALKING: {
      if (kind.zom == Z_sprinter &&
          stats.cooldown + SPRINTER_ZOMBIE_SPEED < msecs) {

        // If frzoen by a snowpea, take twice as long
        if (stats.effects & E_freeze &&
            stats.cooldown + (NORMAL_ZOMBIE_SPEED * 2) > msecs)
          break;

        registry.replace<velocity_t>(entity, -1, 0);

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);

        // NOTE: most zombies share the same speed, except for those above..
      } else if (stats.cooldown + NORMAL_ZOMBIE_SPEED < msecs) {

        // If frozen by a snowpea, take twice as long
        if (stats.effects & E_freeze &&
            stats.cooldown + (NORMAL_ZOMBIE_SPEED * 2) > msecs)
          break;

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
        if (stats.cooldown + NORMAL_ZOMBIE_EATING_SPEED > msecs)
          break;
        // Have a plant track its attacker then set that to null?
        entt::entity target = pvz->rowPositions[pos.y][pos.x - 1];

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);

        if (target == entt::null) {
          registry.replace<state_t>(entity, WALKING);
        } else {
          // entityKind_t targetKind = registry.get<entityKind_t>(target);
          DamageEntity(registry, target, stats.damage, {});
        }
      } else {
        // HACK: assumes a chomper as that's the only plant that 'eats'
        if (stats.cooldown + CHOMPER_DELAY < msecs) {
          registry.replace<state_t>(entity, IDLE);
          registry.replace<sprite_t>(entity, CHOMPER_SPRITE_READY,
                                     COLOR_MAGENTA);
        }
#ifdef ALLOW_ANIMATIONS
        else {
          char frames[TOTAL_ANIMATION_TICKS] = {'c', 'C', 'c', 'C'};
          registry.replace<sprite_t>(entity, frames[np.animationTick],
                                     COLOR_LIGHTGRAY);
        }
#endif
      }
    } break;
    case IDLE: {
      if (kind.pl == P_chomper) {
        entt::entity maybeEnemy = pvz->rowPositions[pos.y][pos.x + 1];
        if (maybeEnemy != entt::null &&
            registry.get<entityKind_t>(maybeEnemy).side == T_zombie) {
          DamageEntity(registry, maybeEnemy, 231u, {});
          registry.replace<state_t>(entity, EATING);
          registry.replace<sprite_t>(entity, CHOMPER_SPRITE_EATING,
                                     COLOR_LIGHTGRAY);
          // Start cooldown
          stats_t updatedStats = registry.get<stats_t>(entity);
          updatedStats.cooldown = msecs;
          registry.replace<stats_t>(entity, updatedStats);
        }
      } else if (!zombieOrder.empty() &&
                 stats.cooldown + PEASHOOTER_DELAY < msecs) {

        if (kind.pl == P_peashooter) {
          SpawnNewBullet(registry, msecs, pos.x, pos.y, {});
        } else if (kind.pl == P_repeater) {
          SpawnNewBullet(registry, msecs, pos.x, pos.y, {});
          SpawnNewBullet(registry, msecs, pos.x + 1, pos.y, {});
        } else if (kind.pl == P_snowpea) {
          SpawnNewBullet(registry, msecs, pos.x, pos.y, E_freeze);
        } else {
          break;
        }

        stats_t updatedStats = registry.get<stats_t>(entity);
        updatedStats.cooldown = msecs;
        registry.replace<stats_t>(entity, updatedStats);

      } else if (kind.pl == P_sunflower &&
                 stats.cooldown + SUNFLOWER_DELAY < msecs) {
        sprite_t updatedSprite = registry.get<sprite_t>(entity);

        updatedSprite.sprite = SUNFLOWER_SPRITE_COLLECT;
        registry.replace<sprite_t>(entity, updatedSprite);
        registry.replace<state_t>(entity, GROWN);
      }
    } break;
    default:
      break;
    }
  });
}

void RemoveEffect(stats_t stats, effect_t eff) { stats.effects &= ~E_freeze; }

int main(int argc, char **argv) {
  entt::registry registry;

  np::np np = {};

  np::Init(np);

  init_pair(ZOMBIE_DEFAULT_FROZEN, COLOR_RED, COLOR_CYAN);

  bool shouldQuit = false;

  PvZ_t pvz = {};
  pvz.mode = NORMAL;
  pvz.cmdBuf[0] = '\0';
  pvz.exBuf[0] = '\0';
  for (uint8_t i = 0; i < NUM_ROWS; ++i) {
    fill(begin(pvz.rowPositions[i]), end(pvz.rowPositions[i]), entt::null);
  }

  int cy, cx;
  getyx(stdscr, cy, cx);

  while (!shouldQuit) {

    int c = getch();
    vector<entt::entity> toDestroy;

    KeyboardInteraction(&pvz, registry, &zombieOrder, toDestroy, cy, cx, c,
                        np.currentTime);

    StateSystem(registry, toDestroy, np, &pvz, np.currentTime);
    PositionSystem(registry, toDestroy, &pvz);
    ProjectileSystem(registry, toDestroy);

    np::BeginLoop(np);
    {
      DrawBackyard(0, 0);

      mvaddstr(np.winH - 2, 0, CursorModeString(pvz.mode));
      char userStatsStr[256];
      UserStatsString(pvz, userStatsStr, 256);
      mvaddstr(np.winH - 4, 0, userStatsStr);
      mvaddstr(np.winH - 1, 0, pvz.exBuf);
      mvaddstr(np.winH - 1, np.winW - strlen(pvz.cmdBuf) - 2, pvz.cmdBuf);

      Draw(registry);
      move(cy, cx);
    }
    np::EndLoop();

    for (auto entity : toDestroy) {
      position_t pos = registry.get<position_t>(entity);
      entityKind_t kind = registry.get<entityKind_t>(entity);
      pvz.rowPositions[pos.y][pos.x] = entt::null;
      if (kind.side == T_zombie) {
        zombieOrder.pop();
      }
      registry.destroy(entity);
    }
  }

  np::Terminate();

  return EXIT_SUCCESS;
}
