#include <glm/gtc/matrix_transform.hpp>

#include "game_object_3D.h"

GameObject3D::GameObject3D(const std::shared_ptr<Model>& model,
                           const glm::vec3&              position,
                           float                         angleOfRotInDeg,
                           const glm::vec3&              axisOfRot,
                           float                         scalingFactor)
   : mModel(model)
   , mPosition(position)
   , mRotationMatrix(axisOfRot != glm::vec3(0.0f) ? glm::rotate(glm::mat4(1.0f), glm::radians(angleOfRotInDeg), axisOfRot) : glm::mat4(1.0f))
   , mScalingFactor(scalingFactor != 0.0f ? scalingFactor : 1.0f)
   , mModelMatrix(1.0f)
   , mCalculateModelMatrix(true)
{
   calculateModelMatrix();
}

GameObject3D::GameObject3D(GameObject3D&& rhs) noexcept
   : mModel(std::move(rhs.mModel))
   , mPosition(std::exchange(rhs.mPosition, glm::vec3(0.0f)))
   , mRotationMatrix(std::exchange(rhs.mRotationMatrix, glm::mat4(1.0f)))
   , mScalingFactor(std::exchange(rhs.mScalingFactor, 1.0f))
   , mModelMatrix(std::exchange(rhs.mModelMatrix, glm::mat4(1.0f)))
   , mCalculateModelMatrix(std::exchange(rhs.mCalculateModelMatrix, true))
{

}

GameObject3D& GameObject3D::operator=(GameObject3D&& rhs) noexcept
{
   mModel                = std::move(rhs.mModel);
   mPosition             = std::exchange(rhs.mPosition, glm::vec3(0.0f));
   mRotationMatrix       = std::exchange(rhs.mRotationMatrix, glm::mat4(1.0f));
   mScalingFactor        = std::exchange(rhs.mScalingFactor, 1.0f);
   mModelMatrix          = std::exchange(rhs.mModelMatrix, glm::mat4(1.0f));
   mCalculateModelMatrix = std::exchange(rhs.mCalculateModelMatrix, true);
   return *this;
}

void GameObject3D::render(const Shader& shader) const
{
   if (mCalculateModelMatrix)
   {
      calculateModelMatrix();
   }

   shader.setMat4("model", mModelMatrix);

   mModel->render(shader);
}

glm::vec3 GameObject3D::getPosition() const
{
   return mPosition;
}

void GameObject3D::setPosition(const glm::vec3& position)
{
   mPosition = position;
   mCalculateModelMatrix = true;
}

float GameObject3D::getScalingFactor() const
{
   return mScalingFactor;
}

void GameObject3D::setRotationMatrix(const glm::mat4& rotationMatrix)
{
   mRotationMatrix = rotationMatrix;
   mCalculateModelMatrix = true;
}

void GameObject3D::translate(const glm::vec3& translation)
{
   mPosition += translation;
   mCalculateModelMatrix = true;
}

void GameObject3D::rotate(float angleOfRotInDeg, const glm::vec3& axisOfRot)
{
   if (axisOfRot != glm::vec3(0.0f))
   {
      mRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angleOfRotInDeg), axisOfRot) * mRotationMatrix;
      mCalculateModelMatrix = true;
   }
}

void GameObject3D::scale(float scalingFactor)
{
   if (scalingFactor != 0.0f)
   {
      mScalingFactor *= scalingFactor;
      mCalculateModelMatrix = true;
   }
}

void GameObject3D::calculateModelMatrix() const
{
   // 3) Translate the model
   mModelMatrix = glm::translate(glm::mat4(1.0f), mPosition);

   // 2) Rotate the model
   mModelMatrix *= mRotationMatrix;

   // 1) Scale the model
   mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mScalingFactor));

   mCalculateModelMatrix = false;
}
