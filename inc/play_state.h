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
             const std::shared_ptr<Shader>&                 gameObject3DShader,
             const std::shared_ptr<GameObject3D>&           table,
             const std::shared_ptr<Paddle>&                 leftPaddle,
             const std::shared_ptr<Paddle>&                 rightPaddle,
             const std::shared_ptr<Ball>&                   ball,
             const std::shared_ptr<GameObject3D>&           point);
   ~PlayState() = default;

   PlayState(const PlayState&) = delete;
   PlayState& operator=(const PlayState&) = delete;

   PlayState(PlayState&&) = delete;
   PlayState& operator=(PlayState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   void update(float deltaTime) override;
   void render() override;
   void exit() override;

   unsigned int getPointsScoredByLeftPaddle() const;
   unsigned int getPointsScoredByRightPaddle() const;

private:

   void calculateInitialDirectionOfBall();

   bool ballIsOutsideOfHorizontalRange();

   void updateScore();

   void resetScene();

   void resetCamera();

   void playSoundOfCollision();

   void displayScore();

   std::shared_ptr<FiniteStateMachine>     mFSM;

   std::shared_ptr<Window>                 mWindow;

   std::shared_ptr<irrklang::ISoundEngine> mSoundEngine;

   std::shared_ptr<Camera>                 mCamera;

   std::shared_ptr<Shader>                 mGameObject3DShader;

   std::shared_ptr<GameObject3D>           mTable;
   std::shared_ptr<Paddle>                 mLeftPaddle;
   std::shared_ptr<Paddle>                 mRightPaddle;
   std::shared_ptr<Ball>                   mBall;
   std::shared_ptr<GameObject3D>           mPoint;

   bool                                    mBallIsInPlay;
   bool                                    mBallIsFalling;

   unsigned int                            mPointsScoredByLeftPaddle;
   unsigned int                            mPointsScoredByRightPaddle;

   std::array<glm::vec3, 3>                mPositionsOfPointsScoredByLeftPaddle;
   std::array<glm::vec3, 3>                mPositionsOfPointsScoredByRightPaddle;
};

#endif
