#include "menu_state.h"

float calculateCWAngularPosOnXYPlaneWRTNegYAxisInDeg(const glm::vec3& point);
float calculateAngularPosWRTPosZAxisInDeg(const glm::vec3& point);

MenuState::MenuState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                     const std::shared_ptr<Window>&             window,
                     const std::shared_ptr<Camera>&             camera,
                     const std::shared_ptr<Shader>&             gameObject3DShader,
                     const std::shared_ptr<GameObject3D>&       title,
                     const std::shared_ptr<GameObject3D>&       table,
                     const std::shared_ptr<Paddle>&             leftPaddle,
                     const std::shared_ptr<Paddle>&             rightPaddle,
                     const std::shared_ptr<Ball>&               ball)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mGameObject3DShader(gameObject3DShader)
   , mTitle(title)
   , mTable(table)
   , mLeftPaddle(leftPaddle)
   , mRightPaddle(rightPaddle)
   , mBall(ball)
   , mCameraPosition(0.0f, -30.0f, 10.0f)
   , mCameraTarget(0.0f, 0.0f, 5.0f)
   , mCameraUp(0.0f, 0.0f, 1.0f)
   , mCameraRight(0.0f)
   , mIdleOrbitalAngularVelocity(-15.0f)
   , mTransitionToPlayState(false)
   , mFirstIterationOfTransitionToPlayState(false)
   , mTimeToCompleteTransitionToPlayStateInSec(7.5f)
   , mHorizontalAngularSpeed(0.0f)
   , mVerticalAngularSpeed(0.0f)
   , mSpeedOfMovementAwayFromTarget(0.0f)
   , mSpeedOfShrink(0.0f)
   , mDoneRotatingHorizontally(false)
   , mDoneRotatingVertically(false)
   , mDoneMovingAwayFromTarget(false)
   , mDoneShrinking(false)
{

}

void MenuState::enter()
{
   // In the menu state, the cursor is disabled when fullscreen and enabled when windowed
   mWindow->enableCursor(!mWindow->isFullScreen());

   mTitle->setRotationMatrix(glm::mat4(1.0f));
   mTitle->rotate(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));

   mBall->reset();
   mBall->scale(7.5f / mBall->getRadius());
   mBall->setPosition(glm::vec3(0.0f, 0.0f, mBall->getScalingFactor() * 1.96875));
   mBall->setRadius(7.5f);

   mCameraPosition = glm::vec3(0.0f, -30.0f, 10.0f);
   mCameraTarget   = glm::vec3(0.0f, 0.0f, 5.0f);
   mCameraUp       = glm::vec3(0.0f, 0.0f, 1.0f);
   mCameraRight    = glm::vec3(0.0f);

   mTransitionToPlayState                 = false;
   mFirstIterationOfTransitionToPlayState = false;

   mHorizontalAngularSpeed        = 0.0f;
   mVerticalAngularSpeed          = 0.0f;
   mSpeedOfMovementAwayFromTarget = 0.0f;
   mSpeedOfShrink                 = 0.0f;

   mDoneRotatingHorizontally = false;
   mDoneRotatingVertically   = false;
   mDoneMovingAwayFromTarget = false;
   mDoneShrinking            = false;
}

void MenuState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());
      mWindow->enableCursor(!mWindow->isFullScreen());
   }

   // Change the number of samples used for anti aliasing
   if (mWindow->keyIsPressed(GLFW_KEY_1) && !mWindow->keyHasBeenProcessed(GLFW_KEY_1))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_1);
      mWindow->setNumberOfSamples(1);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_2) && !mWindow->keyHasBeenProcessed(GLFW_KEY_2))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_2);
      mWindow->setNumberOfSamples(2);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_4) && !mWindow->keyHasBeenProcessed(GLFW_KEY_4))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_4);
      mWindow->setNumberOfSamples(4);
   }
   else if (mWindow->keyIsPressed(GLFW_KEY_8) && !mWindow->keyHasBeenProcessed(GLFW_KEY_8))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_8);
      mWindow->setNumberOfSamples(8);
   }

   if (!mTransitionToPlayState && mWindow->keyIsPressed(GLFW_KEY_SPACE))
   {
      mFirstIterationOfTransitionToPlayState = true;
      mTransitionToPlayState = true;
   }
}

void MenuState::update(float deltaTime)
{
   if (mTransitionToPlayState)
   {
      if (mFirstIterationOfTransitionToPlayState)
      {
         calculateAngularAndMovementSpeeds();
         mFirstIterationOfTransitionToPlayState = false;
      }

      if (!mDoneRotatingHorizontally) { rotateCameraHorizontally(deltaTime); }
      if (!mDoneRotatingVertically)   { rotateCameraVertically(deltaTime); }
      if (!mDoneMovingAwayFromTarget) { moveCameraAwayFromTarget(deltaTime); }
      if (!mDoneShrinking)            { shrinkBall(deltaTime); }

      updateCoordinateFrameOfCamera();

      if (mDoneRotatingHorizontally && mDoneRotatingVertically && mDoneMovingAwayFromTarget && mDoneShrinking)
      {
         mFSM->changeState("play");
         return;
      }
   }
   else
   {
      // Rotate the camera CW around the positive Z axis
      glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mIdleOrbitalAngularVelocity * deltaTime), glm::vec3(0.0f, 0.0f, 1.0f));
      mCameraPosition = glm::mat3(rotationMatrix) * mCameraPosition;

      // Rotate the title CW around the positive Z axis
      mTitle->rotate(mIdleOrbitalAngularVelocity * deltaTime, glm::vec3(0.0f, 0.0f, 1.0f));
   }
}

void MenuState::render()
{
   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mGameObject3DShader->use();
   mGameObject3DShader->setMat4("projectionView", mCamera->getPerspectiveProjectionMatrix() * glm::lookAt(mCameraPosition, mCameraTarget, mCameraUp));
   mGameObject3DShader->setVec3("cameraPos", mCameraPosition);

   if (!mTransitionToPlayState)
   {
      mTitle->render(*mGameObject3DShader);
   }

   mTable->render(*mGameObject3DShader);

   mLeftPaddle->render(*mGameObject3DShader);
   mRightPaddle->render(*mGameObject3DShader);

   // Disable face culling so that we render the inside of the teapot
   glDisable(GL_CULL_FACE);
   mBall->render(*mGameObject3DShader);
   glEnable(GL_CULL_FACE);

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void MenuState::exit()
{

}

void MenuState::calculateAngularAndMovementSpeeds()
{
   float cameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg = calculateCWAngularPosOnXYPlaneWRTNegYAxisInDeg(mCameraPosition);
   float cameraAngularPosWRTPosZAxisInDeg            = calculateAngularPosWRTPosZAxisInDeg(mCameraPosition);

   mHorizontalAngularSpeed        = -(360.0f - cameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg) / mTimeToCompleteTransitionToPlayStateInSec;
   mVerticalAngularSpeed          = cameraAngularPosWRTPosZAxisInDeg / mTimeToCompleteTransitionToPlayStateInSec;
   mSpeedOfMovementAwayFromTarget = (90.0f - glm::length(mCameraPosition - mCameraTarget)) / mTimeToCompleteTransitionToPlayStateInSec;
   mSpeedOfShrink                 = (mBall->getRadius() - 2.5f) / mTimeToCompleteTransitionToPlayStateInSec;
}

void MenuState::rotateCameraHorizontally(float deltaTime)
{
   glm::mat3 horizontalRotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(mHorizontalAngularSpeed * deltaTime), glm::vec3(0.0f, 0.0f, 1.0f)));

   float     cameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg       = calculateCWAngularPosOnXYPlaneWRTNegYAxisInDeg(mCameraPosition);
   glm::vec3 futureCameraPos                                   = horizontalRotationMatrix * mCameraPosition;
   float     futureCameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg = calculateCWAngularPosOnXYPlaneWRTNegYAxisInDeg(futureCameraPos);

   if (futureCameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg < cameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg)
   {
      // If we rotate by what is dictated by (mHorizontalSpeedOfRotation * deltaTime),
      // we would overshoot the desired destination (e.g. we might be rotating from 359.0f to 1.0f instead of from 359.0f to 0.0f)
      // So we modify the rotation matrix so that it only makes us rotate the exact number of degrees left to reach 0.0f
      horizontalRotationMatrix  = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(-(360.0f - cameraCWAngularPosOnXYPlaneWRTNegYAxisInDeg)), glm::vec3(0.0f, 0.0f, 1.0f)));
      mDoneRotatingHorizontally = true;
   }

   mCameraPosition = horizontalRotationMatrix * mCameraPosition;
   mCameraUp       = horizontalRotationMatrix * mCameraUp;
}

void MenuState::rotateCameraVertically(float deltaTime)
{
   updateCoordinateFrameOfCamera();

   glm::mat3 verticalRotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(mVerticalAngularSpeed * deltaTime), mCameraRight));

   float     cameraAngularPosWRTPosZAxisInDeg        = calculateAngularPosWRTPosZAxisInDeg(mCameraPosition);
   glm::vec3 futureCameraPos1                        = verticalRotationMatrix * mCameraPosition;
   glm::vec3 futureCameraPos2                        = verticalRotationMatrix * futureCameraPos1;
   float     futureCameraAngularPosWRTPosZAxisInDeg1 = calculateAngularPosWRTPosZAxisInDeg(futureCameraPos1);
   float     futureCameraAngularPosWRTPosZAxisInDeg2 = calculateAngularPosWRTPosZAxisInDeg(futureCameraPos2);

   if (futureCameraAngularPosWRTPosZAxisInDeg2 > futureCameraAngularPosWRTPosZAxisInDeg1)
   {
      // If we rotate by what is dictated by (mVerticalSpeedOfRotation * deltaTime),
      // we would overshoot the desired destination (e.g. we might be rotating from 0.1f to 0.2f instead of from 0.1f to 0.0f)
      // So we modify the rotation matrix so that it only makes us rotate the exact number of degrees left to reach 0.0f
      verticalRotationMatrix  = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(cameraAngularPosWRTPosZAxisInDeg), mCameraRight));
      mDoneRotatingVertically = true;
   }

   mCameraPosition = verticalRotationMatrix * mCameraPosition;
   mCameraUp       = verticalRotationMatrix * mCameraUp;
}

void MenuState::moveCameraAwayFromTarget(float deltaTime)
{
   glm::vec3 distanceToMoveAwayFromTarget = glm::normalize(mCameraPosition - mCameraTarget) * (mSpeedOfMovementAwayFromTarget * deltaTime);
   glm::vec3 futureCameraPos              = mCameraPosition + distanceToMoveAwayFromTarget;

   if (glm::length(futureCameraPos - mCameraTarget) > 90.0f)
   {
      // If we move by what is dictated by (mSpeedOfMovementAwayFromTarget * deltaTime),
      // we would overshoot the desired destination (e.g. the length of (mCameraPosition - mCameraTarget) might be increasing from 89.0f to 91.0f instead of from 89.0f to 90.0f)
      // So we modify the length we move so that it only makes us move the exact distance left to reach 90.0f
      distanceToMoveAwayFromTarget = glm::normalize(mCameraPosition - mCameraTarget) * (90.0f - glm::length(mCameraPosition - mCameraTarget));
      mDoneMovingAwayFromTarget = true;
   }

   mCameraPosition += distanceToMoveAwayFromTarget;
}

void MenuState::shrinkBall(float deltaTime)
{
   float amountToDecreaseRadiusBy = (mSpeedOfShrink * deltaTime);
   float currentBallRadius        = mBall->getRadius();
   float futureBallRadius         = currentBallRadius - amountToDecreaseRadiusBy;

   if (futureBallRadius < 2.5f)
   {
      // If we decrease the radius by what is dictated by (mSpeedOfShrink * deltaTime),
      // we would overshoot the desired radius (e.g. the radius might be increasing from 2.6f to 2.4f instead of from 2.6f to 2.5f)
      // So we modify the amount we decrease the radius by so that it only makes us decrease the exact amount left to reach 2.5f
      amountToDecreaseRadiusBy = currentBallRadius - 2.5f;
      mDoneShrinking = true;
   }

   float newRadius = currentBallRadius - amountToDecreaseRadiusBy;
   mBall->scale(newRadius / currentBallRadius);
   mBall->setPosition(glm::vec3(0.0f, 0.0f, mBall->getScalingFactor() * 1.96875));
   mBall->setRadius(newRadius);
}

void MenuState::updateCoordinateFrameOfCamera()
{
   glm::vec3 vecFromCameraPositionToCameraTarget = glm::normalize(mCameraTarget - mCameraPosition);

   mCameraRight = glm::normalize(glm::cross(mCameraUp, vecFromCameraPositionToCameraTarget));
   mCameraUp    = glm::normalize(glm::cross(vecFromCameraPositionToCameraTarget, mCameraRight));
}

float calculateCWAngularPosOnXYPlaneWRTNegYAxisInDeg(const glm::vec3& point)
{
   float result = 0.0f;

   // The math of the first two if statements must be wrong
   if ((point.x < 0.0f && point.y < 0.0f) || (point.x < 0.0f && point.y > 0.0f)) // Lower left quadrant or upper left quadrant
   {
      result = glm::degrees(glm::acos(glm::dot(glm::normalize(glm::vec3(point.x, point.y, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f))));
   }
   else if ((point.x > 0.0f && point.y > 0.0f) || (point.x > 0.0f && point.y < 0.0f)) // Upper right quadrant or lower right quadrant
   {
      result = 360.0f - glm::degrees(glm::acos(glm::dot(glm::normalize(glm::vec3(point.x, point.y, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f))));
   }
   else if (point.x == 0.0f && point.y > 0.0f) // Aligned with the positive Y axis
   {
      result = 180.0f;
   }
   else if (point.x == 0.0f && point.y < 0.0f) // Aligned with the negative Y axis
   {
      result = 0.0f;
   }
   else if (point.x > 0.0f && point.y == 0.0f) // Aligned with the positive X axis
   {
      result = 270.0f;
   }
   else if (point.x < 0.0f && point.y == 0.0f) // Aligned with the negative X axis
   {
      result = 90.0f;
   }

   return result;
}

float calculateAngularPosWRTPosZAxisInDeg(const glm::vec3& point)
{
   float result = 0.0f;

   if (point.x != 0.0f || point.y != 0.0f)
   {
      result = glm::degrees(glm::acos(glm::dot(glm::normalize(point), glm::vec3(0.0f, 0.0f, 1.0f))));
   }
   else if (point.z > 0.0f) // Aligned with the positive Z axis
   {
      result = 0.0f;
   }
   else if (point.z < 0.0f) // Aligned with the negative Z axis
   {
      result = 180.0f;
   }

   return result;
}
