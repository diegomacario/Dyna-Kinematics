#ifndef RENDERER_2D_H
#define RENDERER_2D_H

#include "shader.h"
#include "rigid_body_2D.h"
#include "wall.h"

class Renderer2D
{
public:

   Renderer2D(const std::shared_ptr<Shader>& texShader,
              const std::shared_ptr<Shader>& colorShader,
              const std::shared_ptr<Shader>& lineShader);
   ~Renderer2D();

   Renderer2D(const Renderer2D&) = delete;
   Renderer2D& operator=(const Renderer2D&) = delete;

   Renderer2D(Renderer2D&& rhs) noexcept;
   Renderer2D& operator=(Renderer2D&& rhs) noexcept;

   void renderRigidBody(const RigidBody2D& rigidBody2D, bool wireframe) const;
   void renderLine(const Wall& wall) const;

   void updateOrthographicProjection(float width, float height) const;

private:

   void configureVAOs();

   void configureRealVAOs();

   std::shared_ptr<Shader> mTexShader;
   std::shared_ptr<Shader> mColorShader;
   std::shared_ptr<Shader> mLineShader;

   unsigned int            mTexturedQuadVAO;
   unsigned int            mColoredQuadVAO;

   unsigned int            mRealTexturedQuadVAO;
   unsigned int            mRealColoredQuadVAO;

   unsigned int            mQuadVBO;
   unsigned int            mQuadEBO;

   unsigned int            mRealQuadVBO;
   unsigned int            mRealQuadEBO;
};

#endif
