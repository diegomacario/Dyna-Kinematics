#include "world.h"

#include <iostream>

World::World(std::vector<Wall>&&             walls,
             const std::vector<RigidBody2D>& rigidBodies)
   : mWalls(std::move(walls))
   , mRigidBodies(rigidBodies)
   , mBodyWallCollisions(mRigidBodies.size())
   , mVertexVertexCollision()
   , mVertexEdgeCollision()
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

      if ((checkForBodyWallPenetration() == CollisionState::penetrating) ||
          (checkForBodyBodyPenetration() == CollisionState::penetrating))
      {
         // We simulated too far, so subdivide time and try again
         targetTime = (currentTime + targetTime) / 2.0f;
         continue;
      }

      CollisionState bodyWallCollisionState = checkForBodyWallCollision();
      if (bodyWallCollisionState == CollisionState::colliding)
      {
         resolveAllBodyWallCollisions();
      }

      CollisionState bodyBodyCollisionState = checkForBodyBodyCollision();
      if ((bodyBodyCollisionState == CollisionState::collidingVertexVertex) ||
          (bodyBodyCollisionState == CollisionState::collidingVertexEdge))
      {
         int numTimesCollisionHasBeenResolved = 0;
         do
         {
            resolveBodyBodyCollision(bodyBodyCollisionState);
            numTimesCollisionHasBeenResolved++;
            bodyBodyCollisionState = checkForBodyBodyCollision();
         }
         while (((bodyBodyCollisionState == CollisionState::collidingVertexVertex) ||
                 (bodyBodyCollisionState == CollisionState::collidingVertexEdge)) && (numTimesCollisionHasBeenResolved < 100));

         assert(numTimesCollisionHasBeenResolved < 100);
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
      currentState.forceOfCenterOfMass = glm::vec2(0.0f, 0.0f);
      currentState.torque = 0.0f;
      //currentState.forceOfCenterOfMass = glm::vec2(0.0f, -100.0f);
      //currentState.torque = 5.0f;
      currentState.forceOfCenterOfMass = glm::vec2(0.0f, -1.0f) / iter->mOneOverMass;
      //currentState.torque = 0.1f;
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

World::CollisionState World::checkForBodyWallPenetration()
{
   float depthEpsilon = 1.0f;

   for (std::vector<RigidBody2D>::iterator bodyIter = mRigidBodies.begin(); bodyIter != mRigidBodies.end(); ++bodyIter)
   {
      RigidBody2D::KinematicAndDynamicState& futureState = bodyIter->mStates[1];

      for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
      {
         glm::vec2 vertexPos = futureState.vertices[vertexIndex];
         glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
         glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

         // Chasles' Theorem
         // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
         // and a simple rotation of the rest of the body around that point
         glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);

         for (std::vector<Wall>::iterator wallIter = mWalls.begin(); wallIter != mWalls.end(); ++wallIter)
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
               return CollisionState::penetrating;
            }
         }
      }
   }

   return CollisionState::clear;
}

World::CollisionState World::checkForBodyWallCollision()
{
   World::CollisionState collisionState = CollisionState::clear;

   float depthEpsilon = 1.0f;

   for (std::vector<RigidBody2D>::iterator bodyIter = mRigidBodies.begin(); bodyIter != mRigidBodies.end(); ++bodyIter)
   {
      RigidBody2D::KinematicAndDynamicState& futureState = bodyIter->mStates[1];

      for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
      {
         glm::vec2 vertexPos = futureState.vertices[vertexIndex];
         glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
         glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

         // Chasles' Theorem
         // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
         // and a simple rotation of the rest of the body around that point
         glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);

         for (std::vector<Wall>::iterator wallIter = mWalls.begin(); wallIter != mWalls.end(); ++wallIter)
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

            if (distanceFromVertexToClosestPointOnWall < depthEpsilon)
            {
               // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
               // Because the wall is not moving, the relative velocity is the velocity of the vertex
               float relativeNormalVelocity = glm::dot(vertexVelocity, wallIter->getNormal());

               // If the relative normal velocity is negative, we have a collision
               if (relativeNormalVelocity < 0.0f)
               {
                  int collidingBodyIndex = static_cast<int>(bodyIter - mRigidBodies.begin());
                  mBodyWallCollisions[collidingBodyIndex].emplace_back(wallIter->getNormal(),                        // Collision normal
                                                                       collidingBodyIndex,                           // Colliding body index
                                                                       vertexIndex,                                  // Colliding vertex index
                                                                       static_cast<int>(wallIter - mWalls.begin())); // Colliding wall index

                  collisionState = CollisionState::colliding;
               }
            }
         }
      }
   }

   return collisionState;
}

void World::resolveAllBodyWallCollisions()
{
   std::vector<std::vector<BodyWallCollision>>::iterator bodyIter;
   std::vector<BodyWallCollision>::iterator              bodyWallCollisionIter;

   // Loop over all the bodies
   for (bodyIter = mBodyWallCollisions.begin(); bodyIter != mBodyWallCollisions.end(); ++bodyIter)
   {
      // If the current body hasn't collided with any walls then we skip it
      if (bodyIter->size() == 0)
      {
         continue;
      }

      std::vector<glm::vec2> linearVelocities;
      std::vector<float>     angularVelocities;
      std::vector<glm::vec2> collisionNormals;

      // Loop over all the body-wall collisions of the current body
      for (bodyWallCollisionIter = bodyIter->begin(); bodyWallCollisionIter != bodyIter->end(); ++bodyWallCollisionIter)
      {
         std::tuple<glm::vec2, float> velocities = resolveBodyWallCollision(*bodyWallCollisionIter);
         glm::vec2 linearVelocityOfCurrentCollision  = std::get<0>(velocities);
         float     angularVelocityOfCurrentCollision = std::get<1>(velocities);

         int numTimesCollisionHasBeenResolved = 1; // Equal to 1 because resolveBodyWallCollision() has already been called once
         while (!isBodyWallCollisionResolved(*bodyWallCollisionIter, linearVelocityOfCurrentCollision, angularVelocityOfCurrentCollision) && (numTimesCollisionHasBeenResolved < 100))
         {
            std::tuple<glm::vec2, float> velocities = resolveBodyWallCollision(*bodyWallCollisionIter, linearVelocityOfCurrentCollision, angularVelocityOfCurrentCollision);
            linearVelocityOfCurrentCollision  = std::get<0>(velocities);
            angularVelocityOfCurrentCollision = std::get<1>(velocities);
            numTimesCollisionHasBeenResolved++;
         }

         assert(numTimesCollisionHasBeenResolved < 100);

         // Store the linear and angular velocities of the body after the collision has been resolved
         // Also store the collision normal
         linearVelocities.push_back(linearVelocityOfCurrentCollision);
         angularVelocities.push_back(angularVelocityOfCurrentCollision);
         collisionNormals.push_back((*bodyWallCollisionIter).collisionNormal);
      }

      int collidingBodyIndex   = static_cast<int>(bodyIter - mBodyWallCollisions.begin());
      RigidBody2D& currentBody = mRigidBodies[collidingBodyIndex];

      // Compute the new direction of the body and the linear kinetic energy
      glm::vec2 linearVelocityDirection = glm::vec2(0.0f);
      float     avgLinearKineticEnergy  = 0.0f;
      for (std::vector<glm::vec2>::iterator linearVelocityIter = linearVelocities.begin(); linearVelocityIter != linearVelocities.end(); ++linearVelocityIter)
      {
         linearVelocityDirection += glm::normalize(*linearVelocityIter); // TODO: Should I normalize here?
         avgLinearKineticEnergy  += ((1 / 2.0f) * (1 / currentBody.mOneOverMass) * (glm::length(*linearVelocityIter) * glm::length(*linearVelocityIter)));
      }
      linearVelocityDirection = glm::normalize(linearVelocityDirection);
      avgLinearKineticEnergy /= linearVelocities.size();

      // If more than one point of collision, disregard the linearVelocityDirection calculated above and instead reflect the body about the average collision normal
      if (collisionNormals.size() > 1)
      {
         glm::vec2 avgCollisionNormal = glm::vec2(0.0f);
         for (std::vector<glm::vec2>::iterator collisionNormalIter = collisionNormals.begin(); collisionNormalIter != collisionNormals.end(); ++collisionNormalIter)
         {
            avgCollisionNormal += *collisionNormalIter; // TODO: Should I normalize here?
         }
         avgCollisionNormal = glm::normalize(avgCollisionNormal);

         linearVelocityDirection = glm::normalize(glm::reflect(currentBody.mStates[1].velocityOfCenterOfMass, avgCollisionNormal));
      }

      // Compute the new direction of rotation of the body and the angular kinetic energy
      float totalAngularVelocity       = 0.0f;
      float absTotalAngularVelocity    = 0.0f;
      float avgAngularKineticEnergy    = 0.0f;
      float absAvgAngularKineticEnergy = 0.0f;
      for (std::vector<float>::iterator angularVelocityIter = angularVelocities.begin(); angularVelocityIter != angularVelocities.end(); ++angularVelocityIter)
      {
         totalAngularVelocity    += *angularVelocityIter;
         absTotalAngularVelocity += abs(*angularVelocityIter);

         float angularKineticEnergy = ((1 / 2.0f) * (1 / currentBody.mOneOverMomentOfInertia) * ((*angularVelocityIter) * (*angularVelocityIter)));
         if (*angularVelocityIter >= 0.0f)
         {
            avgAngularKineticEnergy += angularKineticEnergy;
         }
         else
         {
            avgAngularKineticEnergy -= angularKineticEnergy;
         }
         absAvgAngularKineticEnergy += angularKineticEnergy;
      }
      avgAngularKineticEnergy    /= angularVelocities.size();
      absAvgAngularKineticEnergy /= angularVelocities.size();

      bool ccwiseRotation = false;
      if (totalAngularVelocity >= 0.0f)
      {
         ccwiseRotation = true;
      }

      // This check prevents a body from rotating because of small precision errors
      // TODO: Use a constant for the threshold
      if ((currentBody.mStates[1].angularVelocity == 0.0f) && (abs(avgAngularKineticEnergy) < 0.01f))
      {
         avgAngularKineticEnergy = 0.0f;
      }

      // Calculate the energy that has been lost because of collisions that cause the body to rotate in opposite directions
      float energyLostThroughCancellations = 0.0f;
      if (angularVelocities.size() > 1)
      {
         energyLostThroughCancellations = absAvgAngularKineticEnergy - abs(avgAngularKineticEnergy); // TODO: Is abs necessary here?
      }

      // Update the linear and angular velocities of the body
      currentBody.mStates[1].velocityOfCenterOfMass = sqrt(2 * (avgLinearKineticEnergy + energyLostThroughCancellations) * currentBody.mOneOverMass) * linearVelocityDirection;
      currentBody.mStates[1].angularVelocity        = sqrt(2 * abs(avgAngularKineticEnergy) * currentBody.mOneOverMomentOfInertia) * (ccwiseRotation ? 1.0f : -1.0f);

      // Since all the collisions of the current body have been resolved we can delete them
      mBodyWallCollisions[collidingBodyIndex].clear();
   }
}

std::tuple<glm::vec2, float> World::resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision)
{
   RigidBody2D& body = mRigidBodies[bodyWallCollision.collidingBodyIndex];
   RigidBody2D::KinematicAndDynamicState& futureState = body.mStates[1];

   glm::vec2 vertexPos = futureState.vertices[bodyWallCollision.collidingVertexIndex];
   glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
   glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

   // Chasles' Theorem
   // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
   // and a simple rotation of the rest of the body around that point
   glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);

   // The wall doesn't move and has an infinite mass, which simplifies the collision response equations

   // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
   // Because the wall is not moving, the relative velocity is the velocity of the vertex
   float relativeNormalVelocity = glm::dot(vertexVelocity, bodyWallCollision.collisionNormal);
   float impulseNumerator       = -(1.0f + body.mCoefficientOfRestitution) * relativeNormalVelocity;

   float CMToVertPerpDotColliNormal = glm::dot(CMToVertexPerpendicular, bodyWallCollision.collisionNormal);
   float impulseDenominator         = body.mOneOverMass + (body.mOneOverMomentOfInertia * CMToVertPerpDotColliNormal * CMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   glm::vec2 linearVelocityAfterCollision  = (futureState.velocityOfCenterOfMass + ((impulse * body.mOneOverMass) * bodyWallCollision.collisionNormal));
   float     angularVelocityAfterCollision = (futureState.angularVelocity        + ((impulse * body.mOneOverMomentOfInertia) * CMToVertPerpDotColliNormal));

   return std::make_tuple(linearVelocityAfterCollision,
                          angularVelocityAfterCollision);
}

std::tuple<glm::vec2, float> World::resolveBodyWallCollision(const BodyWallCollision& bodyWallCollision,
                                                             const glm::vec2&         linearVelocity,
                                                             float                    angularVelocity)
{
   RigidBody2D& body = mRigidBodies[bodyWallCollision.collidingBodyIndex];
   RigidBody2D::KinematicAndDynamicState& futureState = body.mStates[1];

   glm::vec2 vertexPos = futureState.vertices[bodyWallCollision.collidingVertexIndex];
   glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
   glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

   // Chasles' Theorem
   // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
   // and a simple rotation of the rest of the body around that point
   glm::vec2 vertexVelocity = linearVelocity + (angularVelocity * CMToVertexPerpendicular);

   // The wall doesn't move and has an infinite mass, which simplifies the collision response equations

   // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
   // Because the wall is not moving, the relative velocity is the velocity of the vertex
   float relativeNormalVelocity = glm::dot(vertexVelocity, bodyWallCollision.collisionNormal);
   float impulseNumerator       = -(1.0f + body.mCoefficientOfRestitution) * relativeNormalVelocity;

   float CMToVertPerpDotColliNormal = glm::dot(CMToVertexPerpendicular, bodyWallCollision.collisionNormal);
   float impulseDenominator         = body.mOneOverMass + (body.mOneOverMomentOfInertia * CMToVertPerpDotColliNormal * CMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   glm::vec2 linearVelocityAfterCollision  = (linearVelocity  + ((impulse * body.mOneOverMass) * bodyWallCollision.collisionNormal));
   float     angularVelocityAfterCollision = (angularVelocity + ((impulse * body.mOneOverMomentOfInertia) * CMToVertPerpDotColliNormal));

   return std::make_tuple(linearVelocityAfterCollision,
                          angularVelocityAfterCollision);
}

bool World::isBodyWallCollisionResolved(const BodyWallCollision& bodyWallCollision,
                                        const glm::vec2&         linearVelocity,
                                        float                    angVelocity)
{
   float depthEpsilon = 1.0f;

   RigidBody2D::KinematicAndDynamicState& futureState = mRigidBodies[bodyWallCollision.collidingBodyIndex].mStates[1];
   glm::vec2 velocityOfCenterOfMass = linearVelocity;
   float     angularVelocity        = angVelocity;

   glm::vec2 vertexPos = futureState.vertices[bodyWallCollision.collidingVertexIndex];
   glm::vec2 CMToVertex = vertexPos - futureState.positionOfCenterOfMass;
   glm::vec2 CMToVertexPerpendicular = glm::vec2(-CMToVertex.y, CMToVertex.x);

   // Chasles' Theorem
   // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
   // and a simple rotation of the rest of the body around that point
   glm::vec2 vertexVelocity = velocityOfCenterOfMass + (angularVelocity * CMToVertexPerpendicular);

   // Position of vertex = Pv
   // Any point on wall  = Po
   // Normal of wall     = N

   // We can use the projection of (Pv - Po) onto N to determine if we are penetrating the wall
   // That quantity is the distance between the vertex and its closest point on the wall

   // If it's negative, we are penetrating
   // If it's positive, we are not penetrating

   // dot((Pv - Po), N) = dot(Pv, N) - dot(Po, N) = dot(Pv, N) + C
   float distanceFromVertexToClosestPointOnWall = glm::dot(vertexPos, bodyWallCollision.collisionNormal) + mWalls[bodyWallCollision.collidingWallIndex].getC();

   if (distanceFromVertexToClosestPointOnWall < depthEpsilon)
   {
      // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
      // Because the wall is not moving, the relative velocity is the velocity of the vertex
      float relativeNormalVelocity = glm::dot(vertexVelocity, bodyWallCollision.collisionNormal);

      // If the relative normal velocity is negative, we have a collision
      if (relativeNormalVelocity < 0.0f)
      {
         return false;
      }
   }

   return true;
}

bool doesPointProjectOntoSegment(const glm::vec2& pointToTest, const glm::vec2& segmentStartPoint, const glm::vec2& segmentEndPoint)
{
   glm::vec2 segment = segmentEndPoint - segmentStartPoint;

   // Project pointToTest onto segment, computing the parameterized position d(t) = segmentStartPoint + distFromSegStartPointToProjectedPointToTest * (segmentEndPoint - segmentStartPoint)
   float distFromSegStartPointToProjectedPointToTest = glm::dot(pointToTest - segmentStartPoint, segment) / glm::dot(segment, segment);

   // If pointToTest projects outside of the segment, distFromSegStartPointToProjectedPointToTest is smaller than 0 or greater than 1
   if ((distFromSegStartPointToProjectedPointToTest < 0.0f) || (distFromSegStartPointToProjectedPointToTest > 1.0f))
   {
      return false;
   }

   return true;
}

glm::vec2 calculateClosestPointOnSegmentToPoint(const glm::vec2& pointToTest, const glm::vec2& segmentStartPoint, const glm::vec2& segmentEndPoint)
{
   glm::vec2 segment = segmentEndPoint - segmentStartPoint;

   // Project pointToTest onto segment, computing the parameterized position d(t) = segmentStartPoint + distFromSegStartPointToProjectedPointToTest * (segmentEndPoint - segmentStartPoint)
   float distFromSegStartPointToProjectedPointToTest = glm::dot(pointToTest - segmentStartPoint, segment) / glm::dot(segment, segment);

   // If pointToTest projects outside of the segment, distFromSegStartPointToProjectedPointToTest is smaller than 0 or greater than 1
   // If this is the case, clamp distFromSegStartPointToProjectedPointToTest to the closest endpoint
   if (distFromSegStartPointToProjectedPointToTest < 0.0f)
   {
      distFromSegStartPointToProjectedPointToTest = 0.0f;
   }
   else if (distFromSegStartPointToProjectedPointToTest > 1.0f)
   {
      distFromSegStartPointToProjectedPointToTest = 1.0f;
   }

   glm::vec2 closestPointOnSegmentToPoint = segmentStartPoint + (distFromSegStartPointToProjectedPointToTest * segment);

   return closestPointOnSegmentToPoint;
}

World::CollisionState World::checkForBodyBodyPenetration()
{
   // Check for penetration
   for (std::vector<RigidBody2D>::iterator bodyIterA = mRigidBodies.begin(); bodyIterA != mRigidBodies.end(); ++bodyIterA)
   {
      for (std::vector<RigidBody2D>::iterator bodyIterB = mRigidBodies.begin(); bodyIterB != mRigidBodies.end(); ++bodyIterB)
      {
         if (bodyIterA == bodyIterB)
         {
            continue;
         }

         // Check if any of the vertices of body A is inside of body B
         // To do this we check if any of the vertices of body A is to the left of all the CCWISE edges of body B
         for (int bodyAVertexIndex = 0; bodyAVertexIndex < 4; ++bodyAVertexIndex)
         {
            bool penetrating = true;
            for (int bodyBVertexIndex = 0; bodyBVertexIndex < 4; ++bodyBVertexIndex)
            {
               // Calculate a CCWISE edge using adjacent vertices
               glm::vec2 bodyBEdge;
               if (bodyBVertexIndex == 3)
               {
                  bodyBEdge = bodyIterB->mStates[1].vertices[0] - bodyIterB->mStates[1].vertices[bodyBVertexIndex];
               }
               else
               {
                  bodyBEdge = bodyIterB->mStates[1].vertices[bodyBVertexIndex + 1] - bodyIterB->mStates[1].vertices[bodyBVertexIndex];
               }

               glm::vec2 bodyBEdgePerpendicular = glm::vec2(-bodyBEdge.y, bodyBEdge.x);
               glm::vec2 firstVertexOfEdgeToVertexBeingTested = bodyIterA->mStates[1].vertices[bodyAVertexIndex] - bodyIterB->mStates[1].vertices[bodyBVertexIndex];

               // If this dot product is smaller than zero, the vertex is to the right of the edge, which means that there is no penetration
               if (glm::dot(firstVertexOfEdgeToVertexBeingTested, bodyBEdgePerpendicular) < 0)
               {
                  penetrating = false;
                  break;
               }
            }

            if (penetrating)
            {
               return CollisionState::penetrating;
            }
         }
      }
   }

   return CollisionState::clear;
}

World::CollisionState World::checkForBodyBodyCollision()
{
   CollisionState collisionState;

   // Check for vertex-vertex collision
   collisionState = checkForVertexVertexCollision();

   // Check for vertex-edge collision
   if (collisionState == CollisionState::clear)
   {
      collisionState = checkForVertexEdgeCollision();
   }

   return collisionState;
}

World::CollisionState World::checkForVertexVertexCollision()
{
   for (std::vector<RigidBody2D>::iterator bodyIterA = mRigidBodies.begin(); bodyIterA != mRigidBodies.end(); ++bodyIterA)
   {
      for (std::vector<RigidBody2D>::iterator bodyIterB = mRigidBodies.begin(); bodyIterB != mRigidBodies.end(); ++bodyIterB)
      {
         if (bodyIterA == bodyIterB)
         {
            continue;
         }

         for (int bodyAVertexIndex = 0; bodyAVertexIndex < 4; ++bodyAVertexIndex)
         {
            for (int bodyBVertexIndex = 0; bodyBVertexIndex < 4; ++bodyBVertexIndex)
            {
               glm::vec2 bodyAVertex = bodyIterA->mStates[1].vertices[bodyAVertexIndex];
               glm::vec2 bodyBVertex = bodyIterB->mStates[1].vertices[bodyBVertexIndex];

               // If the distance between two vertices is smaller than 0.1f, then we check for a collison
               if (glm::length(bodyAVertex - bodyBVertex) < 0.1f) // TODO: Make threshold a constant
               {
                  // Calculate the velocity of the vertex on body A
                  glm::vec2 bodyACMToVertex              = bodyAVertex - bodyIterA->mStates[1].positionOfCenterOfMass;
                  glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
                  glm::vec2 bodyAVertexVelocity          = bodyIterA->mStates[1].velocityOfCenterOfMass + (bodyIterA->mStates[1].angularVelocity * bodyACMToVertexPerpendicular);

                  // Calculate the velocity of the vertex on body B
                  glm::vec2 bodyBCMToVertex              = bodyBVertex - bodyIterB->mStates[1].positionOfCenterOfMass;
                  glm::vec2 bodyBCMToVertexPerpendicular = glm::vec2(-bodyBCMToVertex.y, bodyBCMToVertex.x);
                  glm::vec2 bodyBVertexVelocity          = bodyIterB->mStates[1].velocityOfCenterOfMass + (bodyIterB->mStates[1].angularVelocity * bodyBCMToVertexPerpendicular);

                  // Calculate the relative velocity
                  glm::vec2 relativeVelocity = bodyAVertexVelocity - bodyBVertexVelocity;

                  // We assume the collision normal is the line that connects the CMs of the two bodies
                  glm::vec2 collisionNormal = glm::normalize(bodyIterA->mStates[1].positionOfCenterOfMass - bodyIterB->mStates[1].positionOfCenterOfMass);

                  // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
                  float relativeNormalVelocity = glm::dot(relativeVelocity, collisionNormal);

                  // If the relative normal velocity is negative, we have a collision
                  if (relativeNormalVelocity < 0.0f)
                  {
                     mVertexVertexCollision.collisionNormal       = collisionNormal;
                     mVertexVertexCollision.collidingBodyAIndex   = static_cast<int>(bodyIterA - mRigidBodies.begin());
                     mVertexVertexCollision.collidingBodyBIndex   = static_cast<int>(bodyIterB - mRigidBodies.begin());
                     mVertexVertexCollision.collidingVertexAIndex = bodyAVertexIndex;
                     mVertexVertexCollision.collidingVertexBIndex = bodyBVertexIndex;
                     return CollisionState::collidingVertexVertex;
                  }
               }
            }
         }
      }
   }

   return CollisionState::clear;
}

World::CollisionState World::checkForVertexEdgeCollision()
{
   for (std::vector<RigidBody2D>::iterator bodyIterA = mRigidBodies.begin(); bodyIterA != mRigidBodies.end(); ++bodyIterA)
   {
      for (std::vector<RigidBody2D>::iterator bodyIterB = mRigidBodies.begin(); bodyIterB != mRigidBodies.end(); ++bodyIterB)
      {
         if (bodyIterA == bodyIterB)
         {
            continue;
         }

         for (int bodyAVertexIndex = 0; bodyAVertexIndex < 4; ++bodyAVertexIndex)
         {
            for (int bodyBVertexIndex = 0; bodyBVertexIndex < 4; ++bodyBVertexIndex)
            {
               glm::vec2 bodyAVertex = bodyIterA->mStates[1].vertices[bodyAVertexIndex];

               // Calculate a CCWISE edge using adjacent vertices
               glm::vec2 startPointOfBodyBEdge;
               glm::vec2 endPointOfBodyBEdge;
               glm::vec2 bodyBEdge;
               if (bodyBVertexIndex == 3)
               {
                  startPointOfBodyBEdge = bodyIterB->mStates[1].vertices[bodyBVertexIndex];
                  endPointOfBodyBEdge   = bodyIterB->mStates[1].vertices[0];
                  bodyBEdge             = endPointOfBodyBEdge - startPointOfBodyBEdge;
               }
               else
               {
                  startPointOfBodyBEdge = bodyIterB->mStates[1].vertices[bodyBVertexIndex];
                  endPointOfBodyBEdge   = bodyIterB->mStates[1].vertices[bodyBVertexIndex + 1];
                  bodyBEdge             = endPointOfBodyBEdge - startPointOfBodyBEdge;
               }

               // If bodyAVertex doesn't project onto bodyBEdge, then they cannot collide
               if (!doesPointProjectOntoSegment(bodyAVertex, startPointOfBodyBEdge, endPointOfBodyBEdge))
               {
                  continue;
               }

               glm::vec2 closestPointOnBodyBEdgeToBodyAVertex = calculateClosestPointOnSegmentToPoint(bodyAVertex, startPointOfBodyBEdge, endPointOfBodyBEdge);

               // If the distance between bodyAVertex and its closest point on bodyBEdge is smaller than 0.1f, then we check for a collison
               float distanceFromBodyAVertexToClosestPointOnBodyBEdge = glm::length(closestPointOnBodyBEdgeToBodyAVertex - bodyAVertex);
               if (distanceFromBodyAVertexToClosestPointOnBodyBEdge < 0.1f) // TODO: Make threshold a constant
               {
                  // Calculate the velocity of bodyAVertex
                  glm::vec2 bodyACMToVertex              = bodyAVertex - bodyIterA->mStates[1].positionOfCenterOfMass;
                  glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
                  glm::vec2 bodyAVertexVelocity          = bodyIterA->mStates[1].velocityOfCenterOfMass + (bodyIterA->mStates[1].angularVelocity * bodyACMToVertexPerpendicular);

                  // Calculate the velocity of the closest point on bodyBEdge to bodyAVertex
                  glm::vec2 bodyBCMToClosestPoint              = closestPointOnBodyBEdgeToBodyAVertex - bodyIterB->mStates[1].positionOfCenterOfMass;
                  glm::vec2 bodyBCMToClosestPointPerpendicular = glm::vec2(-bodyBCMToClosestPoint.y, bodyBCMToClosestPoint.x);
                  glm::vec2 bodyBClosestPointVelocity          = bodyIterB->mStates[1].velocityOfCenterOfMass + (bodyIterB->mStates[1].angularVelocity * bodyBCMToClosestPointPerpendicular);

                  // Calculate the relative velocity
                  glm::vec2 relativeVelocity = bodyAVertexVelocity - bodyBClosestPointVelocity;

                  // The collision normal is the normal of bodyBEdge
                  // We can calculate it by normalizing the vector that goes from closestPointOnBodyBEdgeToBodyAVertex to bodyAVertex
                  glm::vec2 collisionNormal = glm::normalize(bodyAVertex - closestPointOnBodyBEdgeToBodyAVertex);

                  // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
                  float relativeNormalVelocity = glm::dot(relativeVelocity, collisionNormal);

                  // If the relative normal velocity is negative, we have a collision
                  if (relativeNormalVelocity < 0.0f)
                  {
                     mVertexEdgeCollision.collisionNormal       = collisionNormal;
                     mVertexEdgeCollision.collidingBodyAIndex   = static_cast<int>(bodyIterA - mRigidBodies.begin());
                     mVertexEdgeCollision.collidingBodyBIndex   = static_cast<int>(bodyIterB - mRigidBodies.begin());
                     mVertexEdgeCollision.collidingVertexAIndex = bodyAVertexIndex;
                     mVertexEdgeCollision.collidingBodyBPoint   = closestPointOnBodyBEdgeToBodyAVertex;
                     return CollisionState::collidingVertexEdge;
                  }
               }
            }
         }
      }
   }

   return CollisionState::clear;
}

void World::resolveBodyBodyCollision(CollisionState collisionState)
{
   if (collisionState == CollisionState::collidingVertexVertex)
   {
      resolveVertexVertexCollision();
   }
   else if (collisionState == CollisionState::collidingVertexEdge)
   {
      resolveVertexEdgeCollision();
   }
}

void World::resolveVertexVertexCollision()
{
   RigidBody2D& bodyA = mRigidBodies[mVertexVertexCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[mVertexVertexCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[mVertexVertexCollision.collidingVertexAIndex];
   glm::vec2 bodyBVertex = bodyB.mStates[1].vertices[mVertexVertexCollision.collidingVertexBIndex];

   // Calculate the velocity of the vertex on body A
   glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
   glm::vec2 bodyAVertexVelocity          = bodyA.mStates[1].velocityOfCenterOfMass + (bodyA.mStates[1].angularVelocity * bodyACMToVertexPerpendicular);

   // Calculate the velocity of the vertex on body B
   glm::vec2 bodyBCMToVertex              = bodyBVertex - bodyB.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyBCMToVertexPerpendicular = glm::vec2(-bodyBCMToVertex.y, bodyBCMToVertex.x);
   glm::vec2 bodyBVertexVelocity          = bodyB.mStates[1].velocityOfCenterOfMass + (bodyB.mStates[1].angularVelocity * bodyBCMToVertexPerpendicular);

   // Calculate the relative normal velocity
   glm::vec2 relativeVelocity   = bodyAVertexVelocity - bodyBVertexVelocity;
   float relativeNormalVelocity = glm::dot(relativeVelocity, mVertexVertexCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + bodyA.mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal = glm::dot(bodyACMToVertexPerpendicular, mVertexVertexCollision.collisionNormal);
   float bodyBCMToVertPerpDotColliNormal = glm::dot(bodyBCMToVertexPerpendicular, mVertexVertexCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToVertPerpDotColliNormal * bodyBCMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * mVertexVertexCollision.collisionNormal);
   bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * mVertexVertexCollision.collisionNormal);
   bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToVertPerpDotColliNormal);
}

void World::resolveVertexEdgeCollision()
{
   RigidBody2D& bodyA = mRigidBodies[mVertexEdgeCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[mVertexEdgeCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[mVertexEdgeCollision.collidingVertexAIndex];
   glm::vec2 bodyBPoint  = mVertexEdgeCollision.collidingBodyBPoint;

   // Calculate the velocity of the vertex on body A
   glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
   glm::vec2 bodyAVertexVelocity          = bodyA.mStates[1].velocityOfCenterOfMass + (bodyA.mStates[1].angularVelocity * bodyACMToVertexPerpendicular);

   // Calculate the velocity of the vertex on body B
   glm::vec2 bodyBCMToPoint              = bodyBPoint - bodyB.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyBCMToPointPerpendicular = glm::vec2(-bodyBCMToPoint.y, bodyBCMToPoint.x);
   glm::vec2 bodyBPointVelocity          = bodyB.mStates[1].velocityOfCenterOfMass + (bodyB.mStates[1].angularVelocity * bodyBCMToPointPerpendicular);

   // Calculate the relative normal velocity
   glm::vec2 relativeVelocity   = bodyAVertexVelocity - bodyBPointVelocity;
   float relativeNormalVelocity = glm::dot(relativeVelocity, mVertexEdgeCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + bodyA.mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal  = glm::dot(bodyACMToVertexPerpendicular, mVertexEdgeCollision.collisionNormal);
   float bodyBCMToPointPerpDotColliNormal = glm::dot(bodyBCMToPointPerpendicular, mVertexEdgeCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToPointPerpDotColliNormal * bodyBCMToPointPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * mVertexEdgeCollision.collisionNormal);
   bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * mVertexEdgeCollision.collisionNormal);
   bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToPointPerpDotColliNormal);
}

World::BodyWallCollision::BodyWallCollision()
   : collisionNormal(glm::vec2(0.0f))
   , collidingBodyIndex(0)
   , collidingVertexIndex(0)
   , collidingWallIndex(0)
{

}

World::BodyWallCollision::BodyWallCollision(glm::vec2 collisionNormal,
                                            int       collidingBodyIndex,
                                            int       collidingVertexIndex,
                                            int       collidingWallIndex)
   : collisionNormal(collisionNormal)
   , collidingBodyIndex(collidingBodyIndex)
   , collidingVertexIndex(collidingVertexIndex)
   , collidingWallIndex(collidingWallIndex)
{

}

World::VertexVertexCollision::VertexVertexCollision()
   : collisionNormal(glm::vec2(0.0f))
   , collidingBodyAIndex(0)
   , collidingBodyBIndex(0)
   , collidingVertexAIndex(0)
   , collidingVertexBIndex(0)
{

}

World::VertexEdgeCollision::VertexEdgeCollision()
   : collisionNormal(glm::vec2(0.0f))
   , collidingBodyAIndex(0)
   , collidingBodyBIndex(0)
   , collidingVertexAIndex(0)
   , collidingBodyBPoint(glm::vec2(0.0f))
{

}
