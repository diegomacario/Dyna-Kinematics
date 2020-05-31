#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "game.h"

class MenuState : public State
{
public:

   MenuState(const std::shared_ptr<FiniteStateMachine>& finiteStateMachine,
             const std::shared_ptr<Window>&             window,
             const std::shared_ptr<Camera>&             camera,
             const std::shared_ptr<Renderer2D>&         renderer2D,
             const std::shared_ptr<World>&              world);
   ~MenuState();

   MenuState(const MenuState&) = delete;
   MenuState& operator=(const MenuState&) = delete;

   MenuState(MenuState&&) = delete;
   MenuState& operator=(MenuState&&) = delete;

   void enter() override;
   void processInput(float deltaTime) override;
   int  update(float deltaTime) override;
   void render() override;
   void exit() override;

   void changeScene(const glm::vec2& sceneDimensions) override;

   void resetMemoryFramebuffer() override;
   void pauseRememberFrames(bool pause) override;

   void enableWireframeMode(bool enable) override;
   void enableRememberFrames(bool enable) override;
   void changeRememberFramesFrequency(int frequency) override;
   void enableAntiAliasing(bool enable) override;
   void changeAntiAliasingMode(int index) override;

   void enableRecording(bool record) override;
   void generateGIF() override;

private:

   bool                                mChangeScene;
   glm::vec2                           mCurrentSceneDimensions;

   bool                                mResetMemoryFramebuffer;
   bool                                mPauseRememberFrames;

   bool                                mWireframeModeIsEnabled;
   bool                                mRememberFramesIsEnabled;
   int                                 mRememberFramesFrequency;
   bool                                mRememberFramesStatusChanged;
   int                                 mFrameCounter;
   bool                                mAntiAliasingIsEnabled;
   int                                 mAntiAliasingMode;
   bool                                mAntiAliasingStatusChanged;

   bool                                mRecord;
   int                                 mRecordingDirectory;
   int                                 mRecordedFrameCounter;
   GLubyte*                            mRecordedFrameData;

   std::shared_ptr<FiniteStateMachine> mFSM;

   std::shared_ptr<Window>             mWindow;

   std::shared_ptr<Camera>             mCamera;

   std::shared_ptr<Renderer2D>         mRenderer2D;

   std::shared_ptr<World>              mWorld;
};

#endif
