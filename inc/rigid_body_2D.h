#ifndef RIGID_BODY_2D_H
#define RIGID_BODY_2D_H

#include <glm/glm.hpp>

#include <array>

// The moment of inertia measures the extent to which an object resists rotational acceleration about a particular axis
// For a rectangle with the axis of rotation going through its center of mass, the moment of inertia is given by the following equation:
// I = (mass / 12.0f) * (width * width + height * height)

class RigidBody2D
{
public:

   RigidBody2D(float mass,
               float width,
               float height,
               float coefficientOfRestitution);

private:

   float                       mMass;
   float                       mMomentOfInertia;
   float                       mCoefficientOfRestitution;

   struct KinematicAndDynamicState
   {
      glm::vec2                positionOfCenterOfMass;
      float                    orientation;

      glm::vec2                velocityOfCenterOfMass;
      float                    angularVelocity;

      glm::vec2                forceOfCenterOfMass;
      float                    torque;

      std::array<glm::vec2, 4> vertices;
   };

   KinematicAndDynamicState    mCurrentState;
   KinematicAndDynamicState    mFutureState;
};

#endif
