#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

#include "renderer_2D.h"

Renderer2D::Renderer2D(const std::shared_ptr<Shader>& texShader,
                       const std::shared_ptr<Shader>& colorShader,
                       const std::shared_ptr<Shader>& lineShader,
                       const glm::vec2&               currentSceneDimensions,
                       unsigned int                   scaleFactor)
   : mTexShader(texShader)
   , mColorShader(colorShader)
   , mLineShader(lineShader)
   , mCurrentSceneDimensions(currentSceneDimensions)
   , mScaleFactor(scaleFactor)
{
   configureVAOs();
   configureRealVAOs();
}

Renderer2D::~Renderer2D()
{
   glDeleteVertexArrays(1, &mTexturedQuadVAO);
   glDeleteVertexArrays(1, &mColoredQuadVAO);
   glDeleteBuffers(1, &mQuadVBO);
   glDeleteBuffers(1, &mQuadEBO);

   glDeleteVertexArrays(1, &mRealTexturedQuadVAO);
   glDeleteVertexArrays(1, &mRealColoredQuadVAO);
   glDeleteBuffers(1, &mRealQuadVBO);
   glDeleteBuffers(1, &mRealQuadEBO);
}

Renderer2D::Renderer2D(Renderer2D&& rhs) noexcept
   : mTexShader(std::move(rhs.mTexShader))
   , mColorShader(std::move(rhs.mColorShader))
   , mLineShader(std::move(rhs.mLineShader))

   , mTexturedQuadVAO(std::exchange(rhs.mTexturedQuadVAO, 0))
   , mColoredQuadVAO(std::exchange(rhs.mColoredQuadVAO, 0))
   , mQuadVBO(std::exchange(rhs.mQuadVBO, 0))
   , mQuadEBO(std::exchange(rhs.mQuadEBO, 0))

   , mRealTexturedQuadVAO(std::exchange(rhs.mRealTexturedQuadVAO, 0))
   , mRealColoredQuadVAO(std::exchange(rhs.mRealColoredQuadVAO, 0))
   , mRealQuadVBO(std::exchange(rhs.mRealQuadVBO, 0))
   , mRealQuadEBO(std::exchange(rhs.mRealQuadEBO, 0))
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

   mRealTexturedQuadVAO = std::exchange(rhs.mRealTexturedQuadVAO, 0);
   mRealColoredQuadVAO  = std::exchange(rhs.mRealColoredQuadVAO, 0);
   mRealQuadVBO         = std::exchange(rhs.mRealQuadVBO, 0);
   mRealQuadEBO         = std::exchange(rhs.mRealQuadEBO, 0);
   return *this;
}

void Renderer2D::renderRigidBody(const RigidBody2D& rigidBody2D, bool wireframe) const
{
   mColorShader->use();
   mColorShader->setMat4("model", rigidBody2D.getModelMatrix(current));
   mColorShader->setVec3("color", rigidBody2D.mColor);

   // Render colored quad
   if (wireframe)
   {
      glBindVertexArray(mRealColoredQuadVAO);
      //glLineWidth(2);
      glViewport(0, 0, mScaleFactor * mCurrentSceneDimensions.x, mScaleFactor * mCurrentSceneDimensions.y); // This is here because of a bug in GLFW that can only be seen in certain versions of macOS
      glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, 0);
      //glLineWidth(1);
      glBindVertexArray(0);
   }
   else
   {
      glBindVertexArray(mColoredQuadVAO);
      glViewport(0, 0, mScaleFactor * mCurrentSceneDimensions.x, mScaleFactor * mCurrentSceneDimensions.y); // This is here because of a bug in GLFW that can only be seen in certain versions of macOS
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
   }
}

void Renderer2D::renderLine(const Wall& wall) const
{
   mLineShader->use();

   // Render line
   wall.bindVAO();
   //glLineWidth(2);
   glViewport(0, 0, mScaleFactor * mCurrentSceneDimensions.x, mScaleFactor * mCurrentSceneDimensions.y); // This is here because of a bug in GLFW that can only be seen in certain versions of macOS
   glDrawArrays(GL_LINES, 0, 2);
   //glLineWidth(1);
   glBindVertexArray(0);
}

void Renderer2D::updateOrthographicProjection(float width, float height) const
{
   glm::mat4 orthoProj = glm::ortho(-width / 2,  // Left
                                     width / 2,  // Right
                                    -height / 2, // Bottom
                                     height / 2, // Top
                                    -1.0f,       // Near
                                     1.0f);      // Far

   mTexShader->use();
   mTexShader->setMat4("projection", orthoProj);

   mColorShader->use();
   mColorShader->setMat4("projection", orthoProj);

   mLineShader->use();
   mLineShader->setMat4("projection", orthoProj);
}

void Renderer2D::updateSceneDimensions(const glm::vec2& currentSceneDimensions)
{
   mCurrentSceneDimensions = currentSceneDimensions;
}

void Renderer2D::updateScaleFactor(unsigned int scaleFactor)
{
   mScaleFactor = scaleFactor;
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

void Renderer2D::configureRealVAOs()
{
   /*
      Origin is at the top left corner because of the way we set up our orthographic projection matrix
      ABC = Counterclockwise
      ADB = Counterclockwise

        D-----------C
        |           |
        |           |
        |           |
        |           |
        |           |
        |   (0,0)   |
        |           |
        |           |
        |           |
        |           |
        |           |
        A-----------B
   */

                                          // Pos        // Tex coords
   std::array<float, 24> vertices      = {-0.5f, -0.5f, 0.0f, 1.0f,  // A
                                           0.5f, -0.5f, 1.0f, 1.0f,  // B
                                           0.5f,  0.5f, 1.0f, 0.0f,  // C
                                          -0.5f,  0.5f, 0.0f, 0.0f}; // D

   std::array<unsigned int, 4> indices = {0, 1, 2, 3};

   glGenVertexArrays(1, &mRealTexturedQuadVAO);
   glGenVertexArrays(1, &mRealColoredQuadVAO);
   glGenBuffers(1, &mRealQuadVBO);
   glGenBuffers(1, &mRealQuadEBO);

   // 1) Configure the VAO of the textured quad
   glBindVertexArray(mRealTexturedQuadVAO);

   // Load the quad's data into the buffers

   // Positions and texture coordinates
   glBindBuffer(GL_ARRAY_BUFFER, mRealQuadVBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
   // Indices
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mRealQuadEBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

   // Set the vertex attribute pointers

   // Positions and texture coordinates
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

   // 2) Configure the VAO of the non-textured quad
   glBindVertexArray(mRealColoredQuadVAO);

   // Load the quad's data into the buffers

   // Positions
   // The VBO has already been filled
   // It contains the texture coordinates, which the non-textured quad doesn't need
   // We will ignore them when we configure the vertex attributes
   glBindBuffer(GL_ARRAY_BUFFER, mRealQuadVBO);
   // Indices
   // The EBO has already been filled
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mRealQuadEBO);

   // Set the vertex attribute pointers
   // Positions
   // We ignore the texture coordinates by taking advantage of the stride
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

   glBindVertexArray(0);
}
