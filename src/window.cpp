#include <iostream>

#include "window.h"

Window::Window(const std::string& title)
   : mWindow(nullptr)
   , mWidthOfWindowInPix(0)
   , mHeightOfWindowInPix(0)
   , mWidthOfFramebufferInPix(0)
   , mHeightOfFramebufferInPix(0)
   , mTitle(title)
   , mIsFullScreen(true)
   , mKeys()
   , mProcessedKeys()
   , mMouseMoved(false)
   , mFirstCursorPosCallback(true)
   , mLastCursorXPos(0.0)
   , mLastCursorYPos(0.0)
   , mCursorXOffset(0.0)
   , mCursorYOffset(0.0)
   , mScrollWheelMoved(false)
   , mScrollYOffset(0.0)
   , mMultisampleFBO(0)
   , mMultisampleTexture(0)
   , mMultisampleRBO(0)
   , mNumOfSamples(1)
{

}

Window::~Window()
{
   glDeleteFramebuffers(1, &mMultisampleFBO);
   glDeleteTextures(1, &mMultisampleTexture);
   glDeleteRenderbuffers(1, &mMultisampleRBO);

   if (mWindow)
   {
      glfwTerminate();
      mWindow = nullptr;
   }
}

bool Window::initialize()
{
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

   const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
   mWidthOfWindowInPix = mode->width;
   mHeightOfWindowInPix = mode->height;
   mWindow = glfwCreateWindow(mWidthOfWindowInPix, mHeightOfWindowInPix, mTitle.c_str(), glfwGetPrimaryMonitor(), nullptr);
   if (!mWindow)
   {
      std::cout << "Error - Window::initialize - Failed to create the GLFW window" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   glfwGetFramebufferSize(mWindow, &mWidthOfFramebufferInPix, &mHeightOfFramebufferInPix);

   glfwMakeContextCurrent(mWindow);

   setInputCallbacks();

   enableCursor(false);

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      std::cout << "Error - Window::initialize - Failed to load pointers to OpenGL functions using GLAD" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   glViewport(0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glEnable(GL_CULL_FACE);

   if (!configureAntiAliasingSupport())
   {
      std::cout << "Error - Window::initialize - Failed to configure anti aliasing support" << "\n";
      glfwTerminate();
      mWindow = nullptr;
      return false;
   }

   return true;
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

bool Window::isFullScreen() const
{
   return mIsFullScreen;
}

void Window::setFullScreen(bool fullScreen)
{
   if (fullScreen)
   {
      const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
      mWidthOfWindowInPix = mode->width;
      mHeightOfWindowInPix = mode->height;
      glfwSetWindowMonitor(mWindow, glfwGetPrimaryMonitor(), 0, 0, mWidthOfWindowInPix, mHeightOfWindowInPix, GLFW_DONT_CARE);
   }
   else
   {
      mWidthOfWindowInPix = 1280;
      mHeightOfWindowInPix = 720;
      glfwSetWindowMonitor(mWindow, NULL, 20, 50, mWidthOfWindowInPix, mHeightOfWindowInPix, GLFW_DONT_CARE);
   }

   mIsFullScreen = fullScreen;
}

bool Window::keyIsPressed(int key) const
{
   return mKeys.test(key);
}

bool Window::keyHasBeenProcessed(int key) const
{
   return mProcessedKeys.test(key);
}

void Window::setKeyAsProcessed(int key)
{
   mProcessedKeys.set(key);
}

bool Window::mouseMoved() const
{
   return mMouseMoved;
}

void Window::resetMouseMoved()
{
   mMouseMoved = false;
}

void Window::resetFirstMove()
{
   mFirstCursorPosCallback = true;
}

float Window::getCursorXOffset() const
{
   return mCursorXOffset;
}

float Window::getCursorYOffset() const
{
   return mCursorYOffset;
}

void Window::enableCursor(bool enable)
{
   glfwSetInputMode(mWindow, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool Window::scrollWheelMoved() const
{
   return mScrollWheelMoved;
}

void Window::resetScrollWheelMoved()
{
   mScrollWheelMoved = false;
}

float Window::getScrollYOffset() const
{
   return mScrollYOffset;
}

void Window::setInputCallbacks()
{
   glfwSetWindowUserPointer(mWindow, this);

   auto framebufferSizeCallback = [](GLFWwindow* window, int width, int height)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->framebufferSizeCallback(window, width, height);
   };

   auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->keyCallback(window, key, scancode, action, mods);
   };

   auto cursorPosCallback = [](GLFWwindow* window, double xPos, double yPos)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->cursorPosCallback(window, xPos, yPos);
   };

   auto scrollCallback = [](GLFWwindow* window, double xOffset, double yOffset)
   {
      static_cast<Window*>(glfwGetWindowUserPointer(window))->scrollCallback(window, xOffset, yOffset);
   };

   glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);
   glfwSetKeyCallback(mWindow, keyCallback);
   glfwSetCursorPosCallback(mWindow, cursorPosCallback);
   glfwSetScrollCallback(mWindow, scrollCallback);
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
   glfwGetWindowSize(window, &mWidthOfWindowInPix, &mHeightOfWindowInPix);

   mWidthOfFramebufferInPix = width;
   mHeightOfFramebufferInPix = height;

   resizeFramebuffers();

   glViewport(0, 0, width, height);
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (key != -1)
   {
      if (action == GLFW_PRESS)
      {
         mKeys.set(key);
      }
      else if (action == GLFW_RELEASE)
      {
         mKeys.reset(key);
      }

      mProcessedKeys.reset(key);
   }
}

void Window::cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
   if (mFirstCursorPosCallback)
   {
      mLastCursorXPos = xPos;
      mLastCursorYPos = yPos;
      mFirstCursorPosCallback = false;
   }

   // TODO: Ideally this function would tell the camera to update its position based on the offsets.
   // I'm going to make the camera ask the window if it should update its position. Is there a better way to do this?
   mCursorXOffset = static_cast<float>(xPos - mLastCursorXPos);
   mCursorYOffset = static_cast<float>(mLastCursorYPos - yPos); // Reversed since the Y-coordinates of the mouse go from the bottom to the top

   mLastCursorXPos = xPos;
   mLastCursorYPos = yPos;

   mMouseMoved = true;
}

void Window::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
   // TODO: Ideally this function would tell the camera to update its FOVY based on the Y offset.
   // I'm going to make the camera ask the window if it should update its FOVY. Is there a better way to do this?
   mScrollYOffset = static_cast<float>(yOffset);

   mScrollWheelMoved = true;
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
      std::cout << "Error - Window::configureAntiAliasingSupport - Multisample framebuffer is not complete" << "\n";
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

void Window::generateAntiAliasedImage()
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultisampleFBO);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glBlitFramebuffer(0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, 0, 0, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_COLOR_BUFFER_BIT, GL_NEAREST); // TODO: Should this be GL_LINEAR?
}

void Window::resizeFramebuffers()
{
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mMultisampleTexture);
   glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mNumOfSamples, GL_RGB, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix, GL_TRUE);
   glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

   glBindRenderbuffer(GL_RENDERBUFFER, mMultisampleRBO);
   glRenderbufferStorageMultisample(GL_RENDERBUFFER, mNumOfSamples, GL_DEPTH_COMPONENT, mWidthOfFramebufferInPix, mHeightOfFramebufferInPix);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
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
}
