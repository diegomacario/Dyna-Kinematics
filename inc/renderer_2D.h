#ifndef RENDERER_2D_H
#define RENDERER_2D_H

#include "shader.h"
#include "game_object_2D.h"
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

   void renderTexturedQuad(const GameObject2D& gameObj2D) const;
   void renderColoredQuad(const GameObject2D& gameObj2D) const;
   void renderRigidBody(const RigidBody2D& rigidBody2D) const;
   void renderLine(const Wall& wall) const;

private:

   void configureVAOs();

   std::shared_ptr<Shader> mTexShader;
   std::shared_ptr<Shader> mColorShader;
   std::shared_ptr<Shader> mLineShader;

   unsigned int            mTexturedQuadVAO;
   unsigned int            mColoredQuadVAO;

   unsigned int            mQuadVBO;
   unsigned int            mQuadEBO;
};

#endif
