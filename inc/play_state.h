#ifndef PLAY_STATE_H
#define PLAY_STATE_H

#include <array>

#include "game.h"

class PlayState : public State
{
public:

   PlayState(const std::shared_ptr<FiniteStateMachine>&     finiteStateMachine,
             const std::shared_ptr<Window>&                 window,
             const std::shared_ptr<irrklang::ISoundEngine>& soundEngine,
             const std::shared_ptr<Camera>&                 camera,
             const std::shared_ptr<Renderer2D>&             renderer2D);
   ~PlayState() = default;

   PlayState(const PlayState&) = delete;
   PlayState& operator=(const PlayState&) = delete;

   PlayState(PlayState&&) = delete;
   PlayState& operator=(PlayState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   int  update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void resetCamera();

   std::shared_ptr<FiniteStateMachine>     mFSM;

   std::shared_ptr<Window>                 mWindow;

   std::shared_ptr<irrklang::ISoundEngine> mSoundEngine;

   std::shared_ptr<Camera>                 mCamera;

   std::shared_ptr<Renderer2D>             mRenderer2D;
};

#endif
