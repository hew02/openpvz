#include "pvz.hpp"
#include "np.hpp"
#include "types.hpp"
#include <cstdint>

/**
 * @brief 
 * 
 * @param pvz 
 * @param registry 
 * @param cost 
 * @param sprite 
 * @param colorPair 
 * @param health 
 * @param damage 
 * @param damageKind 
 * @param type 
 * @param x 
 * @param y 
 *
 * @todo some plants like nuts don't need cooldowns
 */
void AddPlant(PvZ_t *pvz, entt::registry &registry, uint8_t cost, char sprite,
              uint8_t colorPair, uint8_t health, uint16_t damage,
              damageKind_t damageKind, plant_t type, int x, int y, long long msecs) {
  if (pvz->amountOfSun - cost < 0) {
    return;
  } else if (pvz->rowPositions[y][x] != entt::null) {
    strcpy(pvz->exBuf, "A plant is already there!");
    return;
  }
  const auto entity = registry.create();
  registry.emplace<position_t>(entity, x, y);
  registry.emplace<sprite_t>(entity, sprite, colorPair);
  entityKind_t kind = { T_plant, {.pl = type} };
  registry.emplace<entityKind_t>(entity, kind);
  registry.emplace<stats_t>(entity, health, damage, msecs);
  if (damageKind == melee) {
    //registry.emplace<damage_t>(entity, damage);
    registry.emplace<targetedEntity_t>(entity, entt::null, false);
  }
  registry.emplace<state_t>(entity, IDLE);
  pvz->rowPositions[y][x] = entity;
  pvz->amountOfSun -= cost;
}

void AddZombie(PvZ_t *pvz, entt::registry &registry,
               std::queue<entt::entity> *zombieOrder, char sprite,
               uint8_t colorPair, uint8_t health, uint16_t damage,
               damageKind_t damageKind, zombie_t type, int x, int y, long long msecs) {

  const auto entity = registry.create();
  registry.emplace<position_t>(entity, x, y);
  registry.emplace<sprite_t>(entity, sprite, colorPair);
  registry.emplace<velocity_t>(entity, 0, 0);
  entityKind_t kind = { T_zombie, {.zom = type} };
  registry.emplace<entityKind_t>(entity, kind);
  stats_t stats = (stats_t){
    health,
    damage,
    msecs,
    E_default
  };
  registry.emplace<stats_t>(entity, stats);
  registry.emplace<state_t>(entity, WALKING);
  pvz->rowPositions[y][x] = entity;

  zombieOrder->push(entity);
}