#include "world.h"

World::World(std::vector<Wall>&&             walls,
             const std::vector<RigidBody2D>& rigidBodies)
   : mWalls(std::move(walls))
   , mRigidBodies(rigidBodies)
   , mCollisionNormal(glm::vec2(0.0f))
   , mCollidingBodyIndex(0)
   , mCollidingVertexIndex(0)
{

}

void World::simulate(float deltaTime)
{
   float currentTime = 0.0f;
   float targetTime  = deltaTime;

   while (currentTime < deltaTime)
   {
      computeForces();

      integrate(targetTime - currentTime);

      // Calculate the vertices of each rigid body at the target time
      for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
      {
         iter->calculateVertices(future);
      }

      CollisionState collisionState = checkForCollisions();

      if (collisionState == CollisionState::penetrating)
      {
         // We simulated too far, so subdivide time and try again
         targetTime = (currentTime + targetTime) / 2.0f;
      }
      else
      {
         if (collisionState == CollisionState::colliding)
         {
            int counter = 0;
            do
            {
               resolveCollisions();
               counter++;
            }
            while ((checkForCollisions() == CollisionState::colliding) && (counter < 100));

            assert(counter < 100);
         }

         // We made a successful step, so swap configurations to save the data for the next step
         currentTime = targetTime;
         targetTime = deltaTime;

         for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
         {
            RigidBody2D::KinematicAndDynamicState tempState = iter->mStates[0];
            iter->mStates[0] = iter->mStates[1];
            iter->mStates[1] = tempState;
         }
      }
   }
}

void World::render(const Renderer2D& renderer2D)
{
   for (std::vector<Wall>::iterator iter = mWalls.begin(); iter != mWalls.end(); ++iter)
   {
      renderer2D.renderLine(*iter);
   }

   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      renderer2D.renderRigidBody(*iter);
   }
}

void World::computeForces()
{
   // Clear forces
   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      RigidBody2D::KinematicAndDynamicState& currentState = iter->mStates[0];
      currentState.forceOfCenterOfMass = glm::vec2(0.0f, -1.0f);
      currentState.torque = 5.0f;
   }
}

void World::integrate(float deltaTime)
{
   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      RigidBody2D::KinematicAndDynamicState& currentState = iter->mStates[0];
      RigidBody2D::KinematicAndDynamicState& futureState  = iter->mStates[1];

      // Calculate new position and orientation
      futureState.positionOfCenterOfMass = currentState.positionOfCenterOfMass + (currentState.velocityOfCenterOfMass * deltaTime);
      futureState.orientation = currentState.orientation + (currentState.angularVelocity * deltaTime);

      // Calculate new velocity and angular velocity

      // F = (d/dt)P = M * A
      // A = F / M
      // V_n+1 = V_n + (h * A) = V_n + (h * (F / M))
      futureState.velocityOfCenterOfMass = currentState.velocityOfCenterOfMass + ((deltaTime * iter->mOneOverMass) * currentState.forceOfCenterOfMass);

      // T = (d/dt)L = I * Alpha
      // Alpha = T / I
      // W_n+1 = W_n + (h * Alpha) = W_n + (h * (T / I))
      futureState.angularVelocity = currentState.angularVelocity + ((deltaTime * iter->mOneOverMomentOfInertia) * currentState.torque);
   }
}

World::CollisionState World::checkForCollisions()
{
   CollisionState collisionState = CollisionState::clear;
   float depthEpsilon = 1.0f;

   for (std::vector<RigidBody2D>::iterator bodyIter = mRigidBodies.begin(); ((bodyIter != mRigidBodies.end()) && (collisionState != CollisionState::penetrating)); ++bodyIter)
   {
      RigidBody2D::KinematicAndDynamicState& futureState = bodyIter->mStates[1];

      for (int vertexIndex = 0; ((vertexIndex < 4) && (collisionState != CollisionState::penetrating)); ++vertexIndex)
      {
         glm::vec2 vertexPos = futureState.vertices[vertexIndex];
         glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
         glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

         // Chasles' Theorem
         // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
         // and a simple rotation of the rest of the body around that point
         glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);

         for (std::vector<Wall>::iterator wallIter = mWalls.begin(); ((wallIter != mWalls.end()) && (collisionState != CollisionState::penetrating)); ++wallIter)
         {
            // Position of vertex = Pv
            // Any point on wall  = Po
            // Normal of wall     = N

            // We can use the projection of (Pv - Po) onto N to determine if we are penetrating the wall
            // That quantity is the distance between the vertex and its closest point on the wall

            // If it's negative, we are penetrating
            // If it's positive, we are not penetrating

            // dot((Pv - Po), N) = dot(Pv, N) - dot(Po, N) = dot(Pv, N) + C
            float distanceFromVertexToClosestPointOnWall = glm::dot(vertexPos, wallIter->getNormal()) + wallIter->getC();

            if (distanceFromVertexToClosestPointOnWall < -depthEpsilon)
            {
               collisionState = CollisionState::penetrating;
            }
            else if (distanceFromVertexToClosestPointOnWall < depthEpsilon)
            {
               // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
               // Because the wall is not moving, the relative velocity is the velocity of the vertex
               float relativeNormalVelocity = glm::dot(vertexVelocity, wallIter->getNormal());

               // If the relative normal velocity is negative, we have a collision
               if (relativeNormalVelocity < 0.0f)
               {
                  collisionState        = CollisionState::colliding;
                  mCollisionNormal      = wallIter->getNormal();
                  mCollidingBodyIndex   = static_cast<int>(bodyIter - mRigidBodies.begin());
                  mCollidingVertexIndex = vertexIndex;
               }
            }
         }
      }
   }

   return collisionState;
}

void World::resolveCollisions()
{
   RigidBody2D& body = mRigidBodies[mCollidingBodyIndex];
   RigidBody2D::KinematicAndDynamicState& futureState = body.mStates[1];

   glm::vec2 vertexPos = futureState.vertices[mCollidingVertexIndex];
   glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
   glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

   // Chasles' Theorem
   // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
   // and a simple rotation of the rest of the body around that point
   glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);

   // The wall doesn't move and has an infinite mass, which simplifies the collision response equations

   // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
   // Because the wall is not moving, the relative velocity is the velocity of the vertex
   float relativeNormalVelocity = glm::dot(vertexVelocity, mCollisionNormal);
   float impulseNumerator       = -(1.0f + body.mCoefficientOfRestitution) * relativeNormalVelocity;

   float CMToVertPerpDotColliNormal = glm::dot(CMToVertexPerpendicular, mCollisionNormal);
   float impulseDenominator         = body.mOneOverMass + (body.mOneOverMomentOfInertia * CMToVertPerpDotColliNormal * CMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   futureState.velocityOfCenterOfMass += ((impulse * body.mOneOverMass) * mCollisionNormal);
   futureState.angularVelocity        += ((impulse * body.mOneOverMomentOfInertia) * CMToVertPerpDotColliNormal);
}
