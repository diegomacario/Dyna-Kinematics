#ifndef STATE_H
#define STATE_H

#include <glm/glm.hpp>

class State
{
public:

   State() = default;
   virtual ~State() = default;

   State(const State&) = delete;
   State& operator=(const State&) = delete;

   State(State&&) = delete;
   State& operator=(State&&) = delete;

   virtual void enter() = 0;
   virtual void processInput(float deltaTime) = 0;
   virtual int  update(float deltaTime) = 0;
   virtual void render() = 0;
   virtual void exit() = 0;

   virtual void changeScene(const glm::vec2& sceneDimensions) {};

   virtual void resetMemoryFramebuffer() {};
   virtual void pauseRememberFrames(bool pause) {};

   virtual void enableWireframeMode(bool enable) {}
   virtual void enableRememberFrames(bool enable) {}
   virtual void changeRememberFramesFrequency(int frequency) {}
   virtual void enableAntiAliasing(bool enable) {}
   virtual void changeAntiAliasingMode(int index) {}

   virtual void enableRecording(bool enable) {};
};

#endif
