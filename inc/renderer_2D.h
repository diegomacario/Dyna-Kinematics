#ifndef RENDERER_2D_H
#define RENDERER_2D_H

#include "shader.h"
#include "game_object_2D.h"

class Renderer2D
{
public:

   Renderer2D(const std::shared_ptr<Shader>& shader);
   ~Renderer2D();

   Renderer2D(const Renderer2D&) = delete;
   Renderer2D& operator=(const Renderer2D&) = delete;

   Renderer2D(Renderer2D&& rhs) noexcept;
   Renderer2D& operator=(Renderer2D&& rhs) noexcept;

   void render(const GameObject2D& gameObj2D) const;

private:

   void configureVAO();

   std::shared_ptr<Shader> mShader;
   unsigned int            mVAO;
   unsigned int            mVBO;
   unsigned int            mEBO;
};

#endif
