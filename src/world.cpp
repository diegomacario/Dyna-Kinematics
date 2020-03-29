// TODO: Remove this
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>

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

// TODO: Remove this
static long long int simCounter = 0;
static bool first = true;
void World::simulate(float deltaTime)
{
   float currentTime = 0.0f;
   float targetTime  = deltaTime;

   while (currentTime < deltaTime)
   {
      static std::ofstream file;
      if (first)
      {
         file.open("cout.txt");
         std::streambuf* sbuf = std::cout.rdbuf();
         std::cout.rdbuf(file.rdbuf());
         std::cout << std::setprecision(16);
         first = false;
         std::cout << "Mio!" << '\n';
      }

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

            // TODO: Remove this
            std::cout << "Resolved collison!" << '\n';
            std::cout << "counter                = " << counter << '\n';
            std::cout << "positionOfCenterOfMass = " << mRigidBodies[0].mStates[1].positionOfCenterOfMass.x << " " << mRigidBodies[0].mStates[1].positionOfCenterOfMass.y << '\n';
            std::cout << "orientation            = " << mRigidBodies[0].mStates[1].orientation << '\n';
            std::cout << "velocityOfCenterOfMass = " << mRigidBodies[0].mStates[1].velocityOfCenterOfMass.x << " " << mRigidBodies[0].mStates[1].velocityOfCenterOfMass.y << '\n';
            std::cout << "angularVelocity        = " << mRigidBodies[0].mStates[1].angularVelocity << '\n';
            std::cout << "forceOfCenterOfMass    = " << mRigidBodies[0].mStates[1].forceOfCenterOfMass.x << " " << mRigidBodies[0].mStates[1].forceOfCenterOfMass.y << '\n';
            std::cout << "torque                 = " << mRigidBodies[0].mStates[1].torque << '\n';
            std::cout << "vertex[0]              = " << mRigidBodies[0].mStates[1].vertices[0].x << " " << mRigidBodies[0].mStates[1].vertices[0].y << '\n';
            std::cout << "vertex[1]              = " << mRigidBodies[0].mStates[1].vertices[1].x << " " << mRigidBodies[0].mStates[1].vertices[1].y << '\n';
            std::cout << "vertex[2]              = " << mRigidBodies[0].mStates[1].vertices[2].x << " " << mRigidBodies[0].mStates[1].vertices[2].y << '\n';
            std::cout << "vertex[3]              = " << mRigidBodies[0].mStates[1].vertices[3].x << " " << mRigidBodies[0].mStates[1].vertices[3].y << '\n' << '\n';

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

         // TODO: Remove this
         std::cout << "simCounter             = " << simCounter << '\n';
         ++simCounter;
         std::cout << "curr.positionOfCenterOfMass = " << mRigidBodies[0].mStates[0].positionOfCenterOfMass.x << " " << mRigidBodies[0].mStates[0].positionOfCenterOfMass.y << '\n';
         std::cout << "curr.orientation            = " << mRigidBodies[0].mStates[0].orientation << "\n\n";
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

      std::cout << "---------- INTEGRATION STUFF ----------" << '\n';

      std::cout << "currentState.positionOfCenterOfMass      = " << currentState.positionOfCenterOfMass.x << " " << currentState.positionOfCenterOfMass.y << '\n';
      std::cout << "currentState.velocityOfCenterOfMass      = " << currentState.velocityOfCenterOfMass.x << " " << currentState.velocityOfCenterOfMass.y << '\n';
      std::cout << "deltaTime                                = " << deltaTime << '\n';
      std::cout << "futureState.positionOfCenterOfMass       = " << futureState.positionOfCenterOfMass.x << " " << futureState.positionOfCenterOfMass.y << '\n';

      std::cout << "currentState.orientation     = " << currentState.orientation << '\n';
      std::cout << "deltaTime                    = " << deltaTime << '\n';
      std::cout << "currentState.angularVelocity = " << currentState.angularVelocity << '\n';
      std::cout << "futureState.orientation      = " << futureState.orientation << '\n';

      // Calculate new velocity and angular velocity

      // F = (d/dt)P = M * A
      // A = F / M
      // V_n+1 = V_n + (h * A) = V_n + (h * (F / M))
      //futureState.velocityOfCenterOfMass = currentState.velocityOfCenterOfMass + ((currentState.forceOfCenterOfMass * iter->mOneOverMass) * deltaTime);
      futureState.velocityOfCenterOfMass = currentState.velocityOfCenterOfMass + (deltaTime * iter->mOneOverMass) * currentState.forceOfCenterOfMass;
      std::cout << "futureState.velocityOfCenterOfMass      = " << futureState.velocityOfCenterOfMass.x << " " << futureState.velocityOfCenterOfMass.y << '\n';

      // T = (d/dt)L = I * Alpha
      // Alpha = T / I
      // W_n+1 = W_n + (h * Alpha) = W_n + (h * (T / I))
      //futureState.angularVelocity = currentState.angularVelocity + ((currentState.torque * iter->mOneOverMomentOfInertia) * deltaTime);
      futureState.angularVelocity = currentState.angularVelocity + (deltaTime * iter->mOneOverMomentOfInertia) * currentState.torque;
      std::cout << "futureState.angularVelocity      = " << futureState.angularVelocity << '\n';

      std::cout << "-----------------------------------" << '\n';
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
   std::cout << "---------- IMPULSE STUFF ----------" << '\n';
   std::cout << "CMToVertexPerpendicular    = " << CMToVertexPerpendicular.x << " " << CMToVertexPerpendicular.y << '\n';

   // Chasles' Theorem
   // We consider any of movement of a rigid body as a simple translation of a single point in the body (the center of mass)
   // and a simple rotation of the rest of the body around that point
   glm::vec2 vertexVelocity = futureState.velocityOfCenterOfMass + (futureState.angularVelocity * CMToVertexPerpendicular);
   std::cout << "vertexVelocity             = " << vertexVelocity.x << " " << vertexVelocity.y << '\n';

   // The wall doesn't move and has an infinite mass, which simplifies the collision response equations

   // The relative normal velocity is the component of the relative velocity in the direction of the collision normal
   // Because the wall is not moving, the relative velocity is the velocity of the vertex
   float relativeNormalVelocity = glm::dot(vertexVelocity, mCollisionNormal);
   std::cout << "relativeNormalVelocity     = " << relativeNormalVelocity << '\n';
   float impulseNumerator       = -(1.0f + body.mCoefficientOfRestitution) * relativeNormalVelocity;
   std::cout << "impulseNumerator           = " << impulseNumerator << '\n';

   float CMToVertPerpDotColliNormal = glm::dot(CMToVertexPerpendicular, mCollisionNormal);
   std::cout << "   CMToVertPerpDotColliNormal = " << CMToVertPerpDotColliNormal << '\n';
   std::cout << "   mOneOverMass               = " << body.mOneOverMass << '\n';
   std::cout << "   mOneOverMomentOfInertia    = " << body.mOneOverMomentOfInertia << '\n';
   //float impulseDenominator         = body.mOneOverMass + (body.mOneOverMomentOfInertia * (CMToVertPerpDotColliNormal * CMToVertPerpDotColliNormal));
   float impulseDenominator         = body.mOneOverMass + (body.mOneOverMomentOfInertia * CMToVertPerpDotColliNormal * CMToVertPerpDotColliNormal);
   std::cout << "impulseDenominator         = " << impulseDenominator << '\n';

   float impulse = impulseNumerator / impulseDenominator;

   // TODO: Remove this
   std::cout << "Impulse                    = " << impulse << '\n';

   futureState.velocityOfCenterOfMass += ((impulse * body.mOneOverMass) * mCollisionNormal);
   std::cout << "CMVelocity                 = " << futureState.velocityOfCenterOfMass.x << " " << futureState.velocityOfCenterOfMass.y << '\n';
   futureState.angularVelocity        += ((impulse * body.mOneOverMomentOfInertia) * CMToVertPerpDotColliNormal);
   std::cout << "AngularVelocity            = " << futureState.angularVelocity << '\n';

   std::cout << "-----------------------------------" << '\n';
}
