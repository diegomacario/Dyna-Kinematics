#include <sys/stat.h>
#include <errno.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <direct.h>
#endif

#include <stb_image_write.h>

#include "menu_state.h"

MenuState::MenuState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                     const std::shared_ptr<Window>&             window,
                     const std::shared_ptr<Renderer2D>&         renderer2D,
                     const std::shared_ptr<World>&              world)
   : mChangeScene(false)
   , mCurrentSceneDimensions(glm::vec2(450.0f, 450.0f))
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
   , mRecord(false)
   , mRecordingDirectory(-1)
   , mRecordedFrameCounter(0)
   , mRecordedFrameData(nullptr)
   , mFSM(finiteStateMachine)
   , mWindow(window)
   , mRenderer2D(renderer2D)
   , mWorld(world)
{

}

MenuState::~MenuState()
{
   delete [] mRecordedFrameData;
}

void MenuState::enter()
{

}

void MenuState::processInput(float deltaTime)
{
   glViewport(0, 0, mWindow->getWidthOfFramebufferInPix(), mWindow->getHeightOfFramebufferInPix());
   if (mWindow->sizeChanged())
   {
      std::lock_guard<std::mutex> guard(mWindow->mMutex);
      mWindow->resetSizeChanged();
      mWindow->updateBufferAndViewportSizes();
      float aspectRatio = ((float) mWindow->getWidthOfFramebufferInPix()) / mWindow->getHeightOfFramebufferInPix();
      mRenderer2D->updateOrthographicProjection(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y, aspectRatio);
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

      mWindow->setSceneLimits(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y);
      mWindow->updateBufferAndViewportSizes();

      float aspectRatio = ((float) mWindow->getWidthOfFramebufferInPix()) / mWindow->getHeightOfFramebufferInPix();
      mRenderer2D->updateSceneDimensions(mCurrentSceneDimensions);
      mRenderer2D->updateScaleFactor(mWindow->getScaleFactor());
      mRenderer2D->updateOrthographicProjection(mCurrentSceneDimensions.x, mCurrentSceneDimensions.y, aspectRatio);

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

   if (mRecord && mRecordedFrameData)
   {
      mWindow->copyMultisampleFramebufferIntoGifFramebuffer();

      stbi_flip_vertically_on_write(true);

      memset(mRecordedFrameData, 0, 3 * mCurrentSceneDimensions.x * mCurrentSceneDimensions.y);

      mWindow->bindGifFramebuffer();

      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadPixels(0, 0, mCurrentSceneDimensions.x, mCurrentSceneDimensions.y, GL_RGB, GL_UNSIGNED_BYTE, mRecordedFrameData);

      std::string imgName = "GIFs\\GIF_" + std::to_string(mRecordingDirectory) + "\\Frames\\" + std::to_string(mRecordedFrameCounter) + ".png";

      stbi_write_png(imgName.c_str(), mCurrentSceneDimensions.x, mCurrentSceneDimensions.y, 3, mRecordedFrameData, mCurrentSceneDimensions.x * 3);
      mRecordedFrameCounter++;
   }

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   //mWindow->pollEvents();
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

void MenuState::enableRecording(bool enable)
{
   if (enable)
   {
      mRecordedFrameCounter = 0;
      mRecordingDirectory++;

      delete [] mRecordedFrameData;
      mRecordedFrameData = nullptr;
      mRecordedFrameData = new GLubyte[3 * mCurrentSceneDimensions.x * mCurrentSceneDimensions.y];

      std::string gifsDirectory = "GIFs";
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
      _mkdir(gifsDirectory.c_str());
#else
      mkdir(gifsDirectory.c_str() , 0775);
#endif

      std::string gifFilePath = gifsDirectory + "\\GIF_" + std::to_string(mRecordingDirectory);
      errno = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
      _mkdir(gifFilePath.c_str());
#else
      mkdir(gifFilePath.c_str() , 0775);
#endif
      while (errno == EEXIST)
      {
         mRecordingDirectory++;
         gifFilePath = "GIFs\\GIF_" + std::to_string(mRecordingDirectory);
         errno = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
         _mkdir(gifFilePath.c_str());
#else
         mkdir(gifFilePath.c_str() , 0775);
#endif
      }

      std::string framesFilePath = gifFilePath + "\\Frames";
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
      _mkdir(framesFilePath.c_str());
#else
      mkdir(framesFilePath.c_str() , 0775);
#endif
   }

   mRecord = enable;
}

void MenuState::generateGIF()
{
   std::string changeDirectoryCmd = "cd GIFs\\GIF_" + std::to_string(mRecordingDirectory) + " & ";
   std::string generateGifCmd     = "ffmpeg -y -framerate 50 -i Frames\\%01d.png GIF_" + std::to_string(mRecordingDirectory) + "_Slow.gif & \
                                     ffmpeg -y -i GIF_" + std::to_string(mRecordingDirectory) + "_Slow.gif -filter:v \"setpts=0.25*PTS\" \
                                     GIF_" + std::to_string(mRecordingDirectory) + "_Fast.gif";
   std::string fullCmd            = changeDirectoryCmd + generateGifCmd;

   system(fullCmd.c_str());
}
