<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Title_Long.gif"/>
</p>

# Dyna-Kinematics

A 2D rigid-body dynamics simulator with some cool features for generating beautiful animations.

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Cool_Dino.gif"/>
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/readme_images/Simulation_Controller_Dino.PNG"/>
 <p align="center">
  <em>The two components that make up this project: the simulation viewer (left) and the simulation controller (right).</em>
 </p>
</p>

**Note**: View this readme [here](https://github.com/diegomacario/Dyna-Kinematics/blob/master/README.md) instead of in the main page of this repository. On some systems Github resizes the GIFs in the main page, which makes them look blurry.

## Motivation

When I was in university I took a class on numerical methods. In that class I was taught many different algorithms for fitting curves, interpolating polynomials and splines, and numerically integrating and differentiating.

The funny thing is that I was taught how to do all those things by hand. I was never asked to implement any of those algorithms in a computer, which felt wrong because they are clearly designed to be executed by a computer.

Since then I have wanted to rectify that wrong by taking what I learned to do by hand and using it to build something cool. The result of that desire is this project, which illustrates the basics of:

- Numerical integration.
- 2D rigid-body dynamics.
- 2D collision detection and response.

## Technical details

In its current form, this project consists of a simulation controller and a simulation viewer.

The simulation controller allows users to:

- Select a scene from a set of hardcoded scenes.
- Play, pause, reset and record simulations as GIFs.
- Change certain simulation and display settings.

The simulation viewer displays simulations in real-time and it can be resized to fit the bounds of a scene.

The libraries used by this project and their purposes are the following:

- [Qt](https://www.qt.io/) is used for the UI of the simulation controller.
- [GLFW](https://www.glfw.org/) is used for the window of the simulation viewer.
- [GLAD](https://glad.dav1d.de/) is used to load pointers to OpenGL functions.
- [GLM](https://glm.g-truc.net/0.9.9/index.html) is used to perform 3D math.
- [stb_image_write](https://github.com/nothings/stb) is used to save frames as PNGs.
- [FFmpeg](https://ffmpeg.org/) is used to generate GIFs.

For information on the techniques used by this project to detect and resolve collisions, see the "Physics" section at the end of this document.

## Evolution

Below you will find a description of how this simulator evolved over time, each step illustrated with GIFs recorded in the simulator itself.

Note that in many examples I present the same GIF twice. The first time with the "Remember Frames" feature disabled, and the second time with the same feature enabled. Being able to visualize trajectories is my favorite thing about this project. The results can be very beautiful.

### 1) Body-wall collisions

The first step in the development process was to implement support for collisions between bodies and walls. This type of collision is a lot simpler than collisions between bodies because walls are not affected in any way by the impact. In the code, a wall is treated as a body with an infinite mass, which simplifies the collision response equations significantly.

In the simulation below, notice how the velocity and angular velocity of the body change depending on the way it hits the walls. That's the magic of rigid-body dynamics. The point of contact and linear and angular effects are taken into consideration to produce results that look natural.

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Single_No_Rem_Long.gif"/>
 <p align="center">
  <em>A single body floating in the vacuum of space.</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Single_Rem.gif"/>
 <p align="center">
  <em>This simulation reminds me of Saul Bass' beautiful <a href="http://annyas.com/saul-bass-vertigo-movie-poster-design/">poster</a> for Vertigo.</em>
 </p>
</p>

### 2) Body-body collisions

The second step in the development process was to implement support for collisions between bodies. This type of collision can occur in two different ways: between two vertices or between a vertex and an edge.

An important part of resolving a collision is knowing what your collision normal is. In a vertex-edge collision, the collision normal is simply the normal of the edge. But what about vertex-vertex collisions? Vertices are simply points, so they don't have normals. There are many ways to calculate an appropriate normal for this type of collision. In my case, I chose a really simple one: the collision normal is the line that connects the centers of mass of the two colliding bodies. This is a deviation from reality, but it produces good looking results:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Pair_No_Rem.gif"/>
 <p align="center">
  <em>Two bodies of equal mass floating in the vacuum of space.</em>
 </p>
</p>

### 3) Momentum and torque

The simulations so far probably look a bit cartoony to you. Are we really simulating physics here? Or are we just reflecting velocity vectors when bodies collide to get good looking results?

A cool way to confirm that we are simulating real physics is to visualize momentum and torque in action. This is possible because the collision response equations take a body's mass, center of mass and moment of inertia into consideration.

In the simulation below you can see momentum in action:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Momentum_No_Rem_Short.gif"/>
 <p align="center">
  <em>A 100 kilogram body (orange) colliding with a 10 kilogram body (yellow).</em>
 </p>
</p>

Since the mass of the orange body is much greater than the mass of the yellow body, its momentum is much greater too. Because of this, the orange body is barely affected by the collision while the yellow one reverses its direction.

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Momentum_Rem_Short.gif"/>
 <p align="center">
  <em>A stoppable force meets a movable object.</em>
 </p>
</p>

As for torque, you can see it in action in the simulation below:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Torque_No_Rem_Short.gif"/>
 <p align="center">
  <em>A body (pink) getting hit by another body (turquoise) as far away from its center of mass as possible.</em>
 </p>
</p>

The center of mass of the pink body is halfway between its ends. The point of contact between the two bodies is at its upper end, or in other words, as far way from its center of mass as possible, which means that the torque that is applied to it by the collision is the maximum possible. That torque translates into the maximum possible angular velocity, which causes it to rotate quickly around its center of mass.

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Torque_Rem_Short.gif"/>
 <p align="center">
  <em>This simulation has a Miami Vice feel to it.</em>
 </p>
</p>

### 4) Gravity

It is time for a confession: all the simulations that I have showed you so far only made use of impulsive forces.

What is an impulsive force? You can think of it as a force that's so powerful, that even when it's integrated over an infinitesimal period of time it still causes a change in the momentum of a body.

In this simulator, impulsive forces are applied when a collision occurs to instantaneously change the linear and angular velocities of the colliding bodies, so as to keep them from penetrating.

But why do we need impulsive forces at all? Why can't we just apply a force and integrate it over time to resolve a collision? The problem is that when we detect a collision, the two rigid-bodies involved in that collision are almost touching since they are within the collision threshold, which is a really small distance. And in this simulator, rigid-bodies are perfectly rigid, which means that they are impenetrable. So how do we keep them from penetrating? We can't apply a force and integrate it over time because we literally don't have enough time to do that. The two bodies are almost touching, so if we take that approach they will certainly penetrate. That's why we need a discontinuous change in their velocities, which can only be achieved by applying a powerful force over an infinitesimal period of time, that is, an impulsive force.

So now that you know that I have been tricking you with impulsive forces this whole time, you might be wondering if this simulator can actually integrate forces over time. The answer is yes! It uses the classical 4th order Runge-Kutta method to integrate any force you want. The simulations below show gravity in action:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Downward_Slope_No_Rem_Short.gif"/>
 <p align="center">
  <em>A body rolling down a hill thanks to gravity.</em>
 </p>
</p>

Note that the body doesn't rest at the bottom of the hill because I configured the simulation so that no energy is lost when a collision occurs. I did this using the coefficient of restitution (COR), which models how much of the incoming energy is disipated during a collision. By setting the COR equal to 1, I made all the collisions perfectly elastic, which means that no energy is lost when they occur. If I had set the COR equal to 0, all the incoming energy would have been lost in the first collision, which would have been a perfectly plastic collision. Anything between 0 and 1 varies the amount of energy that is lost.

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Downward_Slope_Rem_Short.gif"/>
 <p align="center">
  <em>When I started working on this project this was the image that was always in my head. A red body rolling down a hill tracing its trajectory. It felt great when I saw it on the screen for the first time.</em>
 </p>
</p>

And below are similar simulations, but this time going uphill:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Upward_Slope_No_Rem.gif"/>
 <p align="center">
  <em>A body bouncing up a hill against gravity.</em>
 </p>
</p>

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Upward_Slope_Rem.gif"/>
 <p align="center">
  <em>This simulation is super bouncy because all of its collisions are perfectly elastic.</em>
 </p>
</p>

### 5) Simultaneous collisions

The last and most challenging step in the development process was to implement support for resolving multiple collisions in a single timestep.

To understand what "resolving multiple collisions in a single timestep" means, let's first take a step-by-step look at how the simulations that I have shown you so far have been executed:

1. We start a simulation by advancing it by a single timestep. If, for example, the timestep is 20 milliseconds and a body has a speed of 10 meters/second and no acceleration, then that body moves forward by 0.2 meters.
2. We check if any of the bodies in the scene is penetrating another body or the walls. If yes, then we go back in time the same amount we advanced in the previous step, we halve the timestep, and we advance the simulation again. Using the previous example, this means that our timestep would become 10 milliseconds, and our body would only move forward by 0.1 meters.
3. We repeat the previous step until no penetration is occurring. You can think of this process as a binary search for the amount of time that we can advance that results in a scene where no penetration is occurring.
4. Now that we are certain that no penetration is occurring, we check if any of the bodies in the scene is colliding with another body or with the walls. If yes, then **we resolve the first collision we find and we ignore any other collisions**.
5. We advance the simulation by what remains of the timestep and we go back to step two. Using the previous example, this means that we advance the simulation by 10 milliseconds.
6. Once the simulation has been advanced by an entire timestep, we render a frame and we go back to step one.

The key thing to note is highlighted in step four: **we resolve the first collision we find and we ignore any other collisions**. That is what I set out to change in this final step of the development process. I wanted the simulator to be able to resolve any collisions that occur simultaneously in a scene. This is tricky because there are many situations to account for, like for example:

- What if two adjacent vertices of a body collide with a wall at the same time? This is what is commonly referred to as an edge-edge collision.
- What if three of the vertices of a body each collide with a different body?

The possibilities are endless. My solution to this problem involves resolving collisions independently and then combining the results by calculating average linear and angular kinetic energies. It is a complex process that I plan to explain in a separate document. I will update this section once I do.

For now, I can at least show you some of the cool possibilities that this feature opens up:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Plus_Sign_2.gif"/>
 <p align="center">
  <em>Simultaneous collisions in the vacuum of space.</em>
 </p>
</p>

The simulation above resolves eight simultaneous body-body collisions when the bodies meet at the center, and eight simultaneous body-wall collisions when they reach the edges. Without the ability to resolve multiple collisions in a single timestep the bodies would spin out of control.

The simulation below is very similar to the one above, but it shows how vertex-edge collisions can add up to look like edge-edge collisions:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Multiplication_Sign_2.gif"/>
 <p align="center">
  <em>The total energy of this system is always the same.</em>
 </p>
</p>

My favorite example of simultaneous collisions is a stack of bodies:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Stack_CR_0_5.gif"/>
 <p align="center">
  <em>A stack of bodies settling down thanks to gravity and a COR of 0.5.</em>
 </p>
</p>

Without the ability to resolve multiple collisions in a single timestep this simulation would get stuck because it would only resolve one collision and allow all the other colliding bodies to penetrate.

By setting the COR to 1.0, one gets a really funny result: a stack of bodies that never settles down.

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Stack_CR_1_0.gif"/>
 <p align="center">
  <em>Who wants ice cream? Me! Me! Me!</em>
 </p>
</p>

And if you are going to simulate a stack of bodies, you might as well throw something at it:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Stack_Hit_No_Rem_Long.gif"/>
 <p align="center">
  <em>A stack of bodies being hit by another body.</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Stack_Hit_Rem_Long.gif"/>
 <p align="center">
  <em>Remember frames? More like remember flames!</em>
 </p>
</p>

### 6) Fun experiments

In this final section I want to step away from the technical details and just show you some fun simulations, just to remind you that physics simulation is a tool that you can use to bring creative ideas to life.

First up is a simulation in the vacuum of space in which I mirrored the positions of the bodies in the scene and their velocities to create symmetry:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Symmetry_No_Rem_Long.gif"/>
 <p align="center">
  <em>A dance of symmetry.</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Symmetry_Rem_Long.gif"/>
 <p align="center">
  <em>Modern art?</em>
 </p>
</p>

Second up is a simulation of eighty bees inside a behive:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Hexagon_No_Rem.gif"/>
 <p align="center">
  <em>Beloved bees. The pillars of our societies.</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Hexagon_Rem_Short.gif"/>
 <p align="center">
  <em>Thankfully, this simulation is not accurate. Bees fly much better than this.</em>
 </p>
</p>

Third up is a simulation in which the body in the middle behaves like a wall:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Octagon_No_Rem.gif"/>
 <p align="center">
  <em>The purple body weighs 1,000,000 kilograms while the others weigh 1 kilogram. Because of this, the purple body is not affected in any way by the collisions with the other bodies.</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Octagon_Rem.gif"/>
 <p align="center">
  <em>This is my favorite simulation in this entire document. I love how all the remembered frames of the purple body line up perfectly.</em>
 </p>
</p>

Finally, below is a simulation that uses some experimental code that I haven't finished perfecting yet. That same experimental code was used to generate the title GIF and the brachiosaurus GIF that you can find at the top of this document.

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Fall_No_Rem_Short.gif"/>
 <p align="center">
  <em>The walls in this simulation were drawn by hand using a Wacom tablet and 3ds Max. Their vertices and normals were then imported into the simulator to produce this GIF. (Thanks to my sister Ana Carina for teaching me how to use her Wacom tablet).</em>
 </p>
</p>

Below is the same simulation but with the "Remember Frames" feature enabled:

<p align="center">
 <img src="https://github.com/diegomacario/2D-Rigid-Body-Simulator/blob/master/GIFs/Fall_Rem_Short.gif"/>
 <p align="center">
  <em>This simulation makes me think about the possibilities that open up when you start simulating physics in a platformer, although it also reminds me of an Albino Burmese Python.</em>
 </p>
</p>

## Physics

My objective with the core of this project, which is the code that detects collisions and resolves them, was to keep it as simple as possible. Because of this, I sacrificed performance in order to avoid anything that would be too difficult to understand. The most relevant aspects of the core are the following:

- Each body is represented as an oriented bounding box (OBB).
- Collisions between OBBs and corners formed by walls are not supported. Because of this, walls must form convex shapes to guarantee that unsupported collisions can never occur.
- Collisions between OBBs are separated into two categories: vertex-vertex and vertex-edge collisions. For vertex-vertex collisions, the line that connects the centers of mass of the colliding bodies is used as the collision normal.
- Collisions are detected by solving simple geometry problems. Nothing complicated like the separating-axis theorem is used. Vertices are simply projected onto normals or edges to see if they are penetrating or not, and their relative velocities are calculated to see if they are colliding or not.
- Integration is performed using the classical 4th order Runge-Kutta method.
- The timestep is fixed. This means that if you set the timestep to 20 milliseconds, your simulation will be advanced by 20 milliseconds every time a frame is rendered, regardless of how long it took to render each frame. This is generally frowned upon because it creates a problem: if your computer is able to render quickly, simulations will look fast on it, but if it's only able to render slowly, simulations will look slow on it. Despite that, I still decided to keep the timestep fixed because this allows users to get beautiful results using the "Remember Frames" feature. If the number of steps taken in a frame was allowed to change depending on how long it took to render that frame (see [this](https://gafferongames.com/post/fix_your_timestep/) article for more information on this technique), then the "Remember Frames" feature wouldn't produce perfectly spaced results.
- Bodies are not allowed to penetrate. This is a decision that I made early on in the development process because I couldn't find information on how to resolve a collision if two bodies were already penetrating. In this simulator's code, when two bodies are penetrating, the timestep is subdivided until they are not. The problem with this technique is that there are situations in which the timestep can be subdivided indefinitely and still not get bodies to stop penetrating. To keep the simulator from entering an infinite loop in those situations, I have implemented a simple rule: if the timestep is subdivided until it's smaller than 1 microsecond, then the simulation is stopped and an "Unresolvable Penetration" error is displayed. The sad thing is that I recently found [this](https://github.com/erincatto/box2d-lite/blob/master/docs/GDC2006_Catto_Erin_PhysicsTutorial.pdf) presentation that explains how to resolve a collision if two bodies are already penetrating. You live and you learn.

## Learning resources

I always try to keep the code of my open source projects clean so that it can be useful to others. Unfortunately, that's not the case with this one.

I got carried away exploring new ideas and libraries, and I ended up neglecting clarity and organization in the process. The end result is perfectly stable and really fun to use, but not something worth studying yet.

Once I clean up the code and add a feature that allows users to describe scenes in text files using a simple scene description language (so that they don't have to modify the code to create new scenes), I will update this readme with instructions on how to build this project.

For now, if you are interesting in learning more about rigid-body dynamics and physics simulation in general, I recommend you get started here:

- Chris Hecker's Game Developer Magazine [articles](http://www.chrishecker.com/Rigid_Body_Dynamics#Physics_Articles) on rigid-body dynamics are amazing. They will teach the basics and they are super easy to read.
- Erin Catto's [Box2D-Lite](https://github.com/erincatto/box2d-lite) physics engine, which is the learning version of his production-ready engine [Box2D](https://github.com/erincatto/box2d), will teach you modern techniques for detecting and resolving collisions.

## Dedication

This one is for my mom and dad. Thank you for taking care of me during the pandemic.
