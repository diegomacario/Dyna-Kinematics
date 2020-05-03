#ifndef RIGID_BODY_2D_H
#define RIGID_BODY_2D_H

#include <glm/glm.hpp>

#include <array>

// The moment of inertia measures the extent to which an object resists rotational acceleration about a particular axis
// For a rectangle with the axis of rotation going through its center of mass, the moment of inertia is given by the following equation:
// I = (mass / 12.0f) * (width * width + height * height)

enum RigidBodyState
{
   current = 0,
   future  = 1,
};

class RigidBody2D
{
public:

   RigidBody2D(float     mass,
               float     width,
               float     height,
               float     coefficientOfRestitution,
               glm::vec2 positionOfCenterOfMass,
               float     orientation,
               glm::vec2 velocityOfCenterOfMass,
               float     angularVelocity,
               glm::vec3 color);

   void      calculateVertices(RigidBodyState state);
   glm::mat4 getModelMatrix(RigidBodyState state) const;

//private: // TODO: Decide what to do here

   float                                   mOneOverMass;
   float                                   mWidth;
   float                                   mHeight;
   float                                   mOneOverMomentOfInertia;
   float                                   mCoefficientOfRestitution;

   struct KinematicAndDynamicState
   {
      KinematicAndDynamicState();
      KinematicAndDynamicState(glm::vec2 positionOfCenterOfMass,
                               float     orientation,
                               glm::vec2 velocityOfCenterOfMass,
                               float     angularVelocity);

      glm::vec2                            positionOfCenterOfMass;
      float                                orientation;

      glm::vec2                            velocityOfCenterOfMass;
      float                                angularVelocity;

      glm::vec2                            forceOfCenterOfMass;
      float                                torque;

      std::array<glm::vec2, 4>             vertices;
   };

   std::array<KinematicAndDynamicState, 2> mStates;

   glm::vec3                               mColor;
};

#endif
