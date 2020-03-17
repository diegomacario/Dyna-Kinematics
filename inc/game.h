#ifndef GAME_H
#define GAME_H

#include <irrklang/irrKlang.h>

#include "world.h"
#include "model.h"
#include "renderer_2D.h"
#include "camera.h"
#include "window.h"
#include "state.h"
#include "finite_state_machine.h"

class Game
{
public:

   Game();
   ~Game();

   Game(const Game&) = delete;
   Game& operator=(const Game&) = delete;

   Game(Game&&) = delete;
   Game& operator=(Game&&) = delete;

   bool  initialize(const std::string& title);
   void  executeGameLoop();

private:

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
