#include <array>

#include "collision.h"

bool circleAndAABBCollided(const Ball& circle, const Paddle& AABB, glm::vec2& vecFromCenterOfCircleToPointOfCollision)
{
   glm::vec2 centerOfCircle(circle.getPosition());

   glm::vec2 centerOfAABB(AABB.getPosition());
   glm::vec2 halfExtentsOfAABB(AABB.getWidth() / 2, AABB.getHeight() / 2);

   // Calculate the difference vector between both centers and clamp it to the AABB's half-extents
   glm::vec2 vecFromCenterOfAABBToCenterOfCircle        = centerOfCircle - centerOfAABB;
   glm::vec2 clampedVecFromCenterOfAABBToCenterOfCircle = glm::clamp(vecFromCenterOfAABBToCenterOfCircle, -halfExtentsOfAABB, halfExtentsOfAABB);

   // Add the clamped difference vector to the AABB's center to get the point on the AABB's edge that is closest to the center of the circle
   glm::vec2 closest = centerOfAABB + clampedVecFromCenterOfAABBToCenterOfCircle;

   // Calculate the difference vector between the center of the circle and the closest point on the AABB's edge
   vecFromCenterOfCircleToPointOfCollision = closest - centerOfCircle;

   // Check if the distance between the center of the circle and the closest point on the AABB's edge is smaller than the radius of the circle, which would indicate a collision
   // Note that the check is not <= because in that case, a collision would also occur when the circle and the AABB are exactly touching each other,
   // which is the state in which we leave the circle and the AABB after they collide
   if (glm::length(vecFromCenterOfCircleToPointOfCollision) < circle.getRadius())
   {
      return true;
   }
   else
   {
      return false;
   }
}

CollisionDirection determineDirectionOfCollisionBetweenCircleAndAABB(const glm::vec2& vecFromCenterOfCircleToPointOfCollision)
{
   std::array<glm::vec2, 4> collisionDirections = {glm::vec2(0.0f, 1.0f),  // Up
                                                   glm::vec2(0.0f, -1.0f), // Down
                                                   glm::vec2(-1.0f, 0.0f), // Left
                                                   glm::vec2(1.0f, 0.0f)}; // Right

   glm::vec2 normalizedVecFromCenterOfCircleToPointOfCollision = glm::normalize(vecFromCenterOfCircleToPointOfCollision);

   float        dotProduct                = 0.0f;
   float        maxDotProduct             = 0.0f;
   unsigned int closestCollisionDirection = -1;
   for (unsigned int i = 0; i < 4; i++)
   {
      dotProduct = glm::dot(normalizedVecFromCenterOfCircleToPointOfCollision, collisionDirections[i]);
      if (dotProduct > maxDotProduct)
      {
         maxDotProduct = dotProduct;
         closestCollisionDirection = i;
      }
   }

   return static_cast<CollisionDirection>(closestCollisionDirection);
}

