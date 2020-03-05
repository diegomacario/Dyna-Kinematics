#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

class Shader
{
public:

   explicit Shader(unsigned int shaderProgID);
   ~Shader();

   Shader(const Shader&) = delete;
   Shader& operator=(const Shader&) = delete;

   Shader(Shader&& rhs) noexcept;
   Shader& operator=(Shader&& rhs) noexcept;

   void         use() const;

   unsigned int getID() const;

   void         setBool(const std::string& name, bool value) const;
   void         setInt(const std::string& name, int value) const;
   void         setFloat(const std::string& name, float value) const;

   void         setVec2(const std::string& name, const glm::vec2& value) const;
   void         setVec2(const std::string& name, float x, float y) const;
   void         setVec3(const std::string& name, const glm::vec3& value) const;
   void         setVec3(const std::string& name, float x, float y, float z) const;
   void         setVec4(const std::string& name, const glm::vec4& value) const;
   void         setVec4(const std::string& name, float x, float y, float z, float w) const;

   void         setMat2(const std::string& name, const glm::mat2& value) const;
   void         setMat3(const std::string& name, const glm::mat3& value) const;
   void         setMat4(const std::string& name, const glm::mat4& value) const;

private:

   int          getUniformLocation(const std::string& name) const;

   unsigned int mShaderProgID;
};

#endif
