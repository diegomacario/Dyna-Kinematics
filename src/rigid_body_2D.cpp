#include <glm/gtc/matrix_transform.hpp>

#include "rigid_body_2D.h"

RigidBody2D::RigidBody2D(float mass,
                         float width,
                         float height,
                         float coefficientOfRestitution)
   : mMass(mass)
   , mWidth(width)
   , mHeight(height)
   , mMomentOfInertia((mass / 12.0f) * (width * width + height * height))
   , mCoefficientOfRestitution(coefficientOfRestitution)
{

}

glm::mat4 RigidBody2D::getModelMatrix() const
{
   // 3) Translate the quad
   glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(mCurrentState.positionOfCenterOfMass, 0.0f));

   // 2) Rotate the quad around the Z axis
   modelMatrix = glm::rotate(modelMatrix, mCurrentState.orientation, glm::vec3(0.0f, 0.0f, 1.0f)); // TODO: Maybe we'll need to convert orientation to degrees

   // 1) Scale the quad
   modelMatrix = glm::scale(modelMatrix, glm::vec3(mWidth, mHeight, 1.0f));

   return modelMatrix;
}
