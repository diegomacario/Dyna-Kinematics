#ifndef PAUSE_STATE_H
#define PAUSE_STATE_H

#include <array>

#include "game.h"

class PauseState : public State
{
public:

   PauseState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
              const std::shared_ptr<Window>&             window,
              const std::shared_ptr<Camera>&             camera,
              const std::shared_ptr<Renderer2D>&         renderer2D);
   ~PauseState() = default;

   PauseState(const PauseState&) = delete;
   PauseState& operator=(const PauseState&) = delete;

   PauseState(PauseState&&) = delete;
   PauseState& operator=(PauseState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   int  update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void resetCamera();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   std::shared_ptr<Camera>             mCamera;

   std::shared_ptr<Renderer2D>         mRenderer2D;
};

#endif
