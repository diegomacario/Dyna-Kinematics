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

/*
void World::integrate(float deltaTime)
{
   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      RigidBody2D::KinematicAndDynamicState& currentState = iter->mStates[0];
      RigidBody2D::KinematicAndDynamicState& futureState  = iter->mStates[1];

      // Calculate new position and velocity using Euler's method

      // X_n+1 = X_n + (h * V_n)
      futureState.positionOfCenterOfMass = currentState.positionOfCenterOfMass + (currentState.velocityOfCenterOfMass * deltaTime);

      // F = (d/dt)P = M * A
      // A = F / M
      // V_n+1 = V_n + (h * A) = V_n + (h * (F / M))
      futureState.velocityOfCenterOfMass = currentState.velocityOfCenterOfMass + ((deltaTime * iter->mOneOverMass) * currentState.forceOfCenterOfMass);

      // Calculate new orientation and angular velocity using Euler's method

      // O_n+1 = O_n + (h * W_n)
      futureState.orientation = currentState.orientation + (currentState.angularVelocity * deltaTime);

      // T = (d/dt)L = I * Alpha
      // Alpha = T / I
      // W_n+1 = W_n + (h * Alpha) = W_n + (h * (T / I))
      futureState.angularVelocity = currentState.angularVelocity + ((deltaTime * iter->mOneOverMomentOfInertia) * currentState.torque);
   }
}
*/

void World::integrate(float deltaTime)
{
   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      RigidBody2D::KinematicAndDynamicState& currentState = iter->mStates[0];
      RigidBody2D::KinematicAndDynamicState& futureState  = iter->mStates[1];

      float midPointOfDeltaTime = deltaTime / 2.0f;

      // Calculate new position and velocity using the classical 4th order Runge-Kutta method

      // We want to solve this 2nd order ODE:
      // F = M * X^dotdot
      // We can solve it by writing it as a system of two 1rst order ODEs:
      // [X^dot] = [  V  ]
      // [V^dot]   [F / M]
      // Where we want to find the position (X) and the velocity (V)

      glm::vec2 k1Pos                     = currentState.velocityOfCenterOfMass;
      glm::vec2 k1Vel                     = (iter->mOneOverMass * currentState.forceOfCenterOfMass);

      glm::vec2 posAtMidPointOfDeltaTime1 = currentState.positionOfCenterOfMass + (k1Pos * midPointOfDeltaTime);
      glm::vec2 velAtMidPointOfDeltaTime1 = currentState.velocityOfCenterOfMass + (k1Vel * midPointOfDeltaTime);

      glm::vec2 k2Pos                     = velAtMidPointOfDeltaTime1;
      glm::vec2 k2Vel                     = (iter->mOneOverMass * currentState.forceOfCenterOfMass);

      glm::vec2 posAtMidPointOfDeltaTime2 = currentState.positionOfCenterOfMass + (k2Pos * midPointOfDeltaTime);
      glm::vec2 velAtMidPointOfDeltaTime2 = currentState.velocityOfCenterOfMass + (k2Vel * midPointOfDeltaTime);

      glm::vec2 k3Pos                     = velAtMidPointOfDeltaTime2;
      glm::vec2 k3Vel                     = (iter->mOneOverMass * currentState.forceOfCenterOfMass);

      glm::vec2 posAtDeltaTime            = currentState.positionOfCenterOfMass + (k3Pos * deltaTime);
      glm::vec2 velAtDeltaTime            = currentState.velocityOfCenterOfMass + (k3Vel * deltaTime);

      glm::vec2 k4Pos                     = velAtDeltaTime;
      glm::vec2 k4Vel                     = (iter->mOneOverMass * currentState.forceOfCenterOfMass);

      glm::vec2 weightedAverageOfPositionSlopes = ((k1Pos + (2.0f * k2Pos) + (2.0f * k3Pos) + k4Pos) / 6.0f);
      glm::vec2 weightedAverageOfVelocitySlopes = ((k1Vel + (2.0f * k2Vel) + (2.0f * k3Vel) + k4Vel) / 6.0f);

      // P_n+1 = P_n + (h * weightedAverageOfPositionSlopes)
      futureState.positionOfCenterOfMass = currentState.positionOfCenterOfMass + (weightedAverageOfPositionSlopes * deltaTime);

      // V_n+1 = V_n + (h * weightedAverageOfVelocitySlopes)
      futureState.velocityOfCenterOfMass = currentState.velocityOfCenterOfMass + (weightedAverageOfVelocitySlopes * deltaTime);

      // Calculate new orientation and angular velocity using the classical 4th order Runge-Kutta method

      // We want to solve this 2nd order ODE:
      // T = I * O^dotdot
      // We can solve it by writing it as a system of two 1rst order ODEs:
      // [O^dot] = [  W  ]
      // [W^dot]   [T / I]
      // Where we want to find the orientation (O) and the angular velocity (W)

      float k1Ori                     = currentState.angularVelocity;
      float k1Ang                     = (iter->mOneOverMomentOfInertia * currentState.torque);

      float oriAtMidPointOfDeltaTime1 = currentState.orientation + (k1Ori * midPointOfDeltaTime);
      float angAtMidPointOfDeltaTime1 = currentState.angularVelocity + (k1Ang * midPointOfDeltaTime);

      float k2Ori                     = angAtMidPointOfDeltaTime1;
      float k2Ang                     = (iter->mOneOverMomentOfInertia * currentState.torque);

      float oriAtMidPointOfDeltaTime2 = currentState.orientation + (k2Ori * midPointOfDeltaTime);
      float angAtMidPointOfDeltaTime2 = currentState.angularVelocity + (k2Ang * midPointOfDeltaTime);

      float k3Ori                     = angAtMidPointOfDeltaTime2;
      float k3Ang                     = (iter->mOneOverMomentOfInertia * currentState.torque);

      float oriAtDeltaTime            = currentState.orientation + (k3Ori * deltaTime);
      float angAtDeltaTime            = currentState.angularVelocity + (k3Ang * deltaTime);

      float k4Ori                     = angAtDeltaTime;
      float k4Ang                     = (iter->mOneOverMomentOfInertia * currentState.torque);

      float weightedAverageOfOrientationSlopes = ((k1Ori + (2.0f * k2Ori) + (2.0f * k3Ori) + k4Ori) / 6.0f);
      float weightedAverageOfAngularVelocitySlopes = ((k1Ang + (2.0f * k2Ang) + (2.0f * k3Ang) + k4Ang) / 6.0f);

      // O_n+1 = O_n + (h * W_n)
      futureState.orientation = currentState.orientation + (weightedAverageOfOrientationSlopes * deltaTime);

      // T = (d/dt)L = I * Alpha
      // Alpha = T / I
      // W_n+1 = W_n + (h * Alpha) = W_n + (h * (T / I))
      futureState.angularVelocity = currentState.angularVelocity + (weightedAverageOfAngularVelocitySlopes * deltaTime);
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
