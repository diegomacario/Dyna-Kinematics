#include <array>

#include "wall.h"

Wall::Wall(glm::vec2 normal,
           glm::vec2 startPoint,
           glm::vec2 endPoint)
   : mNormal(glm::normalize(normal))
   , mStartPoint(startPoint)
   , mEndPoint(endPoint)
   , mC(-glm::dot(mNormal, (startPoint + endPoint) / 2.0f))
   , mVAO(0)
   , mVBO(0)
{
   configureVAO(startPoint, endPoint);
}

Wall::~Wall()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
}

Wall::Wall(Wall&& rhs) noexcept
   : mNormal(std::exchange(rhs.mNormal, glm::vec2(0.0f)))
   , mStartPoint(std::exchange(rhs.mStartPoint, glm::vec2(0.0f)))
   , mEndPoint(std::exchange(rhs.mEndPoint, glm::vec2(0.0f)))
   , mC(std::exchange(rhs.mC, 0.0f))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
{

}

Wall& Wall::operator=(Wall&& rhs) noexcept
{
   mNormal     = std::exchange(rhs.mNormal, glm::vec2(0.0f));
   mStartPoint = std::exchange(rhs.mStartPoint, glm::vec2(0.0f));
   mEndPoint   = std::exchange(rhs.mEndPoint, glm::vec2(0.0f));
   mC          = std::exchange(rhs.mC, 0.0f);
   mVAO        = std::exchange(rhs.mVAO, 0);
   mVBO        = std::exchange(rhs.mVBO, 0);
   return *this;
}

glm::vec2 Wall::getNormal() const
{
   return mNormal;
}

glm::vec2 Wall::getStartPoint() const
{
   return mStartPoint;
}

glm::vec2 Wall::getEndPoint() const
{
   return mEndPoint;
}

float Wall::getC() const
{
   return mC;
}

void Wall::bindVAO() const
{
   glBindVertexArray(mVAO);
}

void Wall::configureVAO(glm::vec2 startPoint, glm::vec2 endPoint)
{
   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);

   std::array<float, 4> vertices = {startPoint.x, startPoint.y, endPoint.x, endPoint.y};

   std::array<unsigned int, 2> indices = {0, 1};

   // Configure the VAO of the line
   glBindVertexArray(mVAO);

   // Load the line's data into the buffers

   // Positions
   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

   // Set the vertex attribute pointers
   // Positions
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

   glBindVertexArray(0);
}
