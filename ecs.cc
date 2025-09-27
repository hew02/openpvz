#include "ecs.hh"

vector<short> nextUid = { 0 }; // Per row uid

laneEntities_t laneEntities;

void InitECS( void ) {
  laneEntities.sprites.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.actionDelays.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.types.resize(MAX_ENTITIES_PER_ROW);
  laneEntities.states.resize(MAX_ENTITIES_PER_ROW);
  fill( begin(laneEntities.spaces), end(laneEntities.spaces), -1 );
}

void AddNewZombie( uint8_t lane, uint8_t x, entityType_t type ) {
  short uid = nextUid[lane]++; 
  laneEntities.spaces[x] = uid;
  laneEntities.positions[uid] = { x, lane };
  laneEntities.types[uid] = type;
  laneEntities.states[uid] = WALKING;

  switch ( type ) {
    case ZOMBIE:
      laneEntities.sprites[uid] = { 'z', 1 };
      laneEntities.actionDelays[uid] = -NORMAL_ZOMBIE_DELAY;
      laneEntities.health[uid] = 6;
      break;
    case ZOMBIE_SPRINTER:
      laneEntities.sprites[uid] = { 'z', 5 };
      laneEntities.actionDelays[uid] = -SPRINTER_ZOMBIE_DELAY;
      laneEntities.health[uid] = 5;
      break;
    default:
      break;
  }
}

void AddNewPlant( uint8_t lane, uint8_t x, entityType_t type ) {
  short uid = nextUid[lane]++; 
  laneEntities.positions[uid] = { x, lane };
  laneEntities.spaces[x] = uid;
  laneEntities.types[uid] = type;
  laneEntities.states[uid] = IDLE;
  // damage 1
  switch ( type ) {
    case PEASHOOTER:
      laneEntities.sprites[uid] = { 'p', 2 };
      laneEntities.actionDelays[uid] = -PEASHOOTER_DELAY;
      laneEntities.health[uid] = 2;
      break;
    case NUT:
      laneEntities.sprites[uid] = { 'n', 9 };
      laneEntities.health[uid] = 8;
      break;
    case CHOMPER:
      laneEntities.sprites[uid] = { 'C', 6 };
      laneEntities.health[uid] = 4;
      break;
    default:
      break;
  }
}

void RemoveEntity( short uid, uint8_t lane ) {
  if ( uid >= MAX_ENTITIES_PER_ROW || uid < 0 ) return;

  short lastUid = nextUid[lane] - 1;
  if ( uid != lastUid ) {
      laneEntities.spaces[laneEntities.positions[uid].x] = -1;
      //laneEntities.positions[uid] = NULL;
      swap(laneEntities.sprites[uid], laneEntities.sprites[lastUid]);
      swap(laneEntities.actionDelays[uid], laneEntities.actionDelays[lastUid]);
      swap(laneEntities.types[uid], laneEntities.types[lastUid]);
      laneEntities.health[uid] = 0;
  }
  
  laneEntities.sprites.pop_back();
  laneEntities.actionDelays.pop_back();
  laneEntities.types.pop_back();

  nextUid[lane]--;
}
