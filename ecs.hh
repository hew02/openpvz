/* Copyright 2025 Punch Software

*/

#ifndef PUNCH_ECS_H
#define PUNCH_ECS_H

#include <vector>

#include "types.hh"
#include "global.hh"

using namespace std;

extern vector<short> nextUid; // Per row uid

struct laneEntities_t {
  short                spaces[48];
  vec2                 positions[MAX_ENTITIES_PER_ROW];
  vector<sprite_t>     sprites;
  vector<long>         actionDelays;
  vector<entity_t> types;
  int8_t               health[MAX_ENTITIES_PER_ROW];
  vector<state_t>      states;
} extern laneEntities;

void InitECS( void );

void AddNewZombie( uint8_t lane, uint8_t x, entity_t type );

void AddNewPlant( uint8_t lane, uint8_t x, entity_t type );

void RemoveEntity( short uid, uint8_t lane );

#endif /* PUNCH_ECS_H */
