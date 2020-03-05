#include <glm/gtc/matrix_transform.hpp>

#include "game_object_2D.h"

GameObject2D::GameObject2D(const std::shared_ptr<Texture>& texture,
                           const glm::vec2&                posOfTopLeftCornerInPix,
                           float                           angleOfRotInDeg,
                           float                           widthInPix,
                           float                           heightInPix)
   : mTexture(texture)
   , mPosOfTopLeftCornerInPix(posOfTopLeftCornerInPix)
   , mAngleOfRotInDeg(angleOfRotInDeg)
   , mWidthInPix(widthInPix != 0.0f ? widthInPix : 1.0f)
   , mHeightInPix(heightInPix != 0.0f ? heightInPix : 1.0f)
   , mModelMatrix(1.0f)
   , mCalculateModelMatrix(true)
{
   calculateModelMatrix();
}

GameObject2D::GameObject2D(GameObject2D&& rhs) noexcept
   : mTexture(std::move(rhs.mTexture))
   , mPosOfTopLeftCornerInPix(std::exchange(rhs.mPosOfTopLeftCornerInPix, glm::vec2(0.0f)))
   , mAngleOfRotInDeg(std::exchange(rhs.mAngleOfRotInDeg, 0.0f))
   , mWidthInPix(std::exchange(rhs.mWidthInPix, 1.0f))
   , mHeightInPix(std::exchange(rhs.mHeightInPix, 1.0f))
   , mModelMatrix(std::exchange(rhs.mModelMatrix, glm::mat4(1.0f)))
   , mCalculateModelMatrix(std::exchange(rhs.mCalculateModelMatrix, true))
{

}

GameObject2D& GameObject2D::operator=(GameObject2D&& rhs) noexcept
{
   mTexture                 = std::move(rhs.mTexture);
   mPosOfTopLeftCornerInPix = std::exchange(rhs.mPosOfTopLeftCornerInPix, glm::vec2(0.0f));
   mAngleOfRotInDeg         = std::exchange(rhs.mAngleOfRotInDeg, 0.0f);
   mWidthInPix              = std::exchange(rhs.mWidthInPix, 1.0f);
   mHeightInPix             = std::exchange(rhs.mHeightInPix, 1.0f);
   mModelMatrix             = std::exchange(rhs.mModelMatrix, glm::mat4(1.0f));
   mCalculateModelMatrix    = std::exchange(rhs.mCalculateModelMatrix, true);
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
   mPosOfTopLeftCornerInPix += translation;
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
      mWidthInPix *= scalingFactors.x;
      mHeightInPix *= scalingFactors.y;
      mCalculateModelMatrix = true;
   }
}

void GameObject2D::calculateModelMatrix() const
{
   // 5) Translate the quad
   mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(mPosOfTopLeftCornerInPix, 0.0f));

   // 4) Move the center of rotation back to the top left corner
   mModelMatrix = glm::translate(mModelMatrix, glm::vec3(0.5f * mWidthInPix, 0.5f * mHeightInPix, 0.0f));
   // 3) Rotate the quad around the Z axis
   mModelMatrix = glm::rotate(mModelMatrix, mAngleOfRotInDeg, glm::vec3(0.0f, 0.0f, 1.0f));
   // 2) Move the center of rotation to the center of the quad
   mModelMatrix = glm::translate(mModelMatrix, glm::vec3(-0.5f * mWidthInPix, -0.5f * mHeightInPix, 0.0f));

   // 1) Scale the quad
   mModelMatrix = glm::scale(mModelMatrix, glm::vec3(mWidthInPix, mHeightInPix, 1.0f));

   mCalculateModelMatrix = false;
}
