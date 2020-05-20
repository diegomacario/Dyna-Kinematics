#ifndef WORLD_H
#define WORLD_H

#include <vector>

#include "wall.h"
#include "rigid_body_2D.h"
#include "renderer_2D.h"

class World
{
public:

   World(std::vector<std::vector<Wall>>&&             wallScenes,
         const std::vector<std::vector<RigidBody2D>>& rigidBodyScenes);

   int  simulate(float deltaTime);
   void render(const Renderer2D& renderer2D, bool wireframe);

   void changeScene(int index);
   void resetScene();
   void setGravityState(int state);
   void setCoefficientOfRestitution(float coefficientOfRestitution);

private:

   enum class CollisionState : unsigned int
   {
      penetrating = 0,
      colliding   = 1,
      clear       = 2,
   };

   struct BodyWallCollision
   {
      BodyWallCollision();

      BodyWallCollision(const glm::vec2& collisionNormal,
                        int              collidingBodyIndex,
                        int              collidingVertexIndex,
                        int              collidingWallIndex);

      glm::vec2 collisionNormal;
      int       collidingBodyIndex;
      int       collidingVertexIndex;
      int       collidingWallIndex;
   };

   struct VertexVertexCollision
   {
      VertexVertexCollision();

      VertexVertexCollision(const glm::vec2& collisionNormal,
                            int              collidingBodyAIndex,
                            int              collidingBodyBIndex,
                            int              collidingVertexAIndex,
                            int              collidingVertexBIndex);

      glm::vec2 collisionNormal;
      int       collidingBodyAIndex;
      int       collidingBodyBIndex;
      int       collidingVertexAIndex;
      int       collidingVertexBIndex;
   };

   struct VertexEdgeCollision
   {
      VertexEdgeCollision();

      VertexEdgeCollision(const glm::vec2& collisionNormal,
                          int              collidingBodyAIndex,
                          int              collidingBodyBIndex,
                          int              collidingVertexAIndex,
                          const glm::vec2& collidingBodyBPoint);

      glm::vec2 collisionNormal;
      int       collidingBodyAIndex;
      int       collidingBodyBIndex;
      int       collidingVertexAIndex;
      glm::vec2 collidingBodyBPoint;
   };

   void                                           computeForces();

   void                                           integrate(float deltaTime);

   CollisionState                                 checkForBodyWallPenetration();
   CollisionState                                 checkForBodyWallCollision();
   int                                            resolveAllBodyWallCollisions();
   std::tuple<glm::vec2, float>                   resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision);
   std::tuple<glm::vec2, float>                   resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision,
                                                                           const glm::vec2&         linearVelocity,
                                                                           float                    angularVelocity);
   bool                                           isBodyWallCollisionResolved(const BodyWallCollision& bodyWallCollision,
                                                                              const glm::vec2&         linearVelocity,
                                                                              float                    angularVelocity);

   CollisionState                                 checkForBodyBodyPenetration();
   CollisionState                                 checkForVertexVertexCollision();
   CollisionState                                 checkForVertexEdgeCollision();
   int                                            resolveAllBodyBodyCollisions();
   std::tuple<glm::vec2, float, glm::vec2, float> resolveVertexVertexCollision(const VertexVertexCollision& vertexVertexCollision);
   std::tuple<glm::vec2, float, glm::vec2, float> resolveVertexVertexCollision(const VertexVertexCollision& vertexVertexCollision,
                                                                               const glm::vec2&             bodyALinearVelocity,
                                                                               float                        bodyAAngularVelocity,
                                                                               const glm::vec2&             bodyBLinearVelocity,
                                                                               float                        bodyBAngularVelocity);
   bool                                           isVertexVertexCollisionResolved(const VertexVertexCollision& vertexVertexCollision,
                                                                                  const glm::vec2&             bodyALinearVelocity,
                                                                                  float                        bodyAAngularVelocity,
                                                                                  const glm::vec2&             bodyBLinearVelocity,
                                                                                  float                        bodyBAngularVelocity);
   std::tuple<glm::vec2, float, glm::vec2, float> resolveVertexEdgeCollision(const VertexEdgeCollision& vertexEdgeCollision);
   std::tuple<glm::vec2, float, glm::vec2, float> resolveVertexEdgeCollision(const VertexEdgeCollision& vertexEdgeCollision,
                                                                             const glm::vec2&           bodyALinearVelocity,
                                                                             float                      bodyAAngularVelocity,
                                                                             const glm::vec2&           bodyBLinearVelocity,
                                                                             float                      bodyBAngularVelocity);
   bool                                           isVertexEdgeCollisionResolved(const VertexEdgeCollision& vertexEdgeCollision,
                                                                                const glm::vec2&           bodyALinearVelocity,
                                                                                float                      bodyAAngularVelocity,
                                                                                const glm::vec2&           bodyBLinearVelocity,
                                                                                float                      bodyBAngularVelocity);

   std::vector<std::vector<Wall>>                  mWallScenes;
   std::vector<Wall>*                              mWalls;

   std::vector<std::vector<RigidBody2D>>           mRigidBodyScenes;
   std::vector<RigidBody2D>                        mRigidBodies;

   std::vector<std::vector<BodyWallCollision>>     mBodyWallCollisions;
   std::vector<std::vector<VertexVertexCollision>> mVertexVertexCollisions;
   std::vector<std::vector<VertexEdgeCollision>>   mVertexEdgeCollisions;

   bool                                            mChangeScene;
   int                                             mSceneIndex;
   int                                             mGravityState;

   float                                           mCoefficientOfRestitution;
};

#endif
