#ifndef GAME_OBJECT_2D_H
#define GAME_OBJECT_2D_H

#include <glm/glm.hpp>

#include <memory>

#include "texture.h"

class GameObject2D
{
public:

   GameObject2D(const std::shared_ptr<Texture>& texture,
                const glm::vec2&                posOfTopLeftCornerInPix,
                float                           angleOfRotInDeg,
                float                           widthInPix,
                float                           heightInPix);
   ~GameObject2D() = default;

   GameObject2D(const GameObject2D&) = default;
   GameObject2D& operator=(const GameObject2D&) = default;

   GameObject2D(GameObject2D&& rhs) noexcept;
   GameObject2D& operator=(GameObject2D&& rhs) noexcept;

   std::shared_ptr<Texture> getTexture() const;
   glm::mat4                getModelMatrix() const;

   void                     translate(const glm::vec2& translation);
   void                     rotate(float angleOfRotInDeg);
   void                     scale(const glm::vec2& scalingFactors);

private:

   void                     calculateModelMatrix() const;

   std::shared_ptr<Texture> mTexture;

   glm::vec2                mPosOfTopLeftCornerInPix;
   float                    mAngleOfRotInDeg;
   float                    mWidthInPix;
   float                    mHeightInPix;

   mutable glm::mat4        mModelMatrix;
   mutable bool             mCalculateModelMatrix;
};

#endif
