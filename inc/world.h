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
      penetrating           = 0,
      colliding             = 1,
      collidingVertexVertex = 2,
      collidingVertexEdge   = 3,
      clear                 = 4,
   };

   struct BodyWallCollision
   {
      BodyWallCollision();

      BodyWallCollision(glm::vec2 collisionNormal,
                        int       collidingBodyIndex,
                        int       collidingVertexIndex,
                        int       collidingWallIndex);

      glm::vec2                   collisionNormal;
      int                         collidingBodyIndex;
      int                         collidingVertexIndex;
      int                         collidingWallIndex;
   };

   struct VertexVertexCollision
   {
      VertexVertexCollision();

      glm::vec2                   collisionNormal;
      int                         collidingBodyAIndex;
      int                         collidingBodyBIndex;
      int                         collidingVertexAIndex;
      int                         collidingVertexBIndex;
   };

   struct VertexEdgeCollision
   {
      VertexEdgeCollision();

      glm::vec2                   collisionNormal;
      int                         collidingBodyAIndex;
      int                         collidingBodyBIndex;
      int                         collidingVertexAIndex;
      glm::vec2                   collidingBodyBPoint;
   };

   void                         computeForces();

   void                         integrate(float deltaTime);

   CollisionState               checkForBodyWallPenetration();
   CollisionState               checkForBodyWallCollision();
   void                         resolveAllBodyWallCollisions();
   std::tuple<glm::vec2, float> resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision);
   std::tuple<glm::vec2, float> resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision,
                                                         const glm::vec2&         linearVelocity,
                                                         float                    angularVelocity);
   bool                         isBodyWallCollisionResolved(const BodyWallCollision& bodyWallCollision,
                                                            const glm::vec2&         linearVelocity,
                                                            float                    angularVelocity);

   CollisionState               checkForBodyBodyPenetration();
   CollisionState               checkForBodyBodyCollision();
   CollisionState               checkForVertexVertexCollision();
   CollisionState               checkForVertexEdgeCollision();
   void                         resolveBodyBodyCollision(CollisionState collisionState);
   void                         resolveVertexVertexCollision();
   void                         resolveVertexEdgeCollision();

   std::vector<Wall>                           mWalls;
   std::vector<RigidBody2D>                    mRigidBodies;

   std::vector<std::vector<BodyWallCollision>> mBodyWallCollisions;
   VertexVertexCollision                       mVertexVertexCollision;
   VertexEdgeCollision                         mVertexEdgeCollision;
};

#endif
