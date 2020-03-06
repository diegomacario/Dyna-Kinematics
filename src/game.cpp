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
   , mSoundEngine(irrklang::createIrrKlangDevice(), [=](irrklang::ISoundEngine* soundEngine){soundEngine->drop();})
   , mCamera()
   , mRenderer2D()
   , mModelManager()
   , mTextureManager()
   , mShaderManager()
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
   float widthInPix = 1280.0f;
   float heightInPix = 720.0f;
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

   // Initialize the 2D renderer
   glm::mat4 orthoProj = glm::ortho(0.0f,        // Left
                                    widthInPix,  // Right
                                    heightInPix, // Bottom
                                    0.0f,        // Top
                                   -1.0f,        // Near
                                    1.0f);       // Far

   auto gameObj2DShader = mShaderManager.loadResource<ShaderLoader>("game_object_2D",
                                                                    "resources/shaders/game_object_2D.vs",
                                                                    "resources/shaders/game_object_2D.fs");
   gameObj2DShader->use();
   gameObj2DShader->setInt("image", 0);
   gameObj2DShader->setMat4("projection", orthoProj);
   mRenderer2D = std::make_unique<Renderer2D>(gameObj2DShader);

   // Initialize the 3D shader
   auto gameObj3DShader = mShaderManager.loadResource<ShaderLoader>("game_object_3D",
                                                                    "resources/shaders/game_object_3D.vs",
                                                                    "resources/shaders/game_object_3D.fs");
   gameObj3DShader->use();
   gameObj3DShader->setVec3("pointLights[0].worldPos", glm::vec3(0.0f, 0.0f, 100.0f));
   gameObj3DShader->setVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   gameObj3DShader->setFloat("pointLights[0].constantAtt", 1.0f);
   gameObj3DShader->setFloat("pointLights[0].linearAtt", 0.01f);
   gameObj3DShader->setFloat("pointLights[0].quadraticAtt", 0.0f);
   gameObj3DShader->setInt("numPointLightsInScene", 1);

   // Load the models

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["menu"] = std::make_shared<MenuState>(mFSM,
                                                 mWindow,
                                                 mCamera,
                                                 gameObj3DShader);

   mStates["play"] = std::make_shared<PlayState>(mFSM,
                                                 mWindow,
                                                 mSoundEngine,
                                                 mCamera,
                                                 gameObj3DShader);

   mStates["pause"] = std::make_shared<PauseState>(mFSM,
                                                   mWindow,
                                                   mCamera,
                                                   gameObj3DShader);

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
