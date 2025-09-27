
#include "zombie.hh"
#include "ecs.hh"
#include "types.hh"

void UpdateZombieAI( short i, entityType_t type, long long msecs ) {

  if ( laneEntities.health[i] <= 0 ) {
    RemoveEntity( i, 0 );
    return;
  }

  switch ( laneEntities.states[i] ) {

    case WALKING: {
      switch ( type ) {
        case ZOMBIE: {
          if ( laneEntities.actionDelays[i] + NORMAL_ZOMBIE_DELAY < msecs ) {

            uint8_t zombieX = laneEntities.positions[i].x;

            if ( laneEntities.spaces[zombieX-1] != -1 ) {
              entityType_t typ =
                laneEntities.types[laneEntities.spaces[zombieX-1]];

              if ( typ == ZOMBIE ) {
                laneEntities.positions[i].x = 
                  zombieX - 1;
              } else {
                laneEntities.states[i] = EATING;
              }
              break;
            }

            if ( zombieX - NORMAL_ZOMBIE_SPEED < 0 ) {
              RemoveEntity( i, 0 );
            } else {
              laneEntities.spaces[zombieX] = -1;
              laneEntities.positions[i].x -= NORMAL_ZOMBIE_SPEED;
              laneEntities.spaces[zombieX - NORMAL_ZOMBIE_SPEED] = i;
              laneEntities.actionDelays[i] = msecs;
            }
          }
        }
        break;

        case ZOMBIE_SPRINTER: {
          if ( laneEntities.actionDelays[i] + SPRINTER_ZOMBIE_DELAY < msecs ) {

            uint8_t zombieX = laneEntities.positions[i].x;

            if ( laneEntities.spaces[zombieX-1] != -1 ) {
              entity_t typ =
                laneEntities.types[laneEntities.spaces[zombieX-1]];

              if ( typ == ZOMBIE ) {
                laneEntities.positions[i].x = 
                  zombieX - 1;
              } else {
                laneEntities.states[i] = EATING;
              }
              break;
            }

            if ( zombieX - NORMAL_ZOMBIE_SPEED < 0 ) {
              RemoveEntity( i, 0 );
            } else {
              laneEntities.spaces[zombieX] = -1;
              laneEntities.positions[i].x -= NORMAL_ZOMBIE_SPEED;
              laneEntities.spaces[zombieX - NORMAL_ZOMBIE_SPEED] = i;
              laneEntities.actionDelays[i] = msecs;
            }
          }
        }
        break;

        default:
          break;
      }

    }

    case EATING: {
      /*laneEntities.health[
        laneEntities.spaces[
            laneEntities.positions[i].x - 1
          ]
        ] = 0;*/
      laneEntities.states[i] = WALKING;

      break;
    }

    default:
      break;

  }
}
