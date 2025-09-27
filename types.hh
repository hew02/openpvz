/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_TYPES_H
#define PUNCH_TYPES_H

#include <cstdlib>

enum cursorMode_t {
  NORMAL,
  EX,
  INSERT
};

enum entitySide_t {
  ZOMBIE,
  PLANT
};

enum plantType_t {
  PEASHOOTER,
  NUT,
  CHOMPER
};

enum zombieType_t {
  DEFAULT_ZOMBIE,
  SPRINTER
};

struct entity_t {
  entitySide_t side;
  union type {
    plantType_t ptype;
    zombieType_t ztype;
  };
};


enum state_t {
  WALKING,
  EATING,
  IDLE
};


struct sprite_t {
  char sprite;
  int colorNum;
};

struct vec2 {
  uint8_t x;
  uint8_t y;
};




#endif /* PUNCH_TYPES_H */
