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
              const std::shared_ptr<Shader>&             gameObject3DShader,
              const std::shared_ptr<GameObject3D>&       table,
              const std::shared_ptr<Paddle>&             leftPaddle,
              const std::shared_ptr<Paddle>&             rightPaddle,
              const std::shared_ptr<Ball>&               ball,
              const std::shared_ptr<GameObject3D>&       point);
   ~PauseState() = default;

   PauseState(const PauseState&) = delete;
   PauseState& operator=(const PauseState&) = delete;

   PauseState(PauseState&&) = delete;
   PauseState& operator=(PauseState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   void resetCamera();

   void displayScore();

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   std::shared_ptr<Camera>             mCamera;

   std::shared_ptr<Shader>             mGameObject3DShader;

   std::shared_ptr<GameObject3D>       mTable;
   std::shared_ptr<Paddle>             mLeftPaddle;
   std::shared_ptr<Paddle>             mRightPaddle;
   std::shared_ptr<Ball>               mBall;
   std::shared_ptr<GameObject3D>       mPoint;

   unsigned int                        mPointsScoredByLeftPaddle;
   unsigned int                        mPointsScoredByRightPaddle;

   std::array<glm::vec3, 3>            mPositionsOfPointsScoredByLeftPaddle;
   std::array<glm::vec3, 3>            mPositionsOfPointsScoredByRightPaddle;
};

#endif
