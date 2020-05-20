#ifndef GAME_H
#define GAME_H

#include <QThread>
#include <irrklang/irrKlang.h>

#include "rigid_body_simulator.h"
#include "world.h"
#include "model.h"
#include "renderer_2D.h"
#include "camera.h"
#include "window.h"
#include "state.h"
#include "finite_state_machine.h"

class Game : public QThread
{
   Q_OBJECT

public:

   Game(QObject* parent);
   ~Game();

   Game(const Game&) = delete;
   Game& operator=(const Game&) = delete;

   Game(Game&&) = delete;
   Game& operator=(Game&&) = delete;

   bool  initialize(const std::string& title);
   void  executeGameLoop();

public slots:

   void changeScene(int index);

   void startSimulation();
   void pauseSimulation();
   void resetSimulation();

   void changeGravity(int state);

   void changeTimeStep(double timeStep);
   void changeCoefficientOfRestitution(double coefficientOfRestitution);

   void enableWireframeMode(bool enable);
   void enableRememberFrames(bool enable);
   void changeRememberFramesFrequency(int frequency);
   void enableAntiAliasing(bool enable);
   void changeAntiAliasingMode(int index);

signals:

   void simulationError(int errorCode);

   void simulatorViewerClosed();

private:

   void  run() override;

   bool                                    mInitialized;
   bool                                    mSimulate;
   bool                                    mTerminate;
   float                                   mTimeStep;
   std::vector<glm::vec2>                  mSceneDimensions;

   std::shared_ptr<FiniteStateMachine>     mFSM;

   std::shared_ptr<Window>                 mWindow;

   std::shared_ptr<irrklang::ISoundEngine> mSoundEngine;

   std::shared_ptr<Camera>                 mCamera;

   std::shared_ptr<Renderer2D>             mRenderer2D;

   ResourceManager<Model>                  mModelManager;
   ResourceManager<Texture>                mTextureManager;
   ResourceManager<Shader>                 mShaderManager;

   std::shared_ptr<World>                  mWorld;
};

#endif
