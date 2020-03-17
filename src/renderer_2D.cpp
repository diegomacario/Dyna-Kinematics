#include <glm/glm.hpp>

#include <array>

#include "renderer_2D.h"

Renderer2D::Renderer2D(const std::shared_ptr<Shader>& texShader,
                       const std::shared_ptr<Shader>& colorShader,
                       const std::shared_ptr<Shader>& lineShader)
   : mTexShader(texShader)
   , mColorShader(colorShader)
   , mLineShader(lineShader)
{
   configureVAOs();
}

Renderer2D::~Renderer2D()
{
   glDeleteVertexArrays(1, &mTexturedQuadVAO);
   glDeleteVertexArrays(1, &mColoredQuadVAO);
   glDeleteBuffers(1, &mQuadVBO);
   glDeleteBuffers(1, &mQuadEBO);
}

Renderer2D::Renderer2D(Renderer2D&& rhs) noexcept
   : mTexShader(std::move(rhs.mTexShader))
   , mColorShader(std::move(rhs.mColorShader))
   , mLineShader(std::move(rhs.mLineShader))
   , mTexturedQuadVAO(std::exchange(rhs.mTexturedQuadVAO, 0))
   , mColoredQuadVAO(std::exchange(rhs.mColoredQuadVAO, 0))
   , mQuadVBO(std::exchange(rhs.mQuadVBO, 0))
   , mQuadEBO(std::exchange(rhs.mQuadEBO, 0))
{

}

Renderer2D& Renderer2D::operator=(Renderer2D&& rhs) noexcept
{
   mTexShader       = std::move(rhs.mTexShader);
   mColorShader     = std::move(rhs.mColorShader);
   mLineShader      = std::move(rhs.mLineShader);
   mTexturedQuadVAO = std::exchange(rhs.mTexturedQuadVAO, 0);
   mColoredQuadVAO  = std::exchange(rhs.mColoredQuadVAO, 0);
   mQuadVBO         = std::exchange(rhs.mQuadVBO, 0);
   mQuadEBO         = std::exchange(rhs.mQuadEBO, 0);
   return *this;
}

void Renderer2D::renderTexturedQuad(const GameObject2D& gameObj2D) const
{
   mTexShader->use();
   mTexShader->setMat4("model", gameObj2D.getModelMatrix());

   glActiveTexture(GL_TEXTURE0);
   gameObj2D.getTexture()->bind();

   // Render textured quad
   glBindVertexArray(mTexturedQuadVAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

void Renderer2D::renderColoredQuad(const GameObject2D& gameObj2D) const
{
   mColorShader->use();
   mColorShader->setMat4("model", gameObj2D.getModelMatrix());

   // Render quad
   glBindVertexArray(mColoredQuadVAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

void Renderer2D::renderRigidBody(const RigidBody2D& rigidBody2D) const
{
   mColorShader->use();
   mColorShader->setMat4("model", rigidBody2D.getModelMatrix());

   // Render quad
   glBindVertexArray(mColoredQuadVAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
   glBindVertexArray(0);
}

void Renderer2D::renderLine(const Wall& wall) const
{
   mLineShader->use();

   // Render line
   wall.bindVAO();
   glDrawArrays(GL_LINES, 0, 2);
   glBindVertexArray(0);
}

void Renderer2D::configureVAOs()
{
   /*
      Origin is at the top left corner because of the way we set up our orthographic projection matrix
      ABC = Counterclockwise
      ADB = Counterclockwise

        C-----------B
        |          /|
        |         / |
        |        /  |
        |       /   |
        |      /    |
        | 1 (0,0) 2 |
        |    /      |
        |   /       |
        |  /        |
        | /         |
        |/          |
        A-----------D
   */

                                          // Pos        // Tex coords
   std::array<float, 24> vertices      = {-0.5f, -0.5f, 0.0f, 1.0f,  // A
                                           0.5f,  0.5f, 1.0f, 0.0f,  // B
                                          -0.5f,  0.5f, 0.0f, 0.0f,  // C
                                           0.5f, -0.5f, 1.0f, 1.0f}; // D

   std::array<unsigned int, 6> indices = {0, 1, 2,  // Triangle 1
                                          0, 3, 1}; // Triangle 2

   glGenVertexArrays(1, &mTexturedQuadVAO);
   glGenVertexArrays(1, &mColoredQuadVAO);
   glGenBuffers(1, &mQuadVBO);
   glGenBuffers(1, &mQuadEBO);

   // 1) Configure the VAO of the textured quad
   glBindVertexArray(mTexturedQuadVAO);

   // Load the quad's data into the buffers

   // Positions and texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Set the vertex attribute pointers

   // Positions and texture coordinates
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

   // 2) Configure the VAO of the non-textured quad
   glBindVertexArray(mColoredQuadVAO);

   // Load the quad's data into the buffers

   // Positions
   // The VBO has already been filled
   // It contains the texture coordinates, which the non-textured quad doesn't need
   // We will ignore them when we configure the vertex attributes
   glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
   // Indices
   // The EBO has already been filled
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadEBO);

   // Set the vertex attribute pointers
   // Positions
   // We ignore the texture coordinates by taking advantage of the stride
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

   glBindVertexArray(0);
}
