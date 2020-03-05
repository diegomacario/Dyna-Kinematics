#ifndef WIN_STATE_H
#define WIN_STATE_H

#include "game.h"

class WinState : public State
{
public:

   WinState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
            const std::shared_ptr<Window>&             window,
            const std::shared_ptr<Camera>&             camera,
            const std::shared_ptr<Shader>&             gameObject3DExplosiveShader,
            const std::shared_ptr<Ball>&               ball,
            const std::shared_ptr<GameObject3D>&       leftPaddleWins,
            const std::shared_ptr<GameObject3D>&       rightPaddleWins);
   ~WinState() = default;

   WinState(const WinState&) = delete;
   WinState& operator=(const WinState&) = delete;

   WinState(WinState&&) = delete;
   WinState& operator=(WinState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

private:

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   std::shared_ptr<Camera>             mCamera;

   std::shared_ptr<Shader>             mGameObject3DExplosiveShader;

   std::shared_ptr<Ball>               mBall;
   std::shared_ptr<GameObject3D>       mLeftPaddleWins;
   std::shared_ptr<GameObject3D>       mRightPaddleWins;

   glm::vec3                           mCameraPosition;
   glm::vec3                           mCameraTarget;
   glm::vec3                           mCameraUp;
   glm::vec3                           mCameraRight;

   float                               mIdleOrbitalAngularVelocity;

   double                              mTimeWhenExplosionShouldBegin;
   bool                                mExplode;
   float                               mSpeedOfExplodingFragments;
   float                               mDistanceTravelledByExplodingFragments;

   enum class Winner : unsigned int
   {
      leftPaddleWon  = 0,
      rightPaddleWon = 1,
   };

   Winner                              mWinner;

   double                              mTimeWhenWinnerIsFirstDisplayed;
   bool                                mDisplayWinner;
};

#endif
