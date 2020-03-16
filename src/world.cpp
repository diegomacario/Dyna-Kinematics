#include "world.h"

World::World(std::vector<Wall>&&             walls,
             const std::vector<RigidBody2D>& rigidBodies)
   : mWalls(std::move(walls))
   , mRigidBodies(rigidBodies)
{

}
