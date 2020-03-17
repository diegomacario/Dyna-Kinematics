#include <glm/gtc/matrix_transform.hpp>

#include "game_object_2D.h"

GameObject2D::GameObject2D(const std::shared_ptr<Texture>& texture,
                           const glm::vec2&                position,
                           float                           angleOfRotInDeg,
                           float                           width,
                           float                           height)
   : mTexture(texture)
   , mPosition(position)
   , mAngleOfRotInDeg(angleOfRotInDeg)
   , mWidth(width != 0.0f ? width : 1.0f)
   , mHeight(height != 0.0f ? height : 1.0f)
   , mModelMatrix(1.0f)
   , mCalculateModelMatrix(true)
{
   calculateModelMatrix();
}

GameObject2D::GameObject2D(GameObject2D&& rhs) noexcept
   : mTexture(std::move(rhs.mTexture))
   , mPosition(std::exchange(rhs.mPosition, glm::vec2(0.0f)))
   , mAngleOfRotInDeg(std::exchange(rhs.mAngleOfRotInDeg, 0.0f))
   , mWidth(std::exchange(rhs.mWidth, 1.0f))
   , mHeight(std::exchange(rhs.mHeight, 1.0f))
   , mModelMatrix(std::exchange(rhs.mModelMatrix, glm::mat4(1.0f)))
   , mCalculateModelMatrix(std::exchange(rhs.mCalculateModelMatrix, true))
{

}

GameObject2D& GameObject2D::operator=(GameObject2D&& rhs) noexcept
{
   mTexture              = std::move(rhs.mTexture);
   mPosition             = std::exchange(rhs.mPosition, glm::vec2(0.0f));
   mAngleOfRotInDeg      = std::exchange(rhs.mAngleOfRotInDeg, 0.0f);
   mWidth                = std::exchange(rhs.mWidth, 1.0f);
   mHeight               = std::exchange(rhs.mHeight, 1.0f);
   mModelMatrix          = std::exchange(rhs.mModelMatrix, glm::mat4(1.0f));
   mCalculateModelMatrix = std::exchange(rhs.mCalculateModelMatrix, true);
   return *this;
}

std::shared_ptr<Texture> GameObject2D::getTexture() const
{
   return mTexture;
}

glm::mat4 GameObject2D::getModelMatrix() const
{
   if (mCalculateModelMatrix)
   {
      calculateModelMatrix();
   }

   return mModelMatrix;
}

void GameObject2D::translate(const glm::vec2& translation)
{
   mPosition += translation;
   mCalculateModelMatrix = true;
}

void GameObject2D::rotate(float angleOfRotInDeg)
{
   mAngleOfRotInDeg += angleOfRotInDeg;
   mCalculateModelMatrix = true;
}

void GameObject2D::scale(const glm::vec2& scalingFactors)
{
   if ((scalingFactors.x != 0.0f) && (scalingFactors.y != 0.0f))
   {
      mWidth *= scalingFactors.x;
      mHeight *= scalingFactors.y;
      mCalculateModelMatrix = true;
   }
}

void GameObject2D::calculateModelMatrix() const
{
   // 3) Translate the quad
   mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(mPosition, 0.0f));

   // 2) Rotate the quad around the Z axis
   mModelMatrix = glm::rotate(mModelMatrix, mAngleOfRotInDeg, glm::vec3(0.0f, 0.0f, 1.0f));

   // 1) Scale the quad
   mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mWidth, mHeight, 1.0f));

   mCalculateModelMatrix = false;
}
