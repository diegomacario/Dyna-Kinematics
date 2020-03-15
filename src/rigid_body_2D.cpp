#include "rigid_body_2D.h"

RigidBody2D::RigidBody2D(float mass,
                         float width,
                         float height,
                         float coefficientOfRestitution)
   : mMass(mass)
   , mMomentOfInertia((mass / 12.0f) * (width * width + height * height))
   , mCoefficientOfRestitution(coefficientOfRestitution)
   , mPositionOfCenterOfMass(glm::vec2(0.0f))
   , mOrientation(0.0f)
   , mVelocityOfCenterOfMass(glm::vec2(0.0f))
   , mAngularVelocity(0.0f)
   , mForceOfCenterOfMass(glm::vec2(0.0f))
   , mTorque(0.0f)
{

}
