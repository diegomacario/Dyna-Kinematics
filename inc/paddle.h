#ifndef PADDLE_H
#define PADDLE_H

#include "movable_game_object_3D.h"

class Paddle : public MovableGameObject3D
{
public:

   Paddle(const std::shared_ptr<Model>& model,
          const glm::vec3&              position,
          float                         angleOfRotInDeg,
          const glm::vec3&              axisOfRot,
          float                         scalingFactor,
          const glm::vec3&              velocity,
          float                         width,
          float                         height);
   ~Paddle() = default;

   Paddle(const Paddle&) = default;
   Paddle& operator=(const Paddle&) = default;

   Paddle(Paddle&& rhs) noexcept;
   Paddle& operator=(Paddle&& rhs) noexcept;

   enum class MovementDirection
   {
      Up,
      Down,
   };

   void  moveAlongLine(float deltaTime, float lineLength, MovementDirection direction);

   float getWidth() const;
   float getHeight() const;

private:

   float mWidth;
   float mHeight;
};

#endif
