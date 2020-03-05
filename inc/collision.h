#ifndef COLLISION_H
#define COLLISION_H

#include "ball.h"
#include "paddle.h"

enum class CollisionDirection
{
   Up,
   Down,
   Left,
   Right
};

bool               circleAndAABBCollided(const Ball& circle, const Paddle& AABB, glm::vec2& vecFromCenterOfCircleToPointOfCollision);
CollisionDirection determineDirectionOfCollisionBetweenCircleAndAABB(const glm::vec2& vecFromCenterOfCircleToPointOfCollision);

#endif
