#include "menu_state.h"

// TODO: Remove
#include "texture_loader.h"
#include "wall.h"
#include "rigid_body_2D.h"
GameObject2D* testGameObj2D = nullptr;
Wall* testWall              = nullptr;
RigidBody2D* testRigidBody  = nullptr;

MenuState::MenuState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
                     const std::shared_ptr<Window>&             window,
                     const std::shared_ptr<Camera>&             camera,
                     const std::shared_ptr<Renderer2D>&         renderer2D)
   : mFSM(finiteStateMachine)
   , mWindow(window)
   , mCamera(camera)
   , mRenderer2D(renderer2D)
{
   // TODO: Remove
   testGameObj2D = new GameObject2D(ResourceManager<Texture>().loadUnmanagedResource<TextureLoader>("resources/textures/awesomeface.png"),
                                    glm::vec2(0, 0),
                                    45,
                                    100,
                                    100);
   testWall = new Wall(glm::vec2(1.0f, 1.0f),
                       glm::vec2(-1280 / 2, -720 / 2),
                       glm::vec2( 1280 / 2,  720 / 2));
   testRigidBody = new RigidBody2D(10.0f, 400.0f, 200.0f, 1);
   testRigidBody->mCurrentState.positionOfCenterOfMass = glm::vec2(0.0f, 0.0f);
   testRigidBody->mCurrentState.orientation = 45.0f;
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
}

void MenuState::update(float deltaTime)
{

}

void MenuState::render()
{
   mWindow->clearAndBindMultisampleFramebuffer();

   // Use the shader and set uniforms

   // Render objects

   // TODO: Remove
   //mRenderer2D->renderColoredQuad(*testGameObj2D);
   //mRenderer2D->renderTexturedQuad(*testGameObj2D);
   mRenderer2D->renderRigidBody(*testRigidBody);
   mRenderer2D->renderLine(*testWall);

   mWindow->generateAntiAliasedImage();

   mWindow->swapBuffers();
   mWindow->pollEvents();
}

void MenuState::exit()
{

}
