#include "menu_state.h"

MenuState::MenuState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                     const std::shared_ptr<Window>&             window,
                     const std::shared_ptr<Camera>&             camera,
                     const std::shared_ptr<Renderer2D>&         renderer2D,
                     const std::shared_ptr<World>&              world)
   : mChangeScene(false)
   , mCurrentSceneDimensions()
   , mResetMemoryFramebuffer(false)
   , mPauseRememberFrames(false)
   , mWireframeModeIsEnabled(true)
   , mRememberFramesIsEnabled(false)
   , mRememberFramesFrequency(25)
   , mRememberFramesStatusChanged(false)
   , mFrameCounter(1)
   , mAntiAliasingIsEnabled(false)
   , mAntiAliasingMode(0)
   , mAntiAliasingStatusChanged(false)
   , mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mRenderer2D(renderer2D)
   , mWorld(world)
{

}

void MenuState::enter()
{
   // In the menu state, the cursor is disabled when fullscreen and enabled when windowed
   mWindow->enableCursor(!mWindow->isFullScreen());
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

   if (mResetMemoryFramebuffer)
   {
      mWindow->clearMemoryFramebuffer();
      mResetMemoryFramebuffer = false;
   }

   if (mChangeScene)
   {
      mWindow->clearMultisampleFramebuffer();
      mWindow->generateAntiAliasedImage();
      mWindow->swapBuffers();
      mWindow->generateAntiAliasedImage();
      mWindow->swapBuffers();

      mWindow->setSizeLimits(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y);
      mWindow->setSceneLimits(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y);
      mWindow->updateBufferAndViewportSizes();

      mRenderer2D->updateOrthographicProjection(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y);

      mChangeScene = false;
   }

   if (mWireframeModeIsEnabled)
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   }
   else
   {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   }

   if (mRememberFramesStatusChanged)
   {
      mWindow->clearAndBindMemoryFramebuffer();
      mRememberFramesStatusChanged = false;
   }

   if (mAntiAliasingStatusChanged)
   {
      if (mAntiAliasingIsEnabled)
      {
         switch (mAntiAliasingMode)
         {
         case 0: mWindow->setNumberOfSamples(2); break;
         case 1: mWindow->setNumberOfSamples(4); break;
         case 2: mWindow->setNumberOfSamples(8); break;
         }
      }
      else
      {
         mWindow->setNumberOfSamples(1);
      }

      mWindow->clearMemoryFramebuffer();
      mWindow->clearMultisampleFramebuffer();

      mAntiAliasingStatusChanged = false;
   }
}

int MenuState::update(float deltaTime)
{
   return mWorld->simulate(deltaTime);
}

void MenuState::render()
{
   if (mRememberFramesIsEnabled)
   {
      if (mFrameCounter % mRememberFramesFrequency == 0 && !mPauseRememberFrames)
      {
         mWindow->bindMemoryFramebuffer();
      }
      else
      {
         mWindow->clearAndBindMultisampleFramebuffer();
      }

      // Use the shader and set uniforms

      // Render objects

      mWindow->copyMemoryFramebufferIntoMultisampleFramebuffer();

      if (mFrameCounter % mRememberFramesFrequency == 0 && !mPauseRememberFrames)
      {
         mWindow->bindMemoryFramebuffer();
      }
      else
      {
         mWindow->bindMultisampleFramebuffer();
      }

      mWorld->render(*mRenderer2D, mWireframeModeIsEnabled);

      if (mFrameCounter % mRememberFramesFrequency == 0 && !mPauseRememberFrames)
      {
         mWindow->copyMemoryFramebufferIntoMultisampleFramebuffer();
      }

      if (!mPauseRememberFrames)
      {
         mFrameCounter++;
         if (mFrameCounter == 10000)
         {
            mFrameCounter = 1;
         }
      }
   }
   else
   {
      mWindow->clearAndBindMultisampleFramebuffer();

      // Use the shader and set uniforms

      // Render objects

      mWorld->render(*mRenderer2D, mWireframeModeIsEnabled);
   }

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void MenuState::exit()
{

}

void MenuState::changeScene(const glm::vec2& sceneDimensions)
{
   mCurrentSceneDimensions = sceneDimensions;
   mChangeScene = true;
}

void MenuState::resetMemoryFramebuffer()
{
   mResetMemoryFramebuffer = true;
   mFrameCounter = 1;
}

void MenuState::pauseRememberFrames(bool pause)
{
   mPauseRememberFrames = pause;
}

void MenuState::enableWireframeMode(bool enable)
{
   mWireframeModeIsEnabled = enable;
}

void MenuState::enableRememberFrames(bool enable)
{
   mRememberFramesIsEnabled = enable;
   mRememberFramesStatusChanged = true;
   mFrameCounter = 1;
}

void MenuState::changeRememberFramesFrequency(int frequency)
{
   mRememberFramesFrequency = frequency;
   mFrameCounter = 1;
}

void MenuState::enableAntiAliasing(bool enable)
{
   mAntiAliasingIsEnabled = enable;
   mAntiAliasingStatusChanged = true;
   mFrameCounter = 1;
}

void MenuState::changeAntiAliasingMode(int index)
{
   if (mAntiAliasingMode != index)
   {
      mAntiAliasingMode = index;
      if (mAntiAliasingIsEnabled)
      {
         mAntiAliasingStatusChanged = true;
         mFrameCounter = 1;
      }
   }
}
