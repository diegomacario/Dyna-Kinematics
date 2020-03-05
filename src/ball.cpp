#include "ball.h"

Ball::Ball(const std::shared_ptr<Model>& model,
           const glm::vec3&              position,
           float                         angleOfRotInDeg,
           const glm::vec3&              axisOfRot,
           float                         scalingFactor,
           const glm::vec3&              velocity,
           float                         radius,
           float                         spinAngularVelocity)
   : MovableGameObject3D(model,
                         position,
                         angleOfRotInDeg,
                         axisOfRot,
                         scalingFactor,
                         velocity)
   , mInitialVelocity(velocity)
   , mRadius(radius)
   , mSpinAngularVelocity(spinAngularVelocity)
   , mSpinAngularVelocityScaledByBounce(spinAngularVelocity)
{

}

Ball::Ball(Ball&& rhs) noexcept
   : MovableGameObject3D(std::move(rhs))
   , mInitialVelocity(std::exchange(rhs.mInitialVelocity, glm::vec3(0.0f)))
   , mRadius(std::exchange(rhs.mRadius, 0.0f))
   , mSpinAngularVelocity(std::exchange(rhs.mSpinAngularVelocity, 0.0f))
   , mSpinAngularVelocityScaledByBounce(std::exchange(rhs.mSpinAngularVelocityScaledByBounce, 0.0f))
{

}

Ball& Ball::operator=(Ball&& rhs) noexcept
{
   GameObject3D::operator=(std::move(rhs));
   mInitialVelocity                   = std::exchange(rhs.mInitialVelocity, glm::vec3(0.0f));
   mRadius                            = std::exchange(rhs.mRadius, 0.0f);
   mSpinAngularVelocity               = std::exchange(rhs.mSpinAngularVelocity, 0.0f);
   mSpinAngularVelocityScaledByBounce = std::exchange(rhs.mSpinAngularVelocityScaledByBounce, 0.0f);
   return *this;
}

void Ball::moveWithinVerticalRange(float deltaTime, float verticalRange)
{
   glm::vec3 currVelocity     = this->getVelocity();
   glm::vec3 currPos          = this->getPosition() + (currVelocity * deltaTime);

   float topBoundary          =   verticalRange / 2.0f;
   float bottomBoundary       =  -topBoundary;

   glm::vec3 incidentVelocity = currVelocity;
   glm::vec3 normalOfCrossedBoundary;
   bool bounced = false;

   if ((currPos.y + mRadius) >= topBoundary)
   {
      // Bounce down
      currVelocity.y          = -currVelocity.y;
      currPos.y               = topBoundary - mRadius;
      normalOfCrossedBoundary = glm::vec3(0.0f, -1.0f, 0.0f);
      bounced                 = true;
   }
   else if ((currPos.y - mRadius) <= bottomBoundary)
   {
      // Bounce up
      currVelocity.y          = -currVelocity.y;
      currPos.y               = bottomBoundary + mRadius;
      normalOfCrossedBoundary = glm::vec3(0.0f, 1.0f, 0.0f);
      bounced                 = true;
   }

   if (bounced)
   {
      // Absolute value of the dot product of:
      // - The normalized incident velocity
      // - The normalized normal of the crossed boundary
      // This value ranges from 0 to 1
      // It is equal to 0 when the two vectors are perpendicular (tangent collision)
      // It is equal to 1 when the two vectors are parallel (direct collision)
      float dotProduct = glm::abs(glm::dot(glm::normalize(incidentVelocity), normalOfCrossedBoundary));

      // The ball spins with its maximum angular velocity when a tangent collision occurs
      // The ball spins with its minimum angular velocity, that is, it doesn't spin, when a direct collision occurs
      mSpinAngularVelocityScaledByBounce = mSpinAngularVelocity * (1.0f - dotProduct);
   }

   this->setVelocity(currVelocity);
   this->setPosition(currPos);
   this->rotate(mSpinAngularVelocityScaledByBounce * deltaTime, glm::vec3(0.0f, 0.0f, 1.0f));
}

void Ball::moveInFreeFall(float deltaTime)
{
   glm::vec3 currPos     = this->getPosition() + (this->getVelocity() * deltaTime);
   float     degToRotate = mSpinAngularVelocityScaledByBounce * deltaTime;

   this->setPosition(currPos);

   glm::vec3 currVelocity = this->getVelocity();
   this->rotate(degToRotate, glm::vec3(currVelocity.x, currVelocity.y, 0.0f));
}

void Ball::reset()
{
   this->setPosition(glm::vec3(0.0f, 0.0f, this->getScalingFactor() * 1.96875));
   this->setRotationMatrix(glm::mat4(1.0f));
   this->rotate(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
   this->setVelocity(mInitialVelocity);
   mSpinAngularVelocityScaledByBounce = mSpinAngularVelocity;
}

glm::vec3 Ball::getInitialVelocity() const
{
   return mInitialVelocity;
}

float Ball::getRadius() const
{
   return mRadius;
}

void Ball::setRadius(float radius)
{
   mRadius = radius;
}
