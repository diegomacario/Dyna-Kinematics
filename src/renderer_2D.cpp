#include <glm/glm.hpp>

#include <array>

#include "renderer_2D.h"

Renderer2D::Renderer2D(const std::shared_ptr<Shader>& shader)
   : mShader(shader)
{
   configureVAO();
}

Renderer2D::~Renderer2D()
{
   glDeleteVertexArrays(1, &mVAO);
   glDeleteBuffers(1, &mVBO);
   glDeleteBuffers(1, &mEBO);
}

Renderer2D::Renderer2D(Renderer2D&& rhs) noexcept
   : mShader(std::move(rhs.mShader))
   , mVAO(std::exchange(rhs.mVAO, 0))
   , mVBO(std::exchange(rhs.mVBO, 0))
   , mEBO(std::exchange(rhs.mEBO, 0))
{

}

Renderer2D& Renderer2D::operator=(Renderer2D&& rhs) noexcept
{
   mShader = std::move(rhs.mShader);
   mVAO    = std::exchange(rhs.mVAO, 0);
   mVBO    = std::exchange(rhs.mVBO, 0);
   mEBO    = std::exchange(rhs.mEBO, 0);
   return *this;
}

void Renderer2D::render(const GameObject2D& gameObj2D) const
{
   mShader->use();
   mShader->setMat4("model", gameObj2D.getModelMatrix());

   glActiveTexture(GL_TEXTURE0);
   gameObj2D.getTexture()->bind();

   // Render textured quad
   glBindVertexArray(mVAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

void Renderer2D::configureVAO()
{
   /*
      Origin is at the top left corner because of the way we set up our orthographic projection matrix
      ABC = Counterclockwise
      ADB = Counterclockwise

      (0,0)
        C-----------B
        |         / |
        |        /  |
        |       /   |
        |      /    |
        |  1  /  2  |
        |    /      |
        |   /       |
        |  /        |
        | /         |
        |/          |
        A-----------D
   */

                                          // Pos      // Tex coords
   std::array<float, 24> vertices      = {0.0f, 1.0f, 0.0f, 1.0f,  // A
                                          1.0f, 0.0f, 1.0f, 0.0f,  // B
                                          0.0f, 0.0f, 0.0f, 0.0f,  // C
                                          1.0f, 1.0f, 1.0f, 1.0f}; // D

   std::array<unsigned int, 6> indices = {0, 1, 2,  // Triangle 1
                                          0, 3, 1}; // Triangle 2

   glGenVertexArrays(1, &mVAO);
   glGenBuffers(1, &mVBO);
   glGenBuffers(1, &mEBO);

   glBindVertexArray(mVAO);

   // Load the quad's data into the buffers

   // Positions and texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Set the vertex attribute pointers

   // Positions and texture coordinates
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

   glBindVertexArray(0);
}
