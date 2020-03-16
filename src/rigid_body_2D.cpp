#include "rigid_body_2D.h"

RigidBody2D::RigidBody2D(float mass,
                         float width,
                         float height,
                         float coefficientOfRestitution)
   : mMass(mass)
   , mMomentOfInertia((mass / 12.0f) * (width * width + height * height))
   , mCoefficientOfRestitution(coefficientOfRestitution)
{

}
