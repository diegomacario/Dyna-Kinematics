#ifndef GAME_OBJECT_3D_H
#define GAME_OBJECT_3D_H

#include <glm/glm.hpp>

#include <memory>

#include "model.h"

class GameObject3D
{
public:

   GameObject3D(const std::shared_ptr<Model>& model,
                const glm::vec3&              position,
                float                         angleOfRotInDeg,
                const glm::vec3&              axisOfRot,
                float                         scalingFactor);
   ~GameObject3D() = default;

   GameObject3D(const GameObject3D&) = default;
   GameObject3D& operator=(const GameObject3D&) = default;

   GameObject3D(GameObject3D&& rhs) noexcept;
   GameObject3D& operator=(GameObject3D&& rhs) noexcept;

   void      render(const Shader& shader) const;

   glm::vec3 getPosition() const;
   void      setPosition(const glm::vec3& position);

   float     getScalingFactor() const;

   void      setRotationMatrix(const glm::mat4& rotationMatrix);

   void      translate(const glm::vec3& translation);
   void      rotate(float angleOfRotInDeg, const glm::vec3& axisOfRot);
   void      scale(float scalingFactor);

private:

   void      calculateModelMatrix() const;

   std::shared_ptr<Model> mModel;

   glm::vec3              mPosition;
   glm::mat4              mRotationMatrix;
   float                  mScalingFactor;

   mutable glm::mat4      mModelMatrix;
   mutable bool           mCalculateModelMatrix;
};

#endif
