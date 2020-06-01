#include <iostream>

#include "shader_loader.h"
#include "texture_loader.h"
#include "model_loader.h"
#include "menu_state.h"
#include "play_state.h"
#include "pause_state.h"
#include "game.h"

Game::Game(QObject* parent)
   : QThread(parent)
   , mInitialized(false)
   , mSimulate(false)
   , mTerminate(false)
   , mTimeStep(0.02f)
   , mSceneDimensions()
   , mRecordGIF(false)
   , mFSM()
   , mWindow()
   //, mSoundEngine(irrklang::createIrrKlangDevice(), [=](irrklang::ISoundEngine* soundEngine){soundEngine->drop();})
   , mCamera()
   , mRenderer2D()
   , mModelManager()
   , mTextureManager()
   , mShaderManager()
   , mWorld()
{
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeScene,     this, &Game::changeScene);

   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::startSimulation, this, &Game::startSimulation);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::pauseSimulation, this, &Game::pauseSimulation);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::resetSimulation, this, &Game::resetSimulation);

   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeGravity, this, &Game::changeGravity);

   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeTimeStep,                 this, &Game::changeTimeStep);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeCoefficientOfRestitution, this, &Game::changeCoefficientOfRestitution);

   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::enableWireframeMode,           this, &Game::enableWireframeMode);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::enableRememberFrames,          this, &Game::enableRememberFrames);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeRememberFramesFrequency, this, &Game::changeRememberFramesFrequency);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::enableAntiAliasing,            this, &Game::enableAntiAliasing);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::changeAntiAliasingMode,        this, &Game::changeAntiAliasingMode);
   QObject::connect(dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::enableRecordGIF,               this, &Game::enableRecordGIF);

   QObject::connect(this, &Game::simulationError, dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::processSimulationError);

   QObject::connect(this, &Game::simulatorViewerClosed, dynamic_cast<RigidBodySimulator*>(parent), &RigidBodySimulator::close);
}

Game::~Game()
{
   mTerminate = true;
   wait();
}

glm::mat2 calc2DRotMat(float angleInDeg)
{
   glm::mat2 rotationMatrix;

   float cosVal = cos(glm::radians(angleInDeg));
   float sinVal = sin(glm::radians(angleInDeg));

   rotationMatrix[0][0] =  cosVal; // Top left
   rotationMatrix[1][0] = -sinVal; // Top right
   rotationMatrix[0][1] =  sinVal; // Bottom left
   rotationMatrix[1][1] =  cosVal; // Bottom right

   return rotationMatrix;
}

float calcRandFloat(float max)
{
   return static_cast<float>(rand()) / static_cast<float>(RAND_MAX / max);
}

float calcRandFloat(float min, float max)
{
   return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (max - min));
}

glm::vec2 calcNormal(glm::vec2 deltas, bool invert = false)
{
   float dx = deltas.x;
   float dy = deltas.y;
   if (invert)
   {
      return glm::normalize(glm::vec2(-dy, dx));
   }
   else
   {
      return glm::normalize(glm::vec2(dy, -dx));
   }
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
   float widthInPix  = 450.0f;
   float heightInPix = 450.0f;
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

   // Creat the scene dimensions
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Single
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Pair
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 300.0f + 50.0f)); // Momentum
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 300.0f + 50.0f)); // Torque
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Plus Sign
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Multiplication Sign
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Star
   mSceneDimensions.push_back(glm::vec2(400.0f + 50.0f, 400.0f + 50.0f)); // Stack
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 400.0f + 50.0f)); // Stack Being Hit
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 800.0f + 50.0f)); // Hexagon
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 800.0f + 50.0f)); // Octagon
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 400.0f + 50.0f)); // Downward slope
   mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 400.0f + 50.0f)); // Upward slope
   //mSceneDimensions.push_back(glm::vec2(800.0f + 50.0f, 800.0f + 50.0f)); // Fall

   // Create the walls
   std::vector<std::vector<Wall>> walls(13);

   float halfWidth  = 200.0f;
   float halfHeight = 200.0f;

   // Single
   walls[0].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[0].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[0].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[0].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Pair
   walls[1].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[1].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[1].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[1].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Momentum
   walls[2].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -400.0f,  150.0f), glm::vec2(  400.0f,  150.0f))); // Top wall
   walls[2].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  400.0f, -150.0f), glm::vec2( -400.0f, -150.0f))); // Bottom wall
   walls[2].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  400.0f,  150.0f), glm::vec2(  400.0f, -150.0f))); // Right wall
   walls[2].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -400.0f, -150.0f), glm::vec2( -400.0f,  150.0f))); // Left wall

   // Torque
   walls[3].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -400.0f,  150.0f), glm::vec2(  400.0f,  150.0f))); // Top wall
   walls[3].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  400.0f, -150.0f), glm::vec2( -400.0f, -150.0f))); // Bottom wall
   walls[3].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  400.0f,  150.0f), glm::vec2(  400.0f, -150.0f))); // Right wall
   walls[3].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -400.0f, -150.0f), glm::vec2( -400.0f,  150.0f))); // Left wall

   // Plus Sign
   walls[4].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[4].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[4].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[4].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Multiplication Sign
   walls[5].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[5].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[5].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[5].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Star
   walls[6].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[6].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[6].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[6].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Stack
   walls[7].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -halfWidth,  halfHeight), glm::vec2(  halfWidth,  halfHeight))); // Top wall
   walls[7].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  halfWidth, -halfHeight), glm::vec2( -halfWidth, -halfHeight))); // Bottom wall
   walls[7].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  halfWidth,  halfHeight), glm::vec2(  halfWidth, -halfHeight))); // Right wall
   walls[7].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -halfWidth, -halfHeight), glm::vec2( -halfWidth,  halfHeight))); // Left wall

   // Stack Being Hit
   walls[8].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -400.0f,  200.0f), glm::vec2(  400.0f,  200.0f))); // Top wall
   walls[8].push_back(Wall(glm::vec2( 0.0f,  1.0f), glm::vec2(  400.0f, -200.0f), glm::vec2( -400.0f, -200.0f))); // Bottom wall
   walls[8].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  400.0f,  200.0f), glm::vec2(  400.0f, -200.0f))); // Right wall
   walls[8].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -400.0f, -200.0f), glm::vec2( -400.0f,  200.0f))); // Left wall

   glm::vec2 vertA(400.0f, 0.0f);
   glm::vec2 vertB(400.0f / 2.0f, 400.0f * sin(glm::radians(60.0f)));
   glm::vec2 normalAB = glm::normalize(glm::vec2(0.0f) - ((vertA + vertB) / 2.0f));
   glm::mat2 rotation = calc2DRotMat(30.0f);

   // Hexagon
   walls[9].push_back(Wall(rotation * normalAB,                            rotation * vertA,                         rotation * vertB));
   walls[9].push_back(Wall(rotation * glm::vec2(0.0f, -1.0f),              rotation * vertB,                         rotation * glm::vec2(-vertB.x, vertB.y)));
   walls[9].push_back(Wall(rotation * glm::vec2(-normalAB.x, normalAB.y),  rotation * glm::vec2(-vertB.x, vertB.y),  rotation * glm::vec2(-vertA.x, vertA.y)));
   walls[9].push_back(Wall(rotation * glm::vec2(-normalAB.x, -normalAB.y), rotation * glm::vec2(-vertA.x, vertA.y),  rotation * glm::vec2(-vertB.x, -vertB.y)));
   walls[9].push_back(Wall(rotation * glm::vec2(0.0f, 1.0f),               rotation * glm::vec2(-vertB.x, -vertB.y), rotation * glm::vec2(vertB.x, -vertB.y)));
   walls[9].push_back(Wall(rotation * glm::vec2(normalAB.x,-normalAB.y),   rotation * glm::vec2(vertB.x, -vertB.y),  rotation * vertA));

   // Octagon
   glm::vec2 vert1(400.0f, 0.0f);
   glm::vec2 vert2 = calc2DRotMat(45.0f) * vert1;
   glm::vec2 normal12 = glm::normalize(glm::vec2(0.0f) - ((vert1 + vert2) / 2.0f));

   walls[10].push_back(Wall(normal12,                                               vert1,                        vert2));
   walls[10].push_back(Wall(calc2DRotMat(45.0)   * normal12, calc2DRotMat(45.0)   * vert1, calc2DRotMat(90.0f)  * vert1));
   walls[10].push_back(Wall(calc2DRotMat(90.0f)  * normal12, calc2DRotMat(90.0f)  * vert1, calc2DRotMat(135.0f) * vert1));
   walls[10].push_back(Wall(calc2DRotMat(135.0f) * normal12, calc2DRotMat(135.0f) * vert1, calc2DRotMat(180.0f) * vert1));
   walls[10].push_back(Wall(calc2DRotMat(180.0f) * normal12, calc2DRotMat(180.0f) * vert1, calc2DRotMat(225.0f) * vert1));
   walls[10].push_back(Wall(calc2DRotMat(225.0f) * normal12, calc2DRotMat(225.0f) * vert1, calc2DRotMat(270.0f) * vert1));
   walls[10].push_back(Wall(calc2DRotMat(270.0f) * normal12, calc2DRotMat(270.0f) * vert1, calc2DRotMat(315.0f) * vert1));
   walls[10].push_back(Wall(calc2DRotMat(315.0f) * normal12, calc2DRotMat(315.0f) * vert1, calc2DRotMat(360.0f) * vert1));

   float dx = -800.0f;
   float dy = 300.0f;
   glm::vec2 normalOfBottomPlane = glm::normalize(glm::vec2(dy, -dx));

   // Donward Slope
   walls[11].push_back(Wall(glm::vec2( 0.0f, -1.0f), glm::vec2( -400.0f,  200.0f), glm::vec2(  400.0f,  200.0f))); // Top wall
   walls[11].push_back(Wall(glm::vec2(-1.0f,  0.0f), glm::vec2(  400.0f,  200.0f), glm::vec2(  400.0f, -200.0f))); // Right wall
   walls[11].push_back(Wall(normalOfBottomPlane,     glm::vec2(  400.0f, -200.0f), glm::vec2( -400.0f,  100.0f))); // Bottom wall
   walls[11].push_back(Wall(glm::vec2( 1.0f,  0.0f), glm::vec2( -400.0f,  100.0f), glm::vec2( -400.0f,  200.0f))); // Left wall

   // Upward Slope
   walls[12].push_back(Wall(glm::vec2( 0.0f, -1.0f),                                  glm::vec2( -400.0f,  200.0f), glm::vec2(  400.0f,  200.0f))); // Top wall
   walls[12].push_back(Wall(glm::vec2(-1.0f,  0.0f),                                  glm::vec2(  400.0f,  200.0f), glm::vec2(  400.0f,  100.0f))); // Right wall
   walls[12].push_back(Wall(glm::vec2(-normalOfBottomPlane.x, normalOfBottomPlane.y), glm::vec2(  400.0f,  100.0f), glm::vec2( -400.0f, -200.0f))); // Bottom wall
   walls[12].push_back(Wall(glm::vec2( 1.0f,  0.0f),                                  glm::vec2( -400.0f, -200.0f), glm::vec2( -400.0f,  200.0f))); // Left wall

   // Fall
   //walls[13].push_back(Wall(calcNormal(glm::vec2(  0.0f,  150.0f) - glm::vec2( -200.0f,  300.0f), true), glm::vec2( -200.0f,  300.0f), glm::vec2(  0.0f,   150.0f)));
   //walls[13].push_back(Wall(calcNormal(glm::vec2(-50.0f, -100.0f) - glm::vec2(  300.0f,  100.0f), true), glm::vec2(  300.0f,  100.0f), glm::vec2( -50.0f, -100.0f)));
   //walls[13].push_back(Wall(calcNormal(glm::vec2(-50.0f, -300.0f) - glm::vec2( -300.0f, -150.0f), true), glm::vec2( -300.0f, -150.0f), glm::vec2( -50.0f, -300.0f)));

   // Create the rigid bodies
   std::vector<std::vector<RigidBody2D>> scenes(13);

   //rigidBodies.push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(50.0f, 0.0f), 0.0f, glm::vec2(10.0f, 0.0f), 0.0f));
   //rigidBodies.push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(-50.0f, 0.0f), glm::radians(45.0f), glm::vec2(10.0f, 0.0f), 0.0f));

   //rigidBodies.push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec2(10.0f, 0.0f), 0.0f));
   //rigidBodies.push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(-50.0f, 0.0f), 0.0f, glm::vec2(-10.0f, 0.0f), 0.0f));

   //rigidBodies.push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(50.0f, 0.0f), 0.0f, glm::vec2(10.0f, 0.0f), 0.0f));
   //rigidBodies.push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(-50.0f, 0.0f), 0.0f, glm::vec2(-10.0f, 0.0f), 0.0f));

   // Fast bodies
   //rigidBodies.push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 1.0f, glm::vec2(0.0f, 0.0f), 3.14159265358979323846f / 8, glm::vec2(80.0f, 80.0f), 0.0f));
   //rigidBodies.push_back(RigidBody2D(100.0f, 60.0f, 30.0f, 1.0f, glm::vec2(-50.0f, -50.0f), -3.14159265358979323846f / 6, glm::vec2(-80.0f, -80.0f), 0.0f));

   // Single
   scenes[0].push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(0.0f, 0.0f), -3.14159265358979323846f / 4, glm::vec2(20.0f, 5.0f), 0.0f,  glm::vec3(1.0f, 0.0f, 0.0f))); // Red

   // Pair
   scenes[1].push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(15.0f, 15.0f), -3.14159265358979323846f / 4, glm::vec2(17.0f, 14.0f), 0.0f,  glm::vec3(0.0f, 1.0f, 0.0f))); // Green
   scenes[1].push_back(RigidBody2D(10.0f, 60.0f, 30.0f, 1.0f, glm::vec2(-15.0f, -15.0f), -3.14159265358979323846f / 4, glm::vec2(-20.0f, -12.5f), 0.0f,  glm::vec3(0.0f, 1.0f, 1.0f))); // Turquoise

   // Momentum
   scenes[2].push_back(RigidBody2D(100.0f, 40.0f, 20.0f, 1.0f, glm::vec2(-370.0f, 0.0f), glm::radians(10.0f), glm::vec2(25.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[2].push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(370.0f, 0.0f), glm::radians(25.0f), glm::vec2(-25.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow

   // Torque
   scenes[3].push_back(RigidBody2D(20.0f, 30.0f, 20.0f, 1.0f,  glm::vec2(-370.0f, 82.5f), glm::radians(0.0f), glm::vec2(25.0f, 0.0f), 0.0f,  glm::vec3(0.0f, 1.0f, 1.0f))); // Turquoise
   scenes[3].push_back(RigidBody2D(10.0f, 10.0f, 150.0f, 1.0f, glm::vec2(0.0f, 0.0f), glm::radians(0.0f), glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.0f, 1.0f))); // Pink

   // Plus Sign
   scenes[4].push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(100.0f, 0.0f), 0.0f, glm::vec2(-20.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));  // Red
   scenes[4].push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(-100.0f, 0.0f), 0.0f, glm::vec2(20.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[4].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(0.0f, 100.0f), 0.0f, glm::vec2(0.0f, -20.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[4].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(0.0f, -100.0f), 0.0f, glm::vec2(0.0f, 20.0f), 0.0f, glm::vec3(1.f, 1.0f, 1.0f)));   // White

   // Multiplication Sign
   scenes[5].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(-100.0f, -100.0f), -3.14159265358979323846f / 4, glm::vec2(20.0f, 20.0f), 0.0f,  glm::vec3(0.0f, 0.0f, 1.0f))); // Blue
   scenes[5].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(100.0f, -100.0f),   3.14159265358979323846f / 4, glm::vec2(-20.0f, 20.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))); // Green
   scenes[5].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(100.0f, 100.0f),   -3.14159265358979323846f / 4, glm::vec2(-20.0f, -20.0f), 0.0f,glm::vec3(0.0f, 1.0f, 1.0f))); // Turquoise
   scenes[5].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(-100.0f, 100.0f),   3.14159265358979323846f / 4, glm::vec2(20.0f, -20.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f))); // Pink

   // Star
   // Plus bodies
   scenes[6].push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(100.0f, 0.0f), 0.0f, glm::vec2(-20.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f)));  // Red
   scenes[6].push_back(RigidBody2D(10.0f, 40.0f, 20.0f, 1.0f, glm::vec2(-100.0f, 0.0f), 0.0f, glm::vec2(20.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(0.0f, 100.0f), 0.0f, glm::vec2(0.0f, -20.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(0.0f, -100.0f), 0.0f, glm::vec2(0.0f, 20.0f), 0.0f, glm::vec3(1.f, 1.0f, 1.0f)));   // White
   // X bodies
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(-100.0f, -100.0f), -3.14159265358979323846f / 4, glm::vec2(20.0f, 20.0f), 0.0f,  glm::vec3(0.0f, 0.0f, 1.0f))); // Blue
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(100.0f, -100.0f),   3.14159265358979323846f / 4, glm::vec2(-20.0f, 20.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))); // Green
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(100.0f, 100.0f),   -3.14159265358979323846f / 4, glm::vec2(-20.0f, -20.0f), 0.0f,glm::vec3(0.0f, 1.0f, 1.0f))); // Turquoise
   scenes[6].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(-100.0f, 100.0f),   3.14159265358979323846f / 4, glm::vec2(20.0f, -20.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f))); // Pink

   // Stack

   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -165.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f)));   // Turquoise
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -135.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f)));  // Orange
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -105.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)));   // Green
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -75.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f)));   // Turquoise
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -45.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f)));  // Orange
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f, -15.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)));   // Green
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  15.0f),   0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f)));  // Turquoise
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  45.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f)));  // Orange
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  75.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)));   // Green
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  105.0f),   0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f))); // Turquoise
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  135.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[7].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(0.0f,  165.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)));  // Green

   // Stack Being Hit

   scenes[8].push_back(RigidBody2D(10.0f, 20.0f, 40.0f, 1.0f, glm::vec2(-350.0f, -100.0f), glm::radians(-45.0f), glm::vec2(135.0f, 35.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f))); // Red

   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -190.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -169.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -148.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -127.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -106.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -85.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -64.0f),   0.0f, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -43.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f, -22.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  -1.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  20.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,   glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  41.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,   glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  62.0f),  0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  83.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,   glm::vec3(1.0f, 1.0f, 0.0f)));  // Yellow
   scenes[8].push_back(RigidBody2D(1.0f, 40.0f, 20.0f, 0.1f, glm::vec2(200.0f,  104.0f), 0.0f, glm::vec2(0.0f, 0.0f), 0.0f,  glm::vec3(1.0f, 0.65f, 0.0f))); // Orange

   // Hexagon

   float velocityScaleFactor = 2.0f;
   float minWidth = 14.0f;

   // Middle
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -350.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -300.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -250.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),       velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 250.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 300.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(0.0f + calcRandFloat(-45.0f, 45.0f), 350.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),      velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow

   // Right side
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -300.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -250.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 250.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(100.0f + calcRandFloat(-45.0f, 45.0f), 300.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), -250.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(200.0f + calcRandFloat(-45.0f, 45.0f), 250.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),     velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(300.0f + calcRandFloat(-45.0f, 15.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow

   // Left side
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -300.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -250.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)), velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 250.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-100.0f + calcRandFloat(-45.0f, 45.0f), 300.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), -250.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)), velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-200.0f + calcRandFloat(-45.0f, 45.0f), 250.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), -200.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), -150.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), -100.0f + calcRandFloat(-15.0f, 15.0f)), glm::radians(calcRandFloat(360.0f)),    velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), -50.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), 0.0f    + calcRandFloat(-15.0f, 15.0f)),    glm::radians(calcRandFloat(360.0f)), velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), 50.0f   + calcRandFloat(-15.0f, 15.0f)),   glm::radians(calcRandFloat(360.0f)),  velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), 100.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), 150.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow
   scenes[9].push_back(RigidBody2D(10.0f, calcRandFloat(minWidth, 20.0f), 10.0f, 1.0f, glm::vec2(-300.0f + calcRandFloat(-15.0f, 45.0f), 200.0f  + calcRandFloat(-15.0f, 15.0f)),  glm::radians(calcRandFloat(360.0f)),   velocityScaleFactor * glm::vec2(calcRandFloat(-10.0f, 10.0f), calcRandFloat(-10.0f, 10.0f)), 0.0f, calcRandFloat(1.0f) > 0.5f ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.65f, 0.0f)));  // Yellow

   // Octagon
   scenes[10].push_back(RigidBody2D(1000000.0f, 10.0f, 700.0f, 1.0f, glm::vec2( 0.0f,    0.0f),   -glm::radians(67.5f), glm::vec2( 0.0f,  0.0f), glm::radians(2.5f), glm::vec3(160.0f / 256.0f, 24.0f / 256.0f, 243.0f / 256.0f))); // Pink
   scenes[10].push_back(RigidBody2D(1.0f, 20.0f, 40.0f,  1.0f, glm::vec2(-150.0f, -150.0f), -3.14159265358979323846f / 4, glm::vec2( 0.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f)));    // Turquoise
   scenes[10].push_back(RigidBody2D(1.0f, 20.0f, 40.0f,  1.0f, glm::vec2( 150.0f,  150.0f), -3.14159265358979323846f / 4, glm::vec2( 0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f)));    // Violet

   // Downard Slope
   scenes[11].push_back(RigidBody2D(10.0f, 20.0f, 10.0f, 1.0f, glm::vec2(-380.0f, 150.0f), -3.14159265358979323846f / 4, glm::vec2(10.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f))); // Red

   // Upward Slope
   scenes[12].push_back(RigidBody2D(10.0f, 20.0f, 10.0f, 1.0f, glm::vec2(-380.0f, -150.0f), -3.14159265358979323846f / 4, glm::vec2(85.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f))); // Green

   // Fall
   //scenes[13].push_back(RigidBody2D(10.0f, 20.0f, 10.0f, 1.0f, glm::vec2(20.0f, 200.0f), -3.14159265358979323846f / 4, glm::vec2(0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow

   // Create the world
   mWorld = std::make_shared<World>(std::move(walls), scenes);

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

   mFSM->getCurrentState()->pauseRememberFrames(true);

   return true;
}

void Game::executeGameLoop()
{
   //double currentFrame = 0.0;
   //double lastFrame    = 0.0;
   //float  deltaTime    = 0.0f;

   while (!mWindow->shouldClose() && !mTerminate)
   {
      //currentFrame = glfwGetTime();
      //deltaTime    = static_cast<float>(currentFrame - lastFrame);
      //lastFrame    = currentFrame;

      mFSM->processInputInCurrentState(mTimeStep);

      if (mSimulate)
      {
         int errorCode = mFSM->updateCurrentState(mTimeStep);
         if (errorCode != 0)
         {
            pauseSimulation();
            emit simulationError(errorCode);
         }
      }

      mFSM->renderCurrentState();
   }
}

void Game::changeScene(int index)
{
   bool oldSimulationStatus = mSimulate;

   mSimulate = false;

   if (mRecordGIF && oldSimulationStatus)
   {
      mFSM->getCurrentState()->enableRecording(false);
   }

   mFSM->getCurrentState()->resetMemoryFramebuffer();
   mFSM->getCurrentState()->changeScene(mSceneDimensions[index]);
   mFSM->getCurrentState()->pauseRememberFrames(true);

   mWorld->changeScene(index);

   if (mRecordGIF && oldSimulationStatus)
   {
      mFSM->getCurrentState()->generateGIF();
   }
}

void Game::startSimulation()
{
   mFSM->getCurrentState()->pauseRememberFrames(false);

   if (mRecordGIF)
   {
      mFSM->getCurrentState()->enableRecording(true);
   }

   mSimulate = true;
}

void Game::pauseSimulation()
{
   bool oldSimulationStatus = mSimulate;

   mSimulate = false;

   mFSM->getCurrentState()->pauseRememberFrames(true);

   if (mRecordGIF && oldSimulationStatus)
   {
      mFSM->getCurrentState()->enableRecording(false);
      mFSM->getCurrentState()->generateGIF();
   }
}

void Game::resetSimulation()
{
   bool oldSimulationStatus = mSimulate;

   mSimulate = false;

   mWorld->resetScene();
   mFSM->getCurrentState()->resetMemoryFramebuffer();
   mFSM->getCurrentState()->pauseRememberFrames(true);

   if (mRecordGIF && oldSimulationStatus)
   {
      mFSM->getCurrentState()->enableRecording(false);
      mFSM->getCurrentState()->generateGIF();
   }
}

void Game::changeGravity(int state)
{
   mWorld->setGravityState(state);
}

void Game::changeTimeStep(double timeStep)
{
   mTimeStep = static_cast<float>(timeStep);
}

void Game::changeCoefficientOfRestitution(double coefficientOfRestitution)
{
   mWorld->setCoefficientOfRestitution(static_cast<float>(coefficientOfRestitution));
}

void Game::enableWireframeMode(bool enable)
{
   mFSM->getCurrentState()->enableWireframeMode(enable);
}

void Game::enableRememberFrames(bool enable)
{
   mFSM->getCurrentState()->enableRememberFrames(enable);
}

void Game::changeRememberFramesFrequency(int frequency)
{
   mFSM->getCurrentState()->changeRememberFramesFrequency(frequency);
}

void Game::enableAntiAliasing(bool enable)
{
   mFSM->getCurrentState()->enableAntiAliasing(enable);
}

void Game::changeAntiAliasingMode(int index)
{
   mFSM->getCurrentState()->changeAntiAliasingMode(index);
}

void Game::enableRecordGIF(bool enable)
{
   mRecordGIF = enable;
}

void Game::run()
{
   if (!mInitialized)
   {
      if (initialize("Simulation Viewer"))
      {
         mInitialized = true;
      }
      else
      {
         std::cout << "Error - main - Failed to initialize the game" << "\n";
      }
   }

   if (mInitialized)
   {
      executeGameLoop();
   }

   emit simulatorViewerClosed();
}
