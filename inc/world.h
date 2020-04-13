#ifndef WORLD_H
#define WORLD_H

#include <vector>

#include "wall.h"
#include "rigid_body_2D.h"
#include "renderer_2D.h"

class World
{
public:

   World(std::vector<Wall>&&             walls,
         const std::vector<RigidBody2D>& rigidBodies);

   void           simulate(float deltaTime);
   void           render(const Renderer2D& renderer2D);

private:

   enum class CollisionState : unsigned int
   {
      penetrating = 0,
      colliding   = 1,
      clear       = 2,
   };

   void           computeForces();

   void           integrate(float deltaTime);

   CollisionState checkForBodyWallCollision();
   void           resolveBodyWallCollision();

   CollisionState checkForBodyBodyCollision();
   void           resolveBodyBodyCollision();

   std::vector<Wall>        mWalls;
   std::vector<RigidBody2D> mRigidBodies;

   struct BodyWallCollision
   {
      BodyWallCollision();

      glm::vec2             collisionNormal;
      int                   collidingBodyIndex;
      int                   collidingVertexIndex;
   };

   struct BodyBodyCollision
   {
      BodyBodyCollision();

      glm::vec2             collisionNormal;
      int                   collidingBodyAIndex;
      int                   collidingBodyBIndex;
      int                   collidingVertexAIndex;
      int                   collidingVertexBIndex;
   };

   BodyWallCollision        mBodyWallCollision;
   BodyBodyCollision        mBodyBodyCollision;
};

#endif
