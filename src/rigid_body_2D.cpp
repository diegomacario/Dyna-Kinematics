#include <glm/gtc/matrix_transform.hpp>

#include "rigid_body_2D.h"

RigidBody2D::RigidBody2D(float     mass,
                         float     width,
                         float     height,
                         float     coefficientOfRestitution,
                         glm::vec2 positionOfCenterOfMass,
                         float     orientation,
                         glm::vec2 velocityOfCenterOfMass,
                         float     angularVelocity)
   : mOneOverMass(1.0f / mass)
   , mWidth(width)
   , mHeight(height)
   , mOneOverMomentOfInertia(1.0f / ((mass / 12.0f) * ((width * width) + (height * height))))
   , mCoefficientOfRestitution(coefficientOfRestitution)
   , mStates({KinematicAndDynamicState(positionOfCenterOfMass, orientation, velocityOfCenterOfMass, angularVelocity), KinematicAndDynamicState()})
{
   calculateVertices(current);
}

void RigidBody2D::calculateVertices(RigidBodyState state)
{
   glm::vec2 translation = mStates[state].positionOfCenterOfMass;

   // A GLM matrix is indexed in the following way: matrix[col][row]
   glm::mat2 rotation;
   float cosVal   = cos(mStates[state].orientation);
   float sinVal   = sin(mStates[state].orientation);
   rotation[0][0] =  cosVal; // Top left
   rotation[1][0] = -sinVal; // Top right
   rotation[0][1] =  sinVal; // Bottom left
   rotation[1][1] =  cosVal; // Bottom right

   float halfWidth  = mWidth / 2.0f;
   float halfHeight = mHeight / 2.0f;

   mStates[state].vertices[0] = translation + (rotation * glm::vec2( halfWidth,  halfHeight));
   mStates[state].vertices[1] = translation + (rotation * glm::vec2( halfWidth, -halfHeight));
   mStates[state].vertices[2] = translation + (rotation * glm::vec2(-halfWidth, -halfHeight));
   mStates[state].vertices[3] = translation + (rotation * glm::vec2(-halfWidth,  halfHeight));
}

glm::mat4 RigidBody2D::getModelMatrix(RigidBodyState state) const
{
   // 3) Translate the quad
   glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(mStates[state].positionOfCenterOfMass, 0.0f));

   // 2) Rotate the quad around the Z axis
   modelMatrix = glm::rotate(modelMatrix, mStates[state].orientation, glm::vec3(0.0f, 0.0f, 1.0f));

   // 1) Scale the quad
   modelMatrix = glm::scale(modelMatrix, glm::vec3(mWidth, mHeight, 1.0f));

   return modelMatrix;
}

RigidBody2D::KinematicAndDynamicState::KinematicAndDynamicState()
   : positionOfCenterOfMass(glm::vec2(0.0f))
   , orientation(0.0f)
   , velocityOfCenterOfMass(glm::vec2(0.0f))
   , angularVelocity(0.0f)
   , forceOfCenterOfMass(glm::vec2(0.0f))
   , torque(0.0f)
   , vertices()
{

}

RigidBody2D::KinematicAndDynamicState::KinematicAndDynamicState(glm::vec2 positionOfCenterOfMass,
                                                                float     orientation,
                                                                glm::vec2 velocityOfCenterOfMass,
                                                                float     angularVelocity)
   : positionOfCenterOfMass(positionOfCenterOfMass)
   , orientation(orientation)
   , velocityOfCenterOfMass(velocityOfCenterOfMass)
   , angularVelocity(angularVelocity)
   , forceOfCenterOfMass(glm::vec2(0.0f))
   , torque(0.0f)
   , vertices()
{

}
