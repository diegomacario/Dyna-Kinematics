#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <mutex>

// TODO: Take advantage of inlining in this class.
class Window
{
public:

   Window(const std::string& title);
   ~Window();

   Window(const Window&) = delete;
   Window& operator=(const Window&) = delete;

   Window(Window&&) = delete;
   Window& operator=(Window&&) = delete;

   bool         initialize();

   void         makeContextCurrent(bool enable);

   bool         shouldClose() const;
   void         setShouldClose(bool shouldClose); // TODO: Could this be considered to be const?
   void         swapBuffers();                    // TODO: Could this be considered to be const?
   void         pollEvents();                     // TODO: Could this be considered to be const?

   // Window
   unsigned int getWidthOfWindowInPix() const         { return mWidthOfWindowInPix; };
   unsigned int getHeightOfWindowInPix() const        { return mHeightOfWindowInPix; };
   unsigned int getWidthOfFramebufferInPix() const    { return mWidthOfFramebufferInPix; };
   unsigned int getHeightOfFramebufferInPix() const   { return mHeightOfFramebufferInPix; };
   void         setSceneLimits(int width, int height);
   void         enableResizing(bool enable);

   // Anti aliasing support
   bool         configureAntiAliasingSupport();
   bool         createMultisampleFramebuffer();
   void         clearAndBindMultisampleFramebuffer();
   void         clearMultisampleFramebuffer();
   void         bindMultisampleFramebuffer();
   void         generateAntiAliasedImage(unsigned int width, unsigned int height);
   void         resizeFramebuffers();
   void         setNumberOfSamples(unsigned int numOfSamples);

   // Memory support
   bool         configureMemorySupport();
   bool         createMemoryFramebuffer();
   void         clearAndBindMemoryFramebuffer();
   void         clearMemoryFramebuffer();
   void         bindMemoryFramebuffer();
   void         copyMemoryFramebufferIntoMultisampleFramebuffer(unsigned int width, unsigned int height);

   // Gif support
   bool         configureGifSupport();
   bool         createGifFramebuffer();
   void         clearAndBindGifFramebuffer();
   void         clearGifFramebuffer();
   void         bindGifFramebuffer();
   void         copyMultisampleFramebufferIntoGifFramebuffer(unsigned int width, unsigned int height);

   // Resize support
   void         updateBufferAndViewportSizes();
   float        getLowerLeftCornerOfViewportX() const { return mLowerLeftCornerOfViewportX; };
   float        getLowerLeftCornerOfViewportY() const { return mLowerLeftCornerOfViewportY; };
   float        getWidthOfViewport() const            { return mWidthOfViewport; };
   float        getHeightOfViewport() const           { return mHeightOfViewport; };
   bool         sizeChanged()                         { return mWindowSizeChanged; };
   void         resetSizeChanged()                    { mWindowSizeChanged = false; };
   std::mutex&  getMutex()                            { return mMutex; };

private:

   void         setInputCallbacks();
   void         framebufferSizeCallback(GLFWwindow* window, int width, int height);

   // Window
   GLFWwindow*                    mWindow;
   int                            mWidthOfWindowInPix;
   int                            mHeightOfWindowInPix;
   int                            mWidthOfFramebufferInPix;
   int                            mHeightOfFramebufferInPix;
   int                            mScaleFactor;
   std::string                    mTitle;

   // Anti aliasing support
   unsigned int                   mMultisampleFBO;
   unsigned int                   mMultisampleTexture;
   unsigned int                   mMultisampleRBO;

   // Memory support
   unsigned int                   mMemoryFBO;
   unsigned int                   mMemoryTexture;
   unsigned int                   mMemoryRBO;

   // Gif support
   unsigned int                   mGifFBO;
   unsigned int                   mGifTexture;

   unsigned int                   mNumOfSamples;

   // Resize support
   int                            mWidthOfScene;
   int                            mHeightOfScene;
   int                            mScaledWidthOfScene;
   int                            mScaledHeightOfScene;
   float                          mLowerLeftCornerOfViewportX;
   float                          mLowerLeftCornerOfViewportY;
   float                          mWidthOfViewport;
   float                          mHeightOfViewport;
   bool                           mWindowSizeChanged;
   std::mutex                     mMutex;
};

#endif
