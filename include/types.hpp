/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_TYPES_H
#define PUNCH_TYPES_H

#include <cstdint>
#include <entt/entity/fwd.hpp>

#define MAX_EFFECTS 6

enum cursorMode_t { NORMAL, EX, INSERT };

/* Components */

struct position_t {
  uint8_t x;
  uint8_t y;
};

struct sprite_t {
  char sprite;
  int colorNum;
};

struct velocity_t {
  int8_t dx;
  int8_t dy;
};

struct organization_t {
  bool inMob;
};

typedef uint16_t damage_t;

enum effect_t {
  E_default = 0,
  E_freeze = 1 << 0, // 0001
  E_poison = 1 << 1, // 0010
  E_speed  = 1 << 2, // 0100
} __attribute__((__packed__));

struct stats_t {
  uint8_t health;
  damage_t damage;
  long long cooldown;
  uint8_t effects;
};

enum damageKind_t { ranged, melee, none };

struct targetedEntity_t {
  entt::entity target;
  bool reachedDest;
} __attribute__((__packed__));

enum modifier_t { M_normal };

enum entityTeam_t { T_zombie, T_plant, T_proj } __attribute__((__packed__));

enum plant_t {
  P_peashooter,
  P_nut,
  P_chomper,
  P_snowpea,
  P_repeater,
  P_sunflower
} __attribute__((__packed__));

enum zombie_t {
  Z_default,
  Z_sprinter,
  Z_brute,
  Z_conehead,
  Z_flag,
  Z_poleVaulter,
  Z_bucketHead,
  Z_newspaper,
  Z_screenDoor,
  Z_football
} __attribute__((__packed__));

struct entityKind_t {
  entityTeam_t side;
  union {
    plant_t pl;
    zombie_t zom;
  };
};

enum state_t { WALKING, EATING, IDLE, GROWN } __attribute__((__packed__));

#endif /* PUNCH_TYPES_H */
