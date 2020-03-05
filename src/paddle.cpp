#include "paddle.h"

Paddle::Paddle(const std::shared_ptr<Model>& model,
           const glm::vec3&              position,
           float                         angleOfRotInDeg,
           const glm::vec3&              axisOfRot,
           float                         scalingFactor,
           const glm::vec3&              velocity,
           float                         width,
           float                         height)
   : MovableGameObject3D(model,
                         position,
                         angleOfRotInDeg,
                         axisOfRot,
                         scalingFactor,
                         velocity)
   , mWidth(width)
   , mHeight(height)
{

}

Paddle::Paddle(Paddle&& rhs) noexcept
   : MovableGameObject3D(std::move(rhs))
   , mWidth(std::exchange(rhs.mWidth, 0.0f))
   , mHeight(std::exchange(rhs.mHeight, 0.0f))
{

}

Paddle& Paddle::operator=(Paddle&& rhs) noexcept
{
   GameObject3D::operator=(std::move(rhs));
   mWidth  = std::exchange(rhs.mWidth, 0.0f);
   mHeight = std::exchange(rhs.mHeight, 0.0f);
   return *this;
}

void Paddle::moveAlongLine(float deltaTime, float lineLength, MovementDirection direction)
{
   switch (direction)
   {
   case MovementDirection::Up:
      if ((this->getPosition().y + (mHeight / 2.0f)) < (lineLength / 2.0f))
      {
         this->translate(this->getVelocity() * deltaTime);
      }
      break;
   case MovementDirection::Down:
      if ((this->getPosition().y - (mHeight / 2.0f)) > -(lineLength / 2.0f))
      {
         this->translate(-this->getVelocity() * deltaTime);
      }
      break;
   }
}

float Paddle::getWidth() const
{
   return mWidth;
}

float Paddle::getHeight() const
{
   return mHeight;
}
