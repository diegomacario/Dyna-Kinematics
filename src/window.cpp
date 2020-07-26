#include <iostream>

#include "window.h"

Window::Window(const std::string& title)
   : mWindow(nullptr)
   , mWidthOfWindowInPix(0)
   , mHeightOfWindowInPix(0)
   , mWidthOfFramebufferInPix(0)
   , mHeightOfFramebufferInPix(0)
   , mScaleFactor(1)
   , mTitle(title)
   , mMultisampleFBO(0)
   , mMultisampleTexture(0)
   , mMultisampleRBO(0)
   , mMemoryFBO(0)
   , mMemoryTexture(0)
   , mMemoryRBO(0)
   , mGifFBO(0)
   , mGifTexture(0)
   , mNumOfSamples(1)
   , mWidthOfScene(450)
   , mHeightOfScene(450)
   , mScaledWidthOfScene(0)
   , mScaledHeightOfScene(0)
   , mLowerLeftCornerOfViewportX(0.0f)
   , mLowerLeftCornerOfViewportY(0.0f)
   , mWidthOfViewport(0.0f)
   , mHeightOfViewport(0.0f)
   , mWindowSizeChanged(false)
   , mMutex()
{

}

Window::~Window()
{
   glDeleteFramebuffers(1, &mMultisampleFBO);
   glDeleteTextures(1, &mMultisampleTexture);
   glDeleteRenderbuffers(1, &mMultisampleRBO);

   glDeleteFramebuffers(1, &mMemoryFBO);
   glDeleteTextures(1, &mMemoryTexture);
   glDeleteRenderbuffers(1, &mMemoryRBO);

   glDeleteFramebuffers(1, &mGifFBO);
   glDeleteTextures(1, &mGifTexture);

   if (mWindow)
   {
      glfwTerminate();
      mWindow = nullptr;
   }
}

bool Window::initialize()
{
   auto errorCallback = [](int error, const char* description)
   {
      std::cout << "Error: " << description << '\n';
   };

   glfwSetErrorCallback(errorCallback);

   if (!glfwInit())
   {
      std::cout << "Error - Window::initialize - Failed to initialize GLFW" << "\n";
      return false;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

   mWidthOfWindowInPix = 650;
   mHeightOfWindowInPix = 650;
   mWindow = glfwCreateWindow(mWidthOfWindowInPix, mHeightOfWindowInPix, mTitle.c_str(), nullptr, nullptr);
   if (!mWindow)
   {
      std::cout << "Error - Window::initialize - Failed to create the GLFW window" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   glfwMakeContextCurrent(mWindow);

   glfwGetWindowSize(mWindow, &mWidthOfWindowInPix, &mHeightOfWindowInPix);
   glfwGetFramebufferSize(mWindow, &mWidthOfFramebufferInPix, &mHeightOfFramebufferInPix);

   mScaleFactor = mWidthOfFramebufferInPix / mWidthOfWindowInPix;
   mScaledWidthOfScene  = mWidthOfScene * mScaleFactor;
   mScaledHeightOfScene = mHeightOfScene * mScaleFactor;

   glfwSetWindowPos(mWindow, 170, 70);

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      std::cout << "Error - Window::initialize - Failed to load pointers to OpenGL functions using GLAD" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   mLowerLeftCornerOfViewportX = 0.0f;
   mLowerLeftCornerOfViewportY = 0.0f;
   mWidthOfViewport            = mWidthOfFramebufferInPix;
   mHeightOfViewport           = mHeightOfFramebufferInPix;
   glViewport(mLowerLeftCornerOfViewportX, mLowerLeftCornerOfViewportY, mWidthOfViewport, mHeightOfViewport);
   glEnable(GL_CULL_FACE);

   if (!configureAntiAliasingSupport())
   {
      std::cout << "Error - Window::initialize - Failed to configure anti aliasing support" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   if (!configureMemorySupport())
   {
      std::cout << "Error - Window::initialize - Failed to configure memory support" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   if (!configureGifSupport())
   {
      std::cout << "Error - Window::initialize - Failed to configure gif support" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   updateBufferAndViewportSizes();

   setInputCallbacks();

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   // For debugging purposes
   //glClearColor(0.0f, 1.0f, 0.5f, 1.0f);

   return true;
}

void Window::makeContextCurrent(bool enable)
{
   if (enable)
   {
      glfwMakeContextCurrent(mWindow);
   }
   else
   {
      glfwMakeContextCurrent(NULL);
   }
}

bool Window::shouldClose() const
{
   int windowShouldClose = glfwWindowShouldClose(mWindow);

   if (windowShouldClose == 0)
   {
      return false;
   }
   else
   {
      return true;
   }
}

void Window::setShouldClose(bool shouldClose)
{
   glfwSetWindowShouldClose(mWindow, shouldClose);
}

void Window::swapBuffers()
{
   glfwSwapBuffers(mWindow);
}

void Window::pollEvents()
{
   glfwPollEvents();
}

unsigned int Window::getWidthOfWindowInPix() const
{
   return mWidthOfWindowInPix;
}

unsigned int Window::getHeightOfWindowInPix() const
{
   return mHeightOfWindowInPix;
}

unsigned int Window::getWidthOfFramebufferInPix() const
{
   return mWidthOfFramebufferInPix;
}

unsigned int Window::getHeightOfFramebufferInPix() const
{
   return mHeightOfFramebufferInPix;
}

void Window::setSceneLimits(int width, int height)
{
   mWidthOfScene  = width;
   mHeightOfScene = height;
   mScaledWidthOfScene  = width * mScaleFactor;
   mScaledHeightOfScene = height * mScaleFactor;
}

void Window::setInputCallbacks()
{
   glfwSetWindowUserPointer(mWindow, this);

   auto framebufferSizeCallback = [](GLFWwindow* window, int width, int height)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->framebufferSizeCallback(window, width, height);
   };

   glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
   std::lock_guard<std::mutex> guard(mMutex);

   glfwGetWindowSize(window, &mWidthOfWindowInPix, &mHeightOfWindowInPix);

   mWidthOfFramebufferInPix = width;
   mHeightOfFramebufferInPix = height;

   mWindowSizeChanged = true;
}

bool Window::configureAntiAliasingSupport()
{
   if (!createMultisampleFramebuffer())
   {
      return false;
   }

   return true;
}

bool Window::createMultisampleFramebuffer()
{
   // Configure a framebuffer object to store raw multisample renders

   glGenFramebuffers(1, &mMultisampleFBO);

   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);

   // Create a multisample texture and use it as a color attachment
   glGenTextures(1, &mMultisampleTexture);

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture, 0);

   // Create a multisample renderbuffer object and use it as a depth attachment
   glGenRenderbuffers(1, &mMultisampleRBO);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mMultisampleRBO);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Window::createMultisampleFramebuffer - Multisample framebuffer is not complete" << "\n";
      return false;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return true;
}

void Window::clearAndBindMultisampleFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::clearMultisampleFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Window::bindMultisampleFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMultisampleFBO);
}

void Window::generateAntiAliasedImage()
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultisampleFBO);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glBlitFramebuffer(0, 0,
                     mWidthOfFramebufferInPix, mHeightOfFramebufferInPix,
                     0, 0,
                     mWidthOfFramebufferInPix, mHeightOfFramebufferInPix,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST); // TODO: Should this be GL_LINEAR?
}

void Window::resizeFramebuffers()
{
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMemoryTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMemoryRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindTexture(GL_TEXTURE_2D, mGifTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   glBindTexture(GL_TEXTURE_2D, 0);
}

void Window::setNumberOfSamples(unsigned int numOfSamples)
{
   mNumOfSamples = numOfSamples;

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMemoryTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMemoryRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

bool Window::configureMemorySupport()
{
   if (!createMemoryFramebuffer())
   {
      return false;
   }

   return true;
}

bool Window::createMemoryFramebuffer()
{
   // Configure a framebuffer object to store raw multisample renders

   glGenFramebuffers(1, &mMemoryFBO);

   glBindFramebuffer(GL_FRAMEBUFFER, mMemoryFBO);

   // Create a multisample texture and use it as a color attachment
   glGenTextures(1, &mMemoryTexture);

   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMemoryTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mMemoryTexture, 0);

   // Create a multisample renderbuffer object and use it as a depth attachment
   glGenRenderbuffers(1, &mMemoryRBO);

   glBindRenderbuffer(GL_RENDERBUFFER, mMemoryRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);

   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mMemoryRBO);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Window::createMemoryFramebuffer - Memory framebuffer is not complete" << "\n";
      return false;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return true;
}

void Window::clearAndBindMemoryFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMemoryFBO);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::clearMemoryFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMemoryFBO);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Window::bindMemoryFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mMemoryFBO);
}

void Window::copyMemoryFramebufferIntoMultisampleFramebuffer()
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, mMemoryFBO);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mMultisampleFBO);
   glBlitFramebuffer(0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_COLOR_BUFFER_BIT, GL_NEAREST); // TODO: Should this be GL_LINEAR?
}

bool Window::configureGifSupport()
{
   if (!createGifFramebuffer())
   {
      return false;
   }

   return true;
}

bool Window::createGifFramebuffer()
{
   // Configure a framebuffer object to store gifs

   glGenFramebuffers(1, &mGifFBO);

   glBindFramebuffer(GL_FRAMEBUFFER, mGifFBO);

   // Create a texture and use it as a color attachment
   glGenTextures(1, &mGifTexture);

   glBindTexture(GL_TEXTURE_2D, mGifTexture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   glBindTexture(GL_TEXTURE_2D, 0);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGifTexture, 0);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   {
      std::cout << "Error - Window::createGifFramebuffer - Gif framebuffer is not complete" << "\n";
      return false;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return true;
}

void Window::clearAndBindGifFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mGifFBO);
   glClear(GL_COLOR_BUFFER_BIT);
}

void Window::clearGifFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mGifFBO);
   glClear(GL_COLOR_BUFFER_BIT);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Window::bindGifFramebuffer()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mGifFBO);
}

void Window::copyMultisampleFramebufferIntoGifFramebuffer()
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultisampleFBO);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mGifFBO);
   glBlitFramebuffer(0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_COLOR_BUFFER_BIT, GL_NEAREST); // TODO: Should this be GL_LINEAR?
}

void Window::updateBufferAndViewportSizes()
{
   glfwGetWindowSize(mWindow, &mWidthOfWindowInPix, &mHeightOfWindowInPix);
   glfwGetFramebufferSize(mWindow, &mWidthOfFramebufferInPix, &mHeightOfFramebufferInPix);

   resizeFramebuffers();

   clearMultisampleFramebuffer();
   clearMemoryFramebuffer();
   clearGifFramebuffer();

   float aspectRatioOfScene  = ((float) mScaledWidthOfScene) / mScaledHeightOfScene;

   // Let's say we want to use the width of the window for the viewport
   // What height would we need to keep the aspect ratio of the scene?
   float requiredHeight = mWidthOfFramebufferInPix * (1.0f / aspectRatioOfScene);

   // If the required height is greater than the height of the window, then we use the height of the window for the viewport
   if (requiredHeight > mHeightOfFramebufferInPix)
   {
      //std::cout << "The required height is greater than the height of the framebuffer: " << requiredHeight << " > " << mHeightOfFramebufferInPix << '\n';
      // What width would we need to keep the aspect ratio of the scene?
      float requiredWidth = mHeightOfFramebufferInPix * aspectRatioOfScene;
      if (requiredWidth > mWidthOfFramebufferInPix)
      {
         //std::cout << "The required width is greater than the width of the framebuffer: " << requiredWidth << " > " << mWidthOfFramebufferInPix << '\n';
         std::cout << "Error - Window::updateBufferAndViewportSizes - Couldn't calculate dimensions that preserve the aspect ratio of the scene" << '\n';
      }
      else
      {
         //std::cout << "Using the required width: " << requiredWidth << '\n';
         float widthOfTheTwoVerticalBars = mWidthOfFramebufferInPix - requiredWidth;
         //std::cout << "Viewport dimensions: " << widthOfTheTwoVerticalBars / 2.0f << " " << 0 << " " << requiredWidth << " " << mHeightOfFramebufferInPix << '\n';

         mLowerLeftCornerOfViewportX = widthOfTheTwoVerticalBars / 2.0f;
         mLowerLeftCornerOfViewportY = 0.0f;
         mWidthOfViewport            = requiredWidth;
         mHeightOfViewport           = mHeightOfFramebufferInPix;
      }
   }
   else
   {
      //std::cout << "Using the required height: " << requiredHeight << '\n';
      float heightOfTheTwoHorizontalBars = mHeightOfFramebufferInPix - requiredHeight;
      //std::cout << "Viewport dimensions: " << 0 << " " << heightOfTheTwoHorizontalBars / 2.0f << " " << mWidthOfFramebufferInPix << " " << requiredHeight << '\n';

      mLowerLeftCornerOfViewportX = 0.0f;
      mLowerLeftCornerOfViewportY = heightOfTheTwoHorizontalBars / 2.0f;
      mWidthOfViewport            = mWidthOfFramebufferInPix;
      mHeightOfViewport           = requiredHeight;
   }

   //std::cout << '\n';
}

bool Window::sizeChanged()
{
   return mWindowSizeChanged;
}

void Window::resetSizeChanged()
{
   mWindowSizeChanged = false;
}

std::mutex& Window::getMutex()
{
   return mMutex;
}
