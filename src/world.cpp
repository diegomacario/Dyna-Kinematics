#include "world.h"

World::World(const std::vector<Wall>&        walls,
             const std::vector<RigidBody2D>& rigidBodies)
   : mWalls(walls)
   , mRigidBodies(rigidBodies)
{

}
