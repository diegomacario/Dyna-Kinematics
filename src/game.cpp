#include <iostream>

#include "shader_loader.h"
#include "texture_loader.h"
#include "model_loader.h"
#include "menu_state.h"
#include "play_state.h"
#include "pause_state.h"
#include "game.h"

Game::Game()
   : mFSM()
   , mWindow()
   //, mSoundEngine(irrklang::createIrrKlangDevice(), [=](irrklang::ISoundEngine* soundEngine){soundEngine->drop();})
   , mCamera()
   , mRenderer2D()
   , mModelManager()
   , mTextureManager()
   , mShaderManager()
   , mWorld()
{

}

Game::~Game()
{

}

bool Game::initialize(const std::string& title)
{
   // Initialize the window
   mWindow = std::make_shared<Window>(title);

   if (!mWindow->initialize())
   {
      std::cout << "Error - Game::initialize - Failed to initialize the window" << "\n";
      return false;
   }

   // Initialize the camera
   float widthInPix = 400.0f;
   float heightInPix = 400.0f;
   float aspectRatio = (widthInPix / heightInPix);

   mCamera = std::make_shared<Camera>(glm::vec3(0.0f, 0.0f, 95.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f),
                                      0.0f,
                                      0.0f,
                                      45.0f,       // Fovy
                                      aspectRatio, // Aspect ratio
                                      0.1f,        // Near
                                      130.0f,      // Far
                                      20.0f,       // Movement speed
                                      0.1f);       // Mouse sensitivity

   glm::mat4 orthoProj = glm::ortho(-widthInPix / 2,  // Left
                                     widthInPix / 2,  // Right
                                    -heightInPix / 2, // Bottom
                                     heightInPix / 2, // Top
                                    -1.0f,            // Near
                                     1.0f);           // Far

   auto texture2DShader = mShaderManager.loadUnmanagedResource<ShaderLoader>("resources/shaders/texture_2D.vs",
                                                                             "resources/shaders/texture_2D.fs");
   texture2DShader->use();
   texture2DShader->setInt("image", 0);
   texture2DShader->setMat4("projection", orthoProj);

   auto color2DShader = mShaderManager.loadUnmanagedResource<ShaderLoader>("resources/shaders/color_2D.vs",
                                                                           "resources/shaders/color_2D.fs");
   color2DShader->use();
   color2DShader->setMat4("projection", orthoProj);

   auto line2DShader = mShaderManager.loadUnmanagedResource<ShaderLoader>("resources/shaders/line_2D.vs",
                                                                          "resources/shaders/line_2D.fs");
   line2DShader->use();
   line2DShader->setMat4("projection", orthoProj);

   mRenderer2D = std::make_unique<Renderer2D>(texture2DShader, color2DShader, line2DShader);

   // Create the walls
   float halfWidth  = (widthInPix / 2) - 3;
   float halfHeight = (heightInPix / 2) - 3;
   std::vector<Wall> walls;
   walls.push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls.push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls.push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls.push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Create the rigid bodies
   std::vector<RigidBody2D> rigidBodies;
   rigidBodies.push_back(RigidBody2D(8.0f, 40.0f, 20.0f, 0.75f, glm::vec2(0.0f, 0.0f), 3.14159265358979323846f / 8, glm::vec2(0.0f, 0.0f), 0.0f));

   // Create the world
   mWorld = std::make_shared<World>(std::move(walls), rigidBodies);

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["menu"] = std::make_shared<MenuState>(mFSM,
                                                 mWindow,
                                                 mCamera,
                                                 mRenderer2D,
                                                 mWorld);

   mStates["play"] = std::make_shared<PlayState>(mFSM,
                                                 mWindow,
                                                 mSoundEngine,
                                                 mCamera,
                                                 mRenderer2D);

   mStates["pause"] = std::make_shared<PauseState>(mFSM,
                                                   mWindow,
                                                   mCamera,
                                                   mRenderer2D);

   // Initialize the FSM
   mFSM->initialize(std::move(mStates), "menu");

   return true;
}

void Game::executeGameLoop()
{
   double currentFrame = 0.0;
   double lastFrame    = 0.0;
   float  deltaTime    = 0.0f;

   while (!mWindow->shouldClose())
   {
      currentFrame = glfwGetTime();
      deltaTime    = static_cast<float>(currentFrame - lastFrame);
      lastFrame    = currentFrame;

      mFSM->processInputInCurrentState(deltaTime);
      mFSM->updateCurrentState(deltaTime);
      mFSM->renderCurrentState();
   }
}
