#include "movable_game_object_3D.h"

MovableGameObject3D::MovableGameObject3D(const std::shared_ptr<Model>& model,
                                         const glm::vec3&              position,
                                         float                         angleOfRotInDeg,
                                         const glm::vec3&              axisOfRot,
                                         float                         scalingFactor,
                                         const glm::vec3&              velocity)
   : GameObject3D(model,
                  position,
                  angleOfRotInDeg,
                  axisOfRot,
                  scalingFactor)
   , mVelocity(velocity)
{

}

MovableGameObject3D::MovableGameObject3D(MovableGameObject3D&& rhs) noexcept
   : GameObject3D(std::move(rhs))
   , mVelocity(std::exchange(rhs.mVelocity, glm::vec3(0.0f)))
{

}

MovableGameObject3D& MovableGameObject3D::operator=(MovableGameObject3D&& rhs) noexcept
{
   GameObject3D::operator=(std::move(rhs));
   mVelocity = std::exchange(rhs.mVelocity, glm::vec3(0.0f));
   return *this;
}

glm::vec3 MovableGameObject3D::getVelocity() const
{
   return mVelocity;
}

void MovableGameObject3D::setVelocity(const glm::vec3& velocity)
{
   mVelocity = velocity;
}
