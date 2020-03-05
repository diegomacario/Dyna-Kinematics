#include "play_state.h"
#include "pause_state.h"

   PauseState::PauseState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                          const std::shared_ptr<Window>&             window,
                          const std::shared_ptr<Camera>&             camera,
                          const std::shared_ptr<Shader>&             gameObject3DShader,
                          const std::shared_ptr<GameObject3D>&       table,
                          const std::shared_ptr<Paddle>&             leftPaddle,
                          const std::shared_ptr<Paddle>&             rightPaddle,
                          const std::shared_ptr<Ball>&               ball,
                          const std::shared_ptr<GameObject3D>&       point)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mGameObject3DShader(gameObject3DShader)
   , mTable(table)
   , mLeftPaddle(leftPaddle)
   , mRightPaddle(rightPaddle)
   , mBall(ball)
   , mPoint(point)
   , mPointsScoredByLeftPaddle(0)
   , mPointsScoredByRightPaddle(0)
   , mPositionsOfPointsScoredByLeftPaddle({glm::vec3(-47.5f, -34.0f, 0.0f),
                                           glm::vec3(-43.5f, -34.0f, 0.0f),
                                           glm::vec3(-39.5f, -34.0f, 0.0f)})
   , mPositionsOfPointsScoredByRightPaddle({glm::vec3(47.5f, -34.0f, 0.0f),
                                            glm::vec3(43.5f, -34.0f, 0.0f),
                                            glm::vec3(39.5f, -34.0f, 0.0f)})
{

}

void PauseState::enter()
{
   const PlayState& playState = dynamic_cast<PlayState&>(*mFSM->getPreviousState());
   mPointsScoredByLeftPaddle  = playState.getPointsScoredByLeftPaddle();
   mPointsScoredByRightPaddle = playState.getPointsScoredByRightPaddle();
}

void PauseState::processInput(float deltaTime)
{
   // Close the game
   if (mWindow->keyIsPressed(GLFW_KEY_ESCAPE)) { mWindow->setShouldClose(true); }

   // Make the game full screen or windowed
   if (mWindow->keyIsPressed(GLFW_KEY_F) && !mWindow->keyHasBeenProcessed(GLFW_KEY_F))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_F);
      mWindow->setFullScreen(!mWindow->isFullScreen());

      // In the pause state, the following rules are applied to the cursor:
      // - Fullscreen: Cursor is always disabled
      // - Windowed with a free camera: Cursor is disabled
      // - Windowed with a fixed camera: Cursor is enabled
      if (mWindow->isFullScreen())
      {
         // Disable the cursor when fullscreen
         mWindow->enableCursor(false);
         if (mCamera->isFree())
         {
            // Going from windowed to fullscreen changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
      }
      else if (!mWindow->isFullScreen())
      {
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
            // Going from fullscreen to windowed changes the position of the cursor, so we reset the first move flag to avoid a jump
            mWindow->resetFirstMove();
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      }
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

   // Unpause the game
   if (mWindow->keyIsPressed(GLFW_KEY_P) && !mWindow->keyHasBeenProcessed(GLFW_KEY_P))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_P);
      mFSM->changeState("play");
   }

   // Reset the camera
   if (mWindow->keyIsPressed(GLFW_KEY_R)) { resetCamera(); }

   // Make the camera free or fixed
   if (mWindow->keyIsPressed(GLFW_KEY_C) && !mWindow->keyHasBeenProcessed(GLFW_KEY_C))
   {
      mWindow->setKeyAsProcessed(GLFW_KEY_C);
      mCamera->setFree(!mCamera->isFree());

      if (!mWindow->isFullScreen())
      {
         if (mCamera->isFree())
         {
            // Disable the cursor when windowed with a free camera
            mWindow->enableCursor(false);
         }
         else
         {
            // Enable the cursor when windowed with a fixed camera
            mWindow->enableCursor(true);
         }
      }

      mWindow->resetMouseMoved();
   }

   // Move and orient the camera
   if (mCamera->isFree())
   {
      // Move
      if (mWindow->keyIsPressed(GLFW_KEY_W)) { mCamera->processKeyboardInput(Camera::MovementDirection::Forward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_S)) { mCamera->processKeyboardInput(Camera::MovementDirection::Backward, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_A)) { mCamera->processKeyboardInput(Camera::MovementDirection::Left, deltaTime); }
      if (mWindow->keyIsPressed(GLFW_KEY_D)) { mCamera->processKeyboardInput(Camera::MovementDirection::Right, deltaTime); }

      // Orient
      if (mWindow->mouseMoved())
      {
         mCamera->processMouseMovement(mWindow->getCursorXOffset(), mWindow->getCursorYOffset());
         mWindow->resetMouseMoved();
      }

      // Zoom
      if (mWindow->scrollWheelMoved())
      {
         mCamera->processScrollWheelMovement(mWindow->getScrollYOffset());
         mWindow->resetScrollWheelMoved();
      }
   }

   mWindow->pollEvents();
}

void PauseState::update(float /*deltaTime*/)
{

}

void PauseState::render()
{
   mWindow->clearAndBindMultisampleFramebuffer();

   // Enable depth testing for 3D objects
   glEnable(GL_DEPTH_TEST);

   mGameObject3DShader->use();
   mGameObject3DShader->setMat4("projectionView", mCamera->getPerspectiveProjectionViewMatrix());
   mGameObject3DShader->setVec3("cameraPos", mCamera->getPosition());

   mTable->render(*mGameObject3DShader);

   mLeftPaddle->render(*mGameObject3DShader);
   mRightPaddle->render(*mGameObject3DShader);

   // Disable face culling so that we render the inside of the teapot
   glDisable(GL_CULL_FACE);
   mBall->render(*mGameObject3DShader);
   glEnable(GL_CULL_FACE);

   displayScore();

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void PauseState::exit()
{

}

void PauseState::resetCamera()
{
   mCamera->reposition(glm::vec3(0.0f, 0.0f, 95.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f),
                       0.0f,
                       0.0f,
                       45.0f);
}

void PauseState::displayScore()
{
   for (unsigned int i = 0; i < mPointsScoredByLeftPaddle; ++i)
   {
      mPoint->setPosition(mPositionsOfPointsScoredByLeftPaddle[i]);
      mPoint->render(*mGameObject3DShader);
   }

   for (unsigned int i = 0; i < mPointsScoredByRightPaddle; ++i)
   {
      mPoint->setPosition(mPositionsOfPointsScoredByRightPaddle[i]);
      mPoint->render(*mGameObject3DShader);
   }
}
