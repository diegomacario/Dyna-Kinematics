#include "game.h"
#include "world.h"

#include <iostream>

World::World(std::vector<std::vector<Wall>>&&             wallScenes,
             const std::vector<std::vector<RigidBody2D>>& rigidBodyScenes)
   : mWallScenes(std::move(wallScenes))
   , mWalls(&mWallScenes[0])
   , mRigidBodyScenes(rigidBodyScenes)
   , mRigidBodies(rigidBodyScenes[0])
   , mBodyWallCollisions(mRigidBodies.size())
   , mVertexVertexCollisions(mRigidBodies.size())
   , mVertexEdgeCollisions(mRigidBodies.size())
   , mChangeScene(false)
   , mSceneIndex(0)
   , mGravityState(0)
   , mCoefficientOfRestitution(1.0f)
{

}

int World::simulate(float deltaTime)
{
   float currentTime = 0.0f;
   float targetTime  = deltaTime;

   while (currentTime < deltaTime)
   {
      if ((targetTime - currentTime) < 1e-6) // TODO: Make threshold a constant
      {
         return 1; // Unresolvable penetration error
      }

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
         int errorCode = resolveAllBodyWallCollisions();
         if (errorCode != 0)
         {
            return errorCode;
         }
      }

      CollisionState vertexVertexCollisionState = checkForVertexVertexCollision();
      CollisionState vertexEdgeCollisionState   = checkForVertexEdgeCollision();
      if ((vertexVertexCollisionState == CollisionState::colliding) ||
          (vertexEdgeCollisionState   == CollisionState::colliding))
      {
         int errorCode = resolveAllBodyBodyCollisions();
         if (errorCode != 0)
         {
            return errorCode;
         }
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

   return 0; // No error
}

void World::render(const Renderer2D& renderer2D, bool wireframe)
{
   if (mChangeScene)
   {
      mWalls = &mWallScenes[mSceneIndex];
      mRigidBodies = mRigidBodyScenes[mSceneIndex];
      mBodyWallCollisions.resize(mRigidBodies.size());
      mVertexVertexCollisions.resize(mRigidBodies.size());
      mVertexEdgeCollisions.resize(mRigidBodies.size());
      mChangeScene = false;
   }

   for (std::vector<Wall>::iterator iter = mWalls->begin(); iter != mWalls->end(); ++iter)
   {
      renderer2D.renderLine(*iter);
   }

   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      renderer2D.renderRigidBody(*iter, wireframe);
   }
}

void World::changeScene(int index)
{
   mSceneIndex  = index;
   mChangeScene = true;
}

void World::resetScene()
{
   mChangeScene = true;
}

void World::setGravityState(int state)
{
   mGravityState = state;
}

void World::setCoefficientOfRestitution(float coefficientOfRestitution)
{
   mCoefficientOfRestitution = coefficientOfRestitution;
}

void World::computeForces()
{
   // Clear forces
   for (std::vector<RigidBody2D>::iterator iter = mRigidBodies.begin(); iter != mRigidBodies.end(); ++iter)
   {
      RigidBody2D::KinematicAndDynamicState& currentState = iter->mStates[0];

      currentState.torque = 0.0f;

      if (mGravityState == 0)
      {
         currentState.forceOfCenterOfMass = glm::vec2(0.0f, 0.0f);
      }
      else if (mGravityState == 1)
      {
         currentState.forceOfCenterOfMass = glm::vec2(0.0f, -10.0f) / iter->mOneOverMass;
      }
      else if (mGravityState == 2)
      {
         currentState.forceOfCenterOfMass = glm::vec2(0.0f, 10.0f) / iter->mOneOverMass;
      }
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

         for (std::vector<Wall>::iterator wallIter = mWalls->begin(); wallIter != mWalls->end(); ++wallIter)
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

            glm::vec2 closestPointOnWall = calculateClosestPointOnSegmentToPoint(vertexPos, wallIter->getStartPoint(), wallIter->getEndPoint());

            if ((distanceFromVertexToClosestPointOnWall < -depthEpsilon) &&
                (glm::length(vertexPos - closestPointOnWall) < depthEpsilon) &&
                doesPointProjectOntoSegment(vertexPos, wallIter->getStartPoint(), wallIter->getEndPoint()))
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

         for (std::vector<Wall>::iterator wallIter = mWalls->begin(); wallIter != mWalls->end(); ++wallIter)
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

            glm::vec2 closestPointOnWall = calculateClosestPointOnSegmentToPoint(vertexPos, wallIter->getStartPoint(), wallIter->getEndPoint());

            if ((distanceFromVertexToClosestPointOnWall < depthEpsilon) &&
                (glm::length(vertexPos - closestPointOnWall) < depthEpsilon) &&
                doesPointProjectOntoSegment(vertexPos, wallIter->getStartPoint(), wallIter->getEndPoint()))
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
                                                                       static_cast<int>(wallIter - mWalls->begin())); // Colliding wall index

                  collisionState = CollisionState::colliding;
               }
            }
         }
      }
   }

   return collisionState;
}

int World::resolveAllBodyWallCollisions()
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

         if (numTimesCollisionHasBeenResolved >= 100)
         {
            return 2; // Unresolvable body-wall collision error
         }

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

      // Since all the body-wall collisions of the current body have been resolved we can delete them
      mBodyWallCollisions[collidingBodyIndex].clear();
   }

   return 0; // No error
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
   float impulseNumerator       = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity;

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
   float impulseNumerator       = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity;

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
   float distanceFromVertexToClosestPointOnWall = glm::dot(vertexPos, bodyWallCollision.collisionNormal) + mWalls->at(bodyWallCollision.collidingWallIndex).getC();

   glm::vec2 closestPointOnWall = calculateClosestPointOnSegmentToPoint(vertexPos, mWalls->at(bodyWallCollision.collidingWallIndex).getStartPoint(), mWalls->at(bodyWallCollision.collidingWallIndex).getEndPoint());

   if ((distanceFromVertexToClosestPointOnWall < depthEpsilon) &&
       (glm::length(vertexPos - closestPointOnWall) < depthEpsilon) &&
       doesPointProjectOntoSegment(vertexPos, mWalls->at(bodyWallCollision.collidingWallIndex).getStartPoint(), mWalls->at(bodyWallCollision.collidingWallIndex).getEndPoint()))
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

World::CollisionState World::checkForVertexVertexCollision()
{
   CollisionState collisionState = CollisionState::clear;

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
                     int collidingBodyAIndex = static_cast<int>(bodyIterA - mRigidBodies.begin());
                     int collidingBodyBIndex = static_cast<int>(bodyIterB - mRigidBodies.begin());

                     // Only body A stores the collision
                     // In a future iteration body B will store it too
                     mVertexVertexCollisions[collidingBodyAIndex].emplace_back(collisionNormal,     // Collision normal
                                                                               collidingBodyAIndex, // Colliding body A index
                                                                               collidingBodyBIndex, // Colliding body B index
                                                                               bodyAVertexIndex,    // Colliding vertex A index
                                                                               bodyBVertexIndex);   // Colliding vertex B index

                     collisionState = CollisionState::colliding;
                  }
               }
            }
         }
      }
   }

   return collisionState;
}

World::CollisionState World::checkForVertexEdgeCollision()
{
   CollisionState collisionState = CollisionState::clear;

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
                     int collidingBodyAIndex = static_cast<int>(bodyIterA - mRigidBodies.begin());
                     int collidingBodyBIndex = static_cast<int>(bodyIterB - mRigidBodies.begin());

                     bool collisionAlreadyDetectedAsVertexVertexCollison = false;
                     for (std::vector<VertexVertexCollision>::iterator vertexVertexCollisionIter = mVertexVertexCollisions[collidingBodyAIndex].begin();
                          vertexVertexCollisionIter != mVertexVertexCollisions[collidingBodyAIndex].end();
                          ++vertexVertexCollisionIter)
                     {
                        // If the current vertex-edge collision has already been detected as a vertex-vertex collision, don't store the vertex-edge collision
                        if ((vertexVertexCollisionIter->collidingBodyBIndex   == collidingBodyBIndex) &&
                            (vertexVertexCollisionIter->collidingVertexAIndex == bodyAVertexIndex))
                        {
                           collisionAlreadyDetectedAsVertexVertexCollison = true;
                           break;
                        }
                     }

                     if (collisionAlreadyDetectedAsVertexVertexCollison)
                     {
                        continue;
                     }

                     // Both body A and body B store the collision because it will not be detected again in future iterations
                     mVertexEdgeCollisions[collidingBodyAIndex].emplace_back(collisionNormal,                       // Collision normal
                                                                             collidingBodyAIndex,                   // Colliding body A index
                                                                             collidingBodyBIndex,                   // Colliding body B index
                                                                             bodyAVertexIndex,                      // Colliding vertex A index
                                                                             closestPointOnBodyBEdgeToBodyAVertex); // Colliding body B point

                     //mVertexEdgeCollisions[collidingBodyBIndex].emplace_back(collisionNormal,                       // Collision normal
                     //                                                        collidingBodyAIndex,                   // Colliding body A index
                     //                                                        collidingBodyBIndex,                   // Colliding body B index
                     //                                                        bodyAVertexIndex,                      // Colliding vertex A index
                     //                                                        closestPointOnBodyBEdgeToBodyAVertex); // Colliding body B point

                     collisionState = CollisionState::colliding;
                  }
               }
            }
         }
      }
   }

   return collisionState;
}

int World::resolveAllBodyBodyCollisions()
{
   std::vector<RigidBody2D>::iterator           bodyIter;
   std::vector<VertexVertexCollision>::iterator vertexVertexCollisionIter;
   std::vector<VertexEdgeCollision>::iterator   vertexEdgeCollisionIter;

   std::vector<std::vector<glm::vec2>> linearVelocities(mRigidBodies.size());
   std::vector<std::vector<float>>     angularVelocities(mRigidBodies.size());
   std::vector<std::vector<glm::vec2>> collisionNormals(mRigidBodies.size());

   // Loop over all the bodies
   for (int bodyIndex = 0; bodyIndex < mRigidBodies.size(); ++bodyIndex)
   {
      // Loop over all the vertex-vertex collisions of the current body
      for (vertexVertexCollisionIter = mVertexVertexCollisions[bodyIndex].begin(); vertexVertexCollisionIter != mVertexVertexCollisions[bodyIndex].end(); ++vertexVertexCollisionIter)
      {
         if (mVertexVertexCollisions[bodyIndex].size() == 0)
         {
            continue; // TODO: Remove this if not necessary
         }

         std::tuple<glm::vec2, float, glm::vec2, float> velocities = resolveVertexVertexCollision(*vertexVertexCollisionIter);
         glm::vec2 bodyALinearVelocityOfCurrentCollision  = std::get<0>(velocities);
         float     bodyAAngularVelocityOfCurrentCollision = std::get<1>(velocities);
         glm::vec2 bodyBLinearVelocityOfCurrentCollision  = std::get<2>(velocities);
         float     bodyBAngularVelocityOfCurrentCollision = std::get<3>(velocities);

         int numTimesCollisionHasBeenResolved = 1; // Equal to 1 because resolveVertexVertexCollision() has already been called once
         while (!isVertexVertexCollisionResolved(*vertexVertexCollisionIter,
                                                 bodyALinearVelocityOfCurrentCollision,
                                                 bodyAAngularVelocityOfCurrentCollision,
                                                 bodyBLinearVelocityOfCurrentCollision,
                                                 bodyBAngularVelocityOfCurrentCollision) && (numTimesCollisionHasBeenResolved < 100))
         {
            std::tuple<glm::vec2, float, glm::vec2, float> velocities = resolveVertexVertexCollision(*vertexVertexCollisionIter,
                                                                                                     bodyALinearVelocityOfCurrentCollision,
                                                                                                     bodyAAngularVelocityOfCurrentCollision,
                                                                                                     bodyBLinearVelocityOfCurrentCollision,
                                                                                                     bodyBAngularVelocityOfCurrentCollision);
            bodyALinearVelocityOfCurrentCollision  = std::get<0>(velocities);
            bodyAAngularVelocityOfCurrentCollision = std::get<1>(velocities);
            bodyBLinearVelocityOfCurrentCollision  = std::get<2>(velocities);
            bodyBAngularVelocityOfCurrentCollision = std::get<3>(velocities);
            numTimesCollisionHasBeenResolved++;
         }

         if (numTimesCollisionHasBeenResolved >= 100)
         {
            return 3; // Unresolvable vertex-vertex collision error
         }

         // Store the linear and angular velocities of body A after the collision has been resolved
         // Also store the collision normal
         linearVelocities[bodyIndex].push_back(bodyALinearVelocityOfCurrentCollision);
         angularVelocities[bodyIndex].push_back(bodyAAngularVelocityOfCurrentCollision);
         collisionNormals[bodyIndex].push_back((*vertexVertexCollisionIter).collisionNormal);
      }

      // Loop over all the vertex-edge collisions of the current body
      for (vertexEdgeCollisionIter = mVertexEdgeCollisions[bodyIndex].begin(); vertexEdgeCollisionIter != mVertexEdgeCollisions[bodyIndex].end(); ++vertexEdgeCollisionIter)
      {
         if (mVertexEdgeCollisions[bodyIndex].size() == 0)
         {
            continue; // TODO: Remove this if not necessary
         }

         std::tuple<glm::vec2, float, glm::vec2, float> velocities = resolveVertexEdgeCollision(*vertexEdgeCollisionIter);
         glm::vec2 bodyALinearVelocityOfCurrentCollision  = std::get<0>(velocities);
         float     bodyAAngularVelocityOfCurrentCollision = std::get<1>(velocities);
         glm::vec2 bodyBLinearVelocityOfCurrentCollision  = std::get<2>(velocities);
         float     bodyBAngularVelocityOfCurrentCollision = std::get<3>(velocities);

         int numTimesCollisionHasBeenResolved = 1; // Equal to 1 because resolveVertexEdgeCollision() has already been called once
         while (!isVertexEdgeCollisionResolved(*vertexEdgeCollisionIter,
                                               bodyALinearVelocityOfCurrentCollision,
                                               bodyAAngularVelocityOfCurrentCollision,
                                               bodyBLinearVelocityOfCurrentCollision,
                                               bodyBAngularVelocityOfCurrentCollision) && (numTimesCollisionHasBeenResolved < 100))
         {
            std::tuple<glm::vec2, float, glm::vec2, float> velocities = resolveVertexEdgeCollision(*vertexEdgeCollisionIter,
                                                                                                   bodyALinearVelocityOfCurrentCollision,
                                                                                                   bodyAAngularVelocityOfCurrentCollision,
                                                                                                   bodyBLinearVelocityOfCurrentCollision,
                                                                                                   bodyBAngularVelocityOfCurrentCollision);
            bodyALinearVelocityOfCurrentCollision  = std::get<0>(velocities);
            bodyAAngularVelocityOfCurrentCollision = std::get<1>(velocities);
            bodyBLinearVelocityOfCurrentCollision  = std::get<2>(velocities);
            bodyBAngularVelocityOfCurrentCollision = std::get<3>(velocities);
            numTimesCollisionHasBeenResolved++;
         }

         if (numTimesCollisionHasBeenResolved >= 100)
         {
            return 4; // Unresolvable vertex-edge collision error
         }

         // Store the linear and angular velocities of body A after the collision has been resolved
         // Also store the collision normal
         linearVelocities[bodyIndex].push_back(bodyALinearVelocityOfCurrentCollision);
         angularVelocities[bodyIndex].push_back(bodyAAngularVelocityOfCurrentCollision);
         collisionNormals[bodyIndex].push_back((*vertexEdgeCollisionIter).collisionNormal);

         // Store the linear and angular velocities of body B after the collision has been resolved
         // Also store the collision normal
         linearVelocities[vertexEdgeCollisionIter->collidingBodyBIndex].push_back(bodyBLinearVelocityOfCurrentCollision);
         angularVelocities[vertexEdgeCollisionIter->collidingBodyBIndex].push_back(bodyBAngularVelocityOfCurrentCollision);
         collisionNormals[vertexEdgeCollisionIter->collidingBodyBIndex].push_back((*vertexEdgeCollisionIter).collisionNormal);
      }

      // Since all the body-body collisions of the current body have been resolved we can delete them
      mVertexVertexCollisions[bodyIndex].clear();
      mVertexEdgeCollisions[bodyIndex].clear();
   }

   // Loop over all the bodies
   for (int bodyIndex = 0; bodyIndex < mRigidBodies.size(); ++bodyIndex)
   {
      if (linearVelocities[bodyIndex].size() == 0 && angularVelocities[bodyIndex].size() == 0)
      {
         continue; // TODO: Remove this if not necessary
      }

      RigidBody2D& currentBody = mRigidBodies[bodyIndex];

      // Compute the new direction of the body and the linear kinetic energy
      glm::vec2 linearVelocityDirection = glm::vec2(0.0f);
      float     avgLinearKineticEnergy  = 0.0f;
      for (std::vector<glm::vec2>::iterator linearVelocityIter = linearVelocities[bodyIndex].begin(); linearVelocityIter != linearVelocities[bodyIndex].end(); ++linearVelocityIter)
      {
         linearVelocityDirection += glm::normalize(*linearVelocityIter); // TODO: Should I normalize here?
         avgLinearKineticEnergy  += ((1 / 2.0f) * (1 / currentBody.mOneOverMass) * (glm::length(*linearVelocityIter) * glm::length(*linearVelocityIter)));
      }

      if (glm::length(linearVelocityDirection) > 0.0001f) // TODO: Make threshold a constant
      {
         linearVelocityDirection = glm::normalize(linearVelocityDirection);
      }
      avgLinearKineticEnergy /= linearVelocities[bodyIndex].size();

      // If more than one point of collision, disregard the linearVelocityDirection calculated above and instead reflect the body about the average collision normal
      if (collisionNormals[bodyIndex].size() > 1)
      {
         glm::vec2 avgCollisionNormal = glm::vec2(0.0f);
         for (std::vector<glm::vec2>::iterator collisionNormalIter = collisionNormals[bodyIndex].begin(); collisionNormalIter != collisionNormals[bodyIndex].end(); ++collisionNormalIter)
         {
            avgCollisionNormal += *collisionNormalIter; // TODO: Should I normalize here?
         }
      
         if (glm::length(avgCollisionNormal) > 0.0001f) // TODO: Make threshold a constant
         {
            avgCollisionNormal      = glm::normalize(avgCollisionNormal);
            linearVelocityDirection = glm::normalize(glm::reflect(currentBody.mStates[1].velocityOfCenterOfMass, avgCollisionNormal));
         }
      }

      // Compute the new direction of rotation of the body and the angular kinetic energy
      float totalAngularVelocity       = 0.0f;
      float absTotalAngularVelocity    = 0.0f;
      float avgAngularKineticEnergy    = 0.0f;
      float absAvgAngularKineticEnergy = 0.0f;
      for (std::vector<float>::iterator angularVelocityIter = angularVelocities[bodyIndex].begin(); angularVelocityIter != angularVelocities[bodyIndex].end(); ++angularVelocityIter)
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
      avgAngularKineticEnergy    /= angularVelocities[bodyIndex].size();
      absAvgAngularKineticEnergy /= angularVelocities[bodyIndex].size();

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
      if (angularVelocities[bodyIndex].size() > 1)
      {
         energyLostThroughCancellations = absAvgAngularKineticEnergy - abs(avgAngularKineticEnergy); // TODO: Is abs necessary here?
      }

      // Update the linear and angular velocities of the body
      currentBody.mStates[1].velocityOfCenterOfMass = sqrt(2 * (avgLinearKineticEnergy + energyLostThroughCancellations) * currentBody.mOneOverMass) * linearVelocityDirection;
      currentBody.mStates[1].angularVelocity        = sqrt(2 * abs(avgAngularKineticEnergy) * currentBody.mOneOverMomentOfInertia) * (ccwiseRotation ? 1.0f : -1.0f);
   }

   return 0; // No error
}

std::tuple<glm::vec2, float, glm::vec2, float> World::resolveVertexVertexCollision(const VertexVertexCollision& vertexVertexCollision)
{
   RigidBody2D& bodyA = mRigidBodies[vertexVertexCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexVertexCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexVertexCollision.collidingVertexAIndex];
   glm::vec2 bodyBVertex = bodyB.mStates[1].vertices[vertexVertexCollision.collidingVertexBIndex];

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
   float relativeNormalVelocity = glm::dot(relativeVelocity, vertexVertexCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal = glm::dot(bodyACMToVertexPerpendicular, vertexVertexCollision.collisionNormal);
   float bodyBCMToVertPerpDotColliNormal = glm::dot(bodyBCMToVertexPerpendicular, vertexVertexCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToVertPerpDotColliNormal * bodyBCMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   //bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * vertexVertexCollision.collisionNormal);
   //bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   //bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * vertexVertexCollision.collisionNormal);
   //bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToVertPerpDotColliNormal);

   glm::vec2 bodyALinearVelocityAfterCollision  = (bodyA.mStates[1].velocityOfCenterOfMass + ((impulse * bodyA.mOneOverMass) * vertexVertexCollision.collisionNormal));
   float     bodyAAngularVelocityAfterCollision = (bodyA.mStates[1].angularVelocity        + ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal));

   glm::vec2 bodyBLinearVelocityAfterCollision  = (bodyB.mStates[1].velocityOfCenterOfMass + ((-impulse * bodyB.mOneOverMass) * vertexVertexCollision.collisionNormal));
   float     bodyBAngularVelocityAfterCollision = (bodyB.mStates[1].angularVelocity        + ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToVertPerpDotColliNormal));

   return std::make_tuple(bodyALinearVelocityAfterCollision,
                          bodyAAngularVelocityAfterCollision,
                          bodyBLinearVelocityAfterCollision,
                          bodyBAngularVelocityAfterCollision);
}

std::tuple<glm::vec2, float, glm::vec2, float> World::resolveVertexVertexCollision(const VertexVertexCollision& vertexVertexCollision,
                                                                                   const glm::vec2&             bodyALinearVelocity,
                                                                                   float                        bodyAAngularVelocity,
                                                                                   const glm::vec2&             bodyBLinearVelocity,
                                                                                   float                        bodyBAngularVelocity)
{
   RigidBody2D& bodyA = mRigidBodies[vertexVertexCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexVertexCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexVertexCollision.collidingVertexAIndex];
   glm::vec2 bodyBVertex = bodyB.mStates[1].vertices[vertexVertexCollision.collidingVertexBIndex];

   // Calculate the velocity of the vertex on body A
   glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
   glm::vec2 bodyAVertexVelocity          = bodyALinearVelocity + (bodyAAngularVelocity * bodyACMToVertexPerpendicular);

   // Calculate the velocity of the vertex on body B
   glm::vec2 bodyBCMToVertex              = bodyBVertex - bodyB.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyBCMToVertexPerpendicular = glm::vec2(-bodyBCMToVertex.y, bodyBCMToVertex.x);
   glm::vec2 bodyBVertexVelocity          = bodyBLinearVelocity + (bodyBAngularVelocity * bodyBCMToVertexPerpendicular);

   // Calculate the relative normal velocity
   glm::vec2 relativeVelocity   = bodyAVertexVelocity - bodyBVertexVelocity;
   float relativeNormalVelocity = glm::dot(relativeVelocity, vertexVertexCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal = glm::dot(bodyACMToVertexPerpendicular, vertexVertexCollision.collisionNormal);
   float bodyBCMToVertPerpDotColliNormal = glm::dot(bodyBCMToVertexPerpendicular, vertexVertexCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToVertPerpDotColliNormal * bodyBCMToVertPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   //bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * vertexVertexCollision.collisionNormal);
   //bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   //bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * vertexVertexCollision.collisionNormal);
   //bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToVertPerpDotColliNormal);

   glm::vec2 bodyALinearVelocityAfterCollision  = (bodyALinearVelocity  + ((impulse * bodyA.mOneOverMass) * vertexVertexCollision.collisionNormal));
   float     bodyAAngularVelocityAfterCollision = (bodyAAngularVelocity + ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal));

   glm::vec2 bodyBLinearVelocityAfterCollision  = (bodyBLinearVelocity  + ((-impulse * bodyB.mOneOverMass) * vertexVertexCollision.collisionNormal));
   float     bodyBAngularVelocityAfterCollision = (bodyBAngularVelocity + ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToVertPerpDotColliNormal));

   return std::make_tuple(bodyALinearVelocityAfterCollision,
                          bodyAAngularVelocityAfterCollision,
                          bodyBLinearVelocityAfterCollision,
                          bodyBAngularVelocityAfterCollision);
}

bool World::isVertexVertexCollisionResolved(const VertexVertexCollision& vertexVertexCollision,
                                            const glm::vec2&             bodyALinearVelocity,
                                            float                        bodyAAngularVelocity,
                                            const glm::vec2&             bodyBLinearVelocity,
                                            float                        bodyBAngularVelocity)
{
   RigidBody2D& bodyA = mRigidBodies[vertexVertexCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexVertexCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexVertexCollision.collidingVertexAIndex];
   glm::vec2 bodyBVertex = bodyB.mStates[1].vertices[vertexVertexCollision.collidingVertexBIndex];

   // If the distance between two vertices is smaller than 0.1f, then we check for a collison
   if (glm::length(bodyAVertex - bodyBVertex) < 0.1f) // TODO: Make threshold a constant
   {
      // Calculate the velocity of the vertex on body A
      glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
      glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
      glm::vec2 bodyAVertexVelocity          = bodyALinearVelocity + (bodyAAngularVelocity * bodyACMToVertexPerpendicular);

      // Calculate the velocity of the vertex on body B
      glm::vec2 bodyBCMToVertex              = bodyBVertex - bodyB.mStates[1].positionOfCenterOfMass;
      glm::vec2 bodyBCMToVertexPerpendicular = glm::vec2(-bodyBCMToVertex.y, bodyBCMToVertex.x);
      glm::vec2 bodyBVertexVelocity          = bodyBLinearVelocity + (bodyBAngularVelocity * bodyBCMToVertexPerpendicular);

      // Calculate the relative velocity
      glm::vec2 relativeVelocity = bodyAVertexVelocity - bodyBVertexVelocity;

      // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
      float relativeNormalVelocity = glm::dot(relativeVelocity, vertexVertexCollision.collisionNormal);

      // If the relative normal velocity is negative, we have a collision
      if (relativeNormalVelocity < 0.0f)
      {
         return false;
      }
   }

   return true;
}

std::tuple<glm::vec2, float, glm::vec2, float> World::resolveVertexEdgeCollision(const VertexEdgeCollision& vertexEdgeCollision)
{
   RigidBody2D& bodyA = mRigidBodies[vertexEdgeCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexEdgeCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexEdgeCollision.collidingVertexAIndex];
   glm::vec2 bodyBPoint  = vertexEdgeCollision.collidingBodyBPoint;

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
   float relativeNormalVelocity = glm::dot(relativeVelocity, vertexEdgeCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal  = glm::dot(bodyACMToVertexPerpendicular, vertexEdgeCollision.collisionNormal);
   float bodyBCMToPointPerpDotColliNormal = glm::dot(bodyBCMToPointPerpendicular, vertexEdgeCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToPointPerpDotColliNormal * bodyBCMToPointPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   //bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * vertexEdgeCollision.collisionNormal);
   //bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   //bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * vertexEdgeCollision.collisionNormal);
   //bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToPointPerpDotColliNormal);

   glm::vec2 bodyALinearVelocityAfterCollision  = (bodyA.mStates[1].velocityOfCenterOfMass + ((impulse * bodyA.mOneOverMass) * vertexEdgeCollision.collisionNormal));
   float     bodyAAngularVelocityAfterCollision = (bodyA.mStates[1].angularVelocity        + ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal));

   glm::vec2 bodyBLinearVelocityAfterCollision  = (bodyB.mStates[1].velocityOfCenterOfMass + ((-impulse * bodyB.mOneOverMass) * vertexEdgeCollision.collisionNormal));
   float     bodyBAngularVelocityAfterCollision = (bodyB.mStates[1].angularVelocity        + ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToPointPerpDotColliNormal));

   return std::make_tuple(bodyALinearVelocityAfterCollision,
                          bodyAAngularVelocityAfterCollision,
                          bodyBLinearVelocityAfterCollision,
                          bodyBAngularVelocityAfterCollision);
}

std::tuple<glm::vec2, float, glm::vec2, float> World::resolveVertexEdgeCollision(const VertexEdgeCollision& vertexEdgeCollision,
                                                                                 const glm::vec2&           bodyALinearVelocity,
                                                                                 float                      bodyAAngularVelocity,
                                                                                 const glm::vec2&           bodyBLinearVelocity,
                                                                                 float                      bodyBAngularVelocity)
{
   RigidBody2D& bodyA = mRigidBodies[vertexEdgeCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexEdgeCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexEdgeCollision.collidingVertexAIndex];
   glm::vec2 bodyBPoint  = vertexEdgeCollision.collidingBodyBPoint;

   // Calculate the velocity of the vertex on body A
   glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
   glm::vec2 bodyAVertexVelocity          = bodyALinearVelocity + (bodyAAngularVelocity * bodyACMToVertexPerpendicular);

   // Calculate the velocity of the vertex on body B
   glm::vec2 bodyBCMToPoint              = bodyBPoint - bodyB.mStates[1].positionOfCenterOfMass;
   glm::vec2 bodyBCMToPointPerpendicular = glm::vec2(-bodyBCMToPoint.y, bodyBCMToPoint.x);
   glm::vec2 bodyBPointVelocity          = bodyBLinearVelocity + (bodyBAngularVelocity * bodyBCMToPointPerpendicular);

   // Calculate the relative normal velocity
   glm::vec2 relativeVelocity   = bodyAVertexVelocity - bodyBPointVelocity;
   float relativeNormalVelocity = glm::dot(relativeVelocity, vertexEdgeCollision.collisionNormal);

   // Calculate the impulse's numerator
   float impulseNumerator = -(1.0f + mCoefficientOfRestitution) * relativeNormalVelocity; // TODO: Currently using coefficient of restitution of body A. That value should not be stored in the body.

   float bodyACMToVertPerpDotColliNormal  = glm::dot(bodyACMToVertexPerpendicular, vertexEdgeCollision.collisionNormal);
   float bodyBCMToPointPerpDotColliNormal = glm::dot(bodyBCMToPointPerpendicular, vertexEdgeCollision.collisionNormal);

   // Calculate the impulse's denominator
   float impulseDenominator = (bodyA.mOneOverMass + bodyB.mOneOverMass) +
                              (bodyA.mOneOverMomentOfInertia * bodyACMToVertPerpDotColliNormal * bodyACMToVertPerpDotColliNormal) +
                              (bodyB.mOneOverMomentOfInertia * bodyBCMToPointPerpDotColliNormal * bodyBCMToPointPerpDotColliNormal);

   float impulse = impulseNumerator / impulseDenominator;

   //bodyA.mStates[1].velocityOfCenterOfMass += ((impulse * bodyA.mOneOverMass) * vertexEdgeCollision.collisionNormal);
   //bodyA.mStates[1].angularVelocity        += ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal);

   //bodyB.mStates[1].velocityOfCenterOfMass += ((-impulse * bodyB.mOneOverMass) * vertexEdgeCollision.collisionNormal);
   //bodyB.mStates[1].angularVelocity        += ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToPointPerpDotColliNormal);

   glm::vec2 bodyALinearVelocityAfterCollision  = (bodyALinearVelocity  + ((impulse * bodyA.mOneOverMass) * vertexEdgeCollision.collisionNormal));
   float     bodyAAngularVelocityAfterCollision = (bodyAAngularVelocity + ((impulse * bodyA.mOneOverMomentOfInertia) * bodyACMToVertPerpDotColliNormal));

   glm::vec2 bodyBLinearVelocityAfterCollision  = (bodyBLinearVelocity  + ((-impulse * bodyB.mOneOverMass) * vertexEdgeCollision.collisionNormal));
   float     bodyBAngularVelocityAfterCollision = (bodyBAngularVelocity + ((-impulse * bodyB.mOneOverMomentOfInertia) * bodyBCMToPointPerpDotColliNormal));

   return std::make_tuple(bodyALinearVelocityAfterCollision,
                          bodyAAngularVelocityAfterCollision,
                          bodyBLinearVelocityAfterCollision,
                          bodyBAngularVelocityAfterCollision);
}

bool World::isVertexEdgeCollisionResolved(const VertexEdgeCollision& vertexEdgeCollision,
                                          const glm::vec2&           bodyALinearVelocity,
                                          float                      bodyAAngularVelocity,
                                          const glm::vec2&           bodyBLinearVelocity,
                                          float                      bodyBAngularVelocity)
{
   RigidBody2D& bodyA = mRigidBodies[vertexEdgeCollision.collidingBodyAIndex];
   RigidBody2D& bodyB = mRigidBodies[vertexEdgeCollision.collidingBodyBIndex];

   glm::vec2 bodyAVertex = bodyA.mStates[1].vertices[vertexEdgeCollision.collidingVertexAIndex];

   glm::vec2 closestPointOnBodyBEdgeToBodyAVertex = vertexEdgeCollision.collidingBodyBPoint;

   // If the distance between bodyAVertex and its closest point on bodyBEdge is smaller than 0.1f, then we check for a collison
   float distanceFromBodyAVertexToClosestPointOnBodyBEdge = glm::length(closestPointOnBodyBEdgeToBodyAVertex - bodyAVertex);
   if (distanceFromBodyAVertexToClosestPointOnBodyBEdge < 0.1f) // TODO: Make threshold a constant
   {
      // Calculate the velocity of bodyAVertex
      glm::vec2 bodyACMToVertex              = bodyAVertex - bodyA.mStates[1].positionOfCenterOfMass;
      glm::vec2 bodyACMToVertexPerpendicular = glm::vec2(-bodyACMToVertex.y, bodyACMToVertex.x);
      glm::vec2 bodyAVertexVelocity          = bodyALinearVelocity + (bodyAAngularVelocity * bodyACMToVertexPerpendicular);

      // Calculate the velocity of the closest point on bodyBEdge to bodyAVertex
      glm::vec2 bodyBCMToClosestPoint              = closestPointOnBodyBEdgeToBodyAVertex - bodyB.mStates[1].positionOfCenterOfMass;
      glm::vec2 bodyBCMToClosestPointPerpendicular = glm::vec2(-bodyBCMToClosestPoint.y, bodyBCMToClosestPoint.x);
      glm::vec2 bodyBClosestPointVelocity          = bodyBLinearVelocity + (bodyBAngularVelocity * bodyBCMToClosestPointPerpendicular);

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
         return false;
      }
   }

   return true;
}

World::BodyWallCollision::BodyWallCollision()
   : collisionNormal(glm::vec2(0.0f))
   , collidingBodyIndex(0)
   , collidingVertexIndex(0)
   , collidingWallIndex(0)
{

}

World::BodyWallCollision::BodyWallCollision(const glm::vec2& collisionNormal,
                                            int              collidingBodyIndex,
                                            int              collidingVertexIndex,
                                            int              collidingWallIndex)
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

World::VertexVertexCollision::VertexVertexCollision(const glm::vec2& collisionNormal,
                                                    int              collidingBodyAIndex,
                                                    int              collidingBodyBIndex,
                                                    int              collidingVertexAIndex,
                                                    int              collidingVertexBIndex)
   : collisionNormal(collisionNormal)
   , collidingBodyAIndex(collidingBodyAIndex)
   , collidingBodyBIndex(collidingBodyBIndex)
   , collidingVertexAIndex(collidingVertexAIndex)
   , collidingVertexBIndex(collidingVertexBIndex)
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

World::VertexEdgeCollision::VertexEdgeCollision(const glm::vec2& collisionNormal,
                                                int              collidingBodyAIndex,
                                                int              collidingBodyBIndex,
                                                int              collidingVertexAIndex,
                                                const glm::vec2& collidingBodyBPoint)
   : collisionNormal(collisionNormal)
   , collidingBodyAIndex(collidingBodyAIndex)
   , collidingBodyBIndex(collidingBodyBIndex)
   , collidingVertexAIndex(collidingVertexAIndex)
   , collidingBodyBPoint(collidingBodyBPoint)
{

}
