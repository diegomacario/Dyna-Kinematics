#include <iostream>

#include "shader_loader.h"
#include "texture_loader.h"
#include "model_loader.h"
#include "menu_state.h"
#include "play_state.h"
#include "pause_state.h"
#include "win_state.h"
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
   , mTitle()
   , mTable()
   , mLeftPaddle()
   , mRightPaddle()
   , mBall()
   , mPoint()
   , mLeftPaddleWins()
   , mRightPaddleWins()
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

   // Initialize the explosive 3D shader
   auto gameObj3DExplosiveShader = mShaderManager.loadResource<ShaderLoader>("game_object_3D_explosive",
                                                                             "resources/shaders/game_object_3D.vs",
                                                                             "resources/shaders/game_object_3D.fs",
                                                                             "resources/shaders/game_object_3D_explosive.gs");
   gameObj3DExplosiveShader->use();
   gameObj3DExplosiveShader->setVec3("pointLights[0].worldPos", glm::vec3(0.0f, 0.0f, 100.0f));
   gameObj3DExplosiveShader->setVec3("pointLights[0].color", glm::vec3(1.0f, 1.0f, 1.0f));
   gameObj3DExplosiveShader->setFloat("pointLights[0].constantAtt", 1.0f);
   gameObj3DExplosiveShader->setFloat("pointLights[0].linearAtt", 0.01f);
   gameObj3DExplosiveShader->setFloat("pointLights[0].quadraticAtt", 0.0f);
   gameObj3DExplosiveShader->setInt("numPointLightsInScene", 1);

   // Load the models
   mModelManager.loadResource<ModelLoader>("title", "resources/models/title/title.obj");
   mModelManager.loadResource<ModelLoader>("table", "resources/models/table/table.obj");
   mModelManager.loadResource<ModelLoader>("left_paddle", "resources/models/left_paddle/paddle.obj");
   mModelManager.loadResource<ModelLoader>("right_paddle", "resources/models/right_paddle/paddle.obj");
   mModelManager.loadResource<ModelLoader>("teapot", "resources/models/teapot/teapot.obj");
   mModelManager.loadResource<ModelLoader>("point", "resources/models/point/point.obj");
   mModelManager.loadResource<ModelLoader>("left_paddle_wins", "resources/models/left_paddle_wins/left_paddle_wins.obj");
   mModelManager.loadResource<ModelLoader>("right_paddle_wins", "resources/models/right_paddle_wins/right_paddle_wins.obj");

   mTitle = std::make_shared<GameObject3D>(mModelManager.getResource("title"),
                                           glm::vec3(0.0f, 0.0f, 13.75f),
                                           90.0f,
                                           glm::vec3(1.0f, 0.0f, 0.0f),
                                           1.0f);

   mTable = std::make_shared<GameObject3D>(mModelManager.getResource("table"),
                                           glm::vec3(0.0f),
                                           90.0f,
                                           glm::vec3(1.0f, 0.0f, 0.0f),
                                           1.0f);

   mLeftPaddle = std::make_shared<Paddle>(mModelManager.getResource("left_paddle"),
                                          glm::vec3(-45.0f, 0.0f, 0.0f),
                                          90.0f,
                                          glm::vec3(1.0f, 0.0f, 0.0f),
                                          1.0f,
                                          glm::vec3(0.0f, 30.0f, 0.0f),
                                          3.5f,
                                          7.5f);

   mRightPaddle = std::make_shared<Paddle>(mModelManager.getResource("right_paddle"),
                                           glm::vec3(45.0f, 0.0f, 0.0f),
                                           90.0f,
                                           glm::vec3(1.0f, 0.0f, 0.0f),
                                           1.0f,
                                           glm::vec3(0.0f, 30.0f, 0.0f),
                                           3.5f,
                                           7.5f);

   mBall = std::make_shared<Ball>(mModelManager.getResource("teapot"),
                                  glm::vec3(0.0f, 0.0f, 1.96875 * (7.5f / 2.5f)),
                                  90.0f,
                                  glm::vec3(1.0f, 0.0f, 0.0f),
                                  7.5f / 2.5f,
                                  glm::vec3(35.0f, 45.0f, 0.0f),
                                  7.5f,
                                  1000.0f);

   mPoint = std::make_shared<GameObject3D>(mModelManager.getResource("point"),
                                           glm::vec3(0.0f),
                                           90.0f,
                                           glm::vec3(1.0f, 0.0f, 0.0f),
                                           7.0f);

   mLeftPaddleWins = std::make_shared<GameObject3D>(mModelManager.getResource("left_paddle_wins"),
                                                    glm::vec3(0.0f),
                                                    90.0f,
                                                    glm::vec3(1.0f, 0.0f, 0.0f),
                                                    1.0f);

   mRightPaddleWins = std::make_shared<GameObject3D>(mModelManager.getResource("right_paddle_wins"),
                                                     glm::vec3(0.0f),
                                                     90.0f,
                                                     glm::vec3(1.0f, 0.0f, 0.0f),
                                                     1.0f);

   // Create the FSM
   mFSM = std::make_shared<FiniteStateMachine>();

   // Initialize the states
   std::unordered_map<std::string, std::shared_ptr<State>> mStates;

   mStates["menu"] = std::make_shared<MenuState>(mFSM,
                                                 mWindow,
                                                 mCamera,
                                                 gameObj3DShader,
                                                 mTitle,
                                                 mTable,
                                                 mLeftPaddle,
                                                 mRightPaddle,
                                                 mBall);

   mStates["play"] = std::make_shared<PlayState>(mFSM,
                                                 mWindow,
                                                 mSoundEngine,
                                                 mCamera,
                                                 gameObj3DShader,
                                                 mTable,
                                                 mLeftPaddle,
                                                 mRightPaddle,
                                                 mBall,
                                                 mPoint);

   mStates["pause"] = std::make_shared<PauseState>(mFSM,
                                                   mWindow,
                                                   mCamera,
                                                   gameObj3DShader,
                                                   mTable,
                                                   mLeftPaddle,
                                                   mRightPaddle,
                                                   mBall,
                                                   mPoint);

   mStates["win"] = std::make_shared<WinState>(mFSM,
                                               mWindow,
                                               mCamera,
                                               gameObj3DExplosiveShader,
                                               mBall,
                                               mLeftPaddleWins,
                                               mRightPaddleWins);

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
