/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_TYPES_H
#define PUNCH_TYPES_H

#include <cstdint>
#include <entt/entity/fwd.hpp>

enum cursorMode_t {
  NORMAL,
  EX,
  INSERT
};

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

struct stats_t {
  uint8_t health;
  uint8_t damage;
  long long cooldown;
};

typedef uint8_t damage_t;

struct targetedEntity_t {
  entt::entity target;
  bool reachedDest;
};

enum modifier_t {
  M_normal
};

enum projectile_t {
  B_default,
  B_freeze
};

enum entityTeam_t { T_zombie, T_plant, T_proj } __attribute__((__packed__));

enum plant_t { P_peashooter, P_nut, P_chomper } __attribute__((__packed__));

enum zombie_t { Z_default, Z_sprinter } __attribute__((__packed__));

struct entityKind_t {
  entityTeam_t side;
  union {
    plant_t pl;
    zombie_t zom;
  };
};

enum state_t { WALKING, EATING, IDLE, SHOOTING };


#endif /* PUNCH_TYPES_H */
