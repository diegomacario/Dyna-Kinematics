#include <utility>

#include "camera.h"

Camera::Camera(glm::vec3 position,
               glm::vec3 worldUp,
               float     yawInDeg,
               float     pitchInDeg,
               float     fieldOfViewYInDeg,
               float     aspectRatio,
               float     near,
               float     far,
               float     movementSpeed,
               float     mouseSensitivity)
   : mPosition(position)
   , mFront()
   , mUp()
   , mRight()
   , mWorldUp(worldUp)
   , mYawInDeg(yawInDeg)
   , mPitchInDeg(pitchInDeg)
   , mFieldOfViewYInDeg(fieldOfViewYInDeg)
   , mAspectRatio(aspectRatio)
   , mNear(near)
   , mFar(far)
   , mMovementSpeed(movementSpeed)
   , mMouseSensitivity(mouseSensitivity)
   , mIsFree(false)
   , mViewMatrix()
   , mPerspectiveProjectionMatrix()
   , mPerspectiveProjectionViewMatrix()
   , mNeedToUpdateViewMatrix(true)
   , mNeedToUpdatePerspectiveProjectionMatrix(true)
   , mNeedToUpdatePerspectiveProjectionViewMatrix(true)
{
   updateCoordinateFrame();
}

Camera::Camera(Camera&& rhs) noexcept
   : mPosition(std::exchange(rhs.mPosition, glm::vec3(0.0f)))
   , mFront(std::exchange(rhs.mFront, glm::vec3(0.0f)))
   , mUp(std::exchange(rhs.mUp, glm::vec3(0.0f)))
   , mRight(std::exchange(rhs.mRight, glm::vec3(0.0f)))
   , mWorldUp(std::exchange(rhs.mWorldUp, glm::vec3(0.0f)))
   , mYawInDeg(std::exchange(rhs.mYawInDeg, 0.0f))
   , mPitchInDeg(std::exchange(rhs.mPitchInDeg, 0.0f))
   , mFieldOfViewYInDeg(std::exchange(rhs.mFieldOfViewYInDeg, 0.0f))
   , mAspectRatio(std::exchange(rhs.mAspectRatio, 0.0f))
   , mNear(std::exchange(rhs.mNear, 0.0f))
   , mFar(std::exchange(rhs.mFar, 0.0f))
   , mMovementSpeed(std::exchange(rhs.mMovementSpeed, 0.0f))
   , mMouseSensitivity(std::exchange(rhs.mMouseSensitivity, 0.0f))
   , mIsFree(std::exchange(rhs.mIsFree, false))
   , mViewMatrix(std::exchange(rhs.mViewMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionMatrix(std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f)))
   , mPerspectiveProjectionViewMatrix(std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f)))
   , mNeedToUpdateViewMatrix(std::exchange(rhs.mNeedToUpdateViewMatrix, true))
   , mNeedToUpdatePerspectiveProjectionMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true))
   , mNeedToUpdatePerspectiveProjectionViewMatrix(std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true))
{

}

Camera& Camera::operator=(Camera&& rhs) noexcept
{
   mPosition                                    = std::exchange(rhs.mPosition, glm::vec3(0.0f));
   mFront                                       = std::exchange(rhs.mFront, glm::vec3(0.0f));
   mUp                                          = std::exchange(rhs.mUp, glm::vec3(0.0f));
   mRight                                       = std::exchange(rhs.mRight, glm::vec3(0.0f));
   mWorldUp                                     = std::exchange(rhs.mWorldUp, glm::vec3(0.0f));
   mYawInDeg                                    = std::exchange(rhs.mYawInDeg, 0.0f);
   mPitchInDeg                                  = std::exchange(rhs.mPitchInDeg, 0.0f);
   mFieldOfViewYInDeg                           = std::exchange(rhs.mFieldOfViewYInDeg, 0.0f);
   mAspectRatio                                 = std::exchange(rhs.mAspectRatio, 0.0f);
   mNear                                        = std::exchange(rhs.mNear, 0.0f);
   mFar                                         = std::exchange(rhs.mFar, 0.0f);
   mMovementSpeed                               = std::exchange(rhs.mMovementSpeed, 0.0f);
   mMouseSensitivity                            = std::exchange(rhs.mMouseSensitivity, 0.0f);
   mIsFree                                      = std::exchange(rhs.mIsFree, false);
   mViewMatrix                                  = std::exchange(rhs.mViewMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionMatrix                 = std::exchange(rhs.mPerspectiveProjectionMatrix, glm::mat4(0.0f));
   mPerspectiveProjectionViewMatrix             = std::exchange(rhs.mPerspectiveProjectionViewMatrix, glm::mat4(0.0f));
   mNeedToUpdateViewMatrix                      = std::exchange(rhs.mNeedToUpdateViewMatrix, true);
   mNeedToUpdatePerspectiveProjectionMatrix     = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionMatrix, true);
   mNeedToUpdatePerspectiveProjectionViewMatrix = std::exchange(rhs.mNeedToUpdatePerspectiveProjectionViewMatrix, true);
   return *this;
}

glm::vec3 Camera::getPosition()
{
   return mPosition;
}

glm::mat4 Camera::getViewMatrix()
{
   if (mNeedToUpdateViewMatrix)
   {
      mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
      mNeedToUpdateViewMatrix = false;
   }

   return mViewMatrix;
}

glm::mat4 Camera::getPerspectiveProjectionMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionMatrix)
   {
      mPerspectiveProjectionMatrix = glm::perspective(glm::radians(mFieldOfViewYInDeg),
                                                      mAspectRatio,
                                                      mNear,
                                                      mFar);
      mNeedToUpdatePerspectiveProjectionMatrix = false;
   }

   return mPerspectiveProjectionMatrix;
}

glm::mat4 Camera::getPerspectiveProjectionViewMatrix()
{
   if (mNeedToUpdatePerspectiveProjectionViewMatrix)
   {
      mPerspectiveProjectionViewMatrix = getPerspectiveProjectionMatrix() * getViewMatrix();
      mNeedToUpdatePerspectiveProjectionViewMatrix = false;
   }

   return mPerspectiveProjectionViewMatrix;
}

void Camera::reposition(const glm::vec3& position,
                        const glm::vec3& worldUp,
                        float            yawInDeg,
                        float            pitchInDeg,
                        float            fieldOfViewYInDeg)
{
   mPosition          = position;
   mWorldUp           = worldUp;
   mYawInDeg          = yawInDeg;
   mPitchInDeg        = pitchInDeg;
   mFieldOfViewYInDeg = fieldOfViewYInDeg;

   updateCoordinateFrame();

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processKeyboardInput(MovementDirection direction, float deltaTime)
{
   float velocity = mMovementSpeed * deltaTime;

   switch (direction)
   {
   case MovementDirection::Forward:
      mPosition += mFront * velocity;
      break;
   case MovementDirection::Backward:
      mPosition -= mFront * velocity;
      break;
   case MovementDirection::Left:
      mPosition -= mRight * velocity;
      break;
   case MovementDirection::Right:
      mPosition += mRight * velocity;
      break;
   }

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
   // TODO: Should the offsets also be scaled by deltaTime?
   xOffset *= mMouseSensitivity;
   yOffset *= mMouseSensitivity;

   mYawInDeg   += xOffset;
   mPitchInDeg += yOffset;

   // Make sure that when the pitch is out of bounds, the screen doesn't get flipped
   if (mPitchInDeg > 89.0f)
   {
      mPitchInDeg = 89.0f;
   }
   else if (mPitchInDeg < -89.0f)
   {
      mPitchInDeg = -89.0f;
   }

   // Update the front, right and up vectors using the updated Euler angles
   updateCoordinateFrame();

   mNeedToUpdateViewMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

void Camera::processScrollWheelMovement(float yOffset)
{
   // The larger the FOV, the smaller things appear on the screen
   // The smaller the FOV, the larger things appear on the screen
   if (mFieldOfViewYInDeg >= 1.0f && mFieldOfViewYInDeg <= 45.0f)
   {
      mFieldOfViewYInDeg -= yOffset;
   }
   else if (mFieldOfViewYInDeg < 1.0f)
   {
      mFieldOfViewYInDeg = 1.0f;
   }
   else if (mFieldOfViewYInDeg > 45.0f)
   {
      mFieldOfViewYInDeg = 45.0f;
   }

   mNeedToUpdatePerspectiveProjectionMatrix = true;
   mNeedToUpdatePerspectiveProjectionViewMatrix = true;
}

bool Camera::isFree() const
{
   return mIsFree;
}

void Camera::setFree(bool free)
{
   mIsFree = free;
}

void Camera::updateCoordinateFrame()
{
   // Calculate the new front vector
   glm::vec3 newFront;
   newFront.x = sin(glm::radians(mYawInDeg)) * cos(glm::radians(mPitchInDeg));
   newFront.y = sin(glm::radians(mPitchInDeg));
   newFront.z = -1.0f * cos(glm::radians(mYawInDeg)) * cos(glm::radians(mPitchInDeg));
   mFront = glm::normalize(newFront);

   // Calculate the new right and up vectors
   mRight = glm::normalize(glm::cross(mFront, mWorldUp));
   mUp    = glm::normalize(glm::cross(mRight, mFront));
}
