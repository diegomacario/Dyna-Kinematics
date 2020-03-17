#include "world.h"

World::World(std::vector<Wall>&&             walls,
             const std::vector<RigidBody2D>& rigidBodies)
   : mWalls(std::move(walls))
   , mRigidBodies(rigidBodies)
{

}

void World::render(const Renderer2D& renderer2D)
{
   for (std::vector<Wall>::iterator iter = mWalls.begin(); iter < mWalls.end(); ++iter)
   {
      renderer2D.renderLine(*iter);
   }

   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter < mRigidBodies.end(); ++iter)
   {
      renderer2D.renderRigidBody(*iter);
   }
}
