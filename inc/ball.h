#ifndef BALL_H
#define BALL_H

#include "movable_game_object_3D.h"

class Ball : public MovableGameObject3D
{
public:

   Ball(const std::shared_ptr<Model>& model,
        const glm::vec3&              position,
        float                         angleOfRotInDeg,
        const glm::vec3&              axisOfRot,
        float                         scalingFactor,
        const glm::vec3&              velocity,
        float                         radius,
        float                         spinAngularVelocity);
   ~Ball() = default;

   Ball(const Ball&) = default;
   Ball& operator=(const Ball&) = default;

   Ball(Ball&& rhs) noexcept;
   Ball& operator=(Ball&& rhs) noexcept;

   void      moveWithinVerticalRange(float deltaTime, float verticalRange);
   void      moveInFreeFall(float deltaTime);
   void      reset();

   glm::vec3 getInitialVelocity() const;

   float     getRadius() const;
   void      setRadius(float radius);

private:

   glm::vec3 mInitialVelocity;
   float     mRadius;
   float     mSpinAngularVelocity;
   float     mSpinAngularVelocityScaledByBounce;
};

#endif
