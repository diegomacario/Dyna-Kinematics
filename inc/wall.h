#ifndef WALL_H
#define WALL_H

#include <glm/glm.hpp>

#include "shader.h"

// A wall is defined using the 2D plane equation:
// ax + by + c = 0
// Where [a, b] is the normal of the plane
// And c is:
// -dot([a, b], (x0, y0))
// Where (x0, y0) is any point on the plane

class Wall
{
public:

   Wall(glm::vec2 normal,
        glm::vec2 startPoint,
        glm::vec2 endPoint);
   ~Wall();

   Wall(const Wall&) = delete;
   Wall& operator=(const Wall&) = delete;

   Wall(Wall&& rhs) noexcept;
   Wall& operator=(Wall&& rhs) noexcept;

   glm::vec2 getNormal() const;
   float     getC() const;

   void      bindVAO() const;

private:

   void      configureVAO(glm::vec2 startPoint,
                          glm::vec2 endPoint);

   glm::vec2    mNormal;
   float        mC;

   unsigned int mVAO;
   unsigned int mVBO;
};

#endif
