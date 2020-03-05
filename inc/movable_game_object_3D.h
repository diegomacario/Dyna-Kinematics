#ifndef MOVABLE_GAME_OBJECT_3D_H
#define MOVABLE_GAME_OBJECT_3D_H

#include "game_object_3D.h"

class MovableGameObject3D : public GameObject3D
{
public:

   MovableGameObject3D(const std::shared_ptr<Model>& model,
                       const glm::vec3&              position,
                       float                         angleOfRotInDeg,
                       const glm::vec3&              axisOfRot,
                       float                         scalingFactor,
                       const glm::vec3&              velocity);
   ~MovableGameObject3D() = default;

   MovableGameObject3D(const MovableGameObject3D&) = default;
   MovableGameObject3D& operator=(const MovableGameObject3D&) = default;

   MovableGameObject3D(MovableGameObject3D&& rhs) noexcept;
   MovableGameObject3D& operator=(MovableGameObject3D&& rhs) noexcept;

   glm::vec3 getVelocity() const;
   void      setVelocity(const glm::vec3& velocity);

private:

   glm::vec3 mVelocity;
};

#endif
