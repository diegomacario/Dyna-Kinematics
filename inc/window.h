#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <bitset>

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

   bool         shouldClose() const;
   void         setShouldClose(bool shouldClose); // TODO: Could this be considered to be const?
   void         swapBuffers();                    // TODO: Could this be considered to be const?
   void         pollEvents();                     // TODO: Could this be considered to be const?

   // Window
   unsigned int getWidthOfWindowInPix() const;
   unsigned int getHeightOfWindowInPix() const;
   unsigned int getWidthOfFramebufferInPix() const;
   unsigned int getHeightOfFramebufferInPix() const;
   bool         isFullScreen() const;
   void         setFullScreen(bool fullScreen);
   void         setSizeLimits(int width, int height);
   void         setSceneLimits(int width, int height);

   // Anti aliasing support
   bool         configureAntiAliasingSupport();
   bool         createMultisampleFramebuffer();
   void         clearAndBindMultisampleFramebuffer();
   void         clearMultisampleFramebuffer();
   void         bindMultisampleFramebuffer();
   void         generateAntiAliasedImage();
   void         resizeFramebuffers();
   void         setNumberOfSamples(unsigned int numOfSamples);

   // Memory support
   bool         configureMemorySupport();
   bool         createMemoryFramebuffer();
   void         clearAndBindMemoryFramebuffer();
   void         clearMemoryFramebuffer();
   void         bindMemoryFramebuffer();
   void         copyMemoryFramebufferIntoMultisampleFramebuffer();

   // Gif support
   bool         configureGifSupport();
   bool         createGifFramebuffer();
   void         clearAndBindGifFramebuffer();
   void         clearGifFramebuffer();
   void         bindGifFramebuffer();
   void         copyMultisampleFramebufferIntoGifFramebuffer();

   // Resize support
   void         updateBufferAndViewportSizes();

private:

   void         setInputCallbacks();
   void         framebufferSizeCallback(GLFWwindow* window, int width, int height);

   // Window
   GLFWwindow*                    mWindow;
   int                            mWidthOfWindowInPix;
   int                            mHeightOfWindowInPix;
   int                            mWidthOfFramebufferInPix;
   int                            mHeightOfFramebufferInPix;
   std::string                    mTitle;
   bool                           mIsFullScreen;

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
   int                            mLowerLeftCornerOfViewportX;
   int                            mLowerLeftCornerOfViewportY;
   int                            mWidthOfScene;
   int                            mHeightOfScene;
};

#endif
