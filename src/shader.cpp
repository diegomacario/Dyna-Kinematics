#include <iostream>

#include "shader.h"

Shader::Shader(unsigned int shaderProgID)
   : mShaderProgID(shaderProgID)
{

}

Shader::~Shader()
{
   glDeleteProgram(mShaderProgID);
}

Shader::Shader(Shader&& rhs) noexcept
   : mShaderProgID(std::exchange(rhs.mShaderProgID, 0))
{

}

Shader& Shader::operator=(Shader&& rhs) noexcept
{
   mShaderProgID = std::exchange(rhs.mShaderProgID, 0);
   return *this;
}

void Shader::use() const
{
   glUseProgram(mShaderProgID);
}

unsigned int Shader::getID() const
{
   return mShaderProgID;
}

void Shader::setBool(const std::string& name, bool value) const
{
   glUniform1i(getUniformLocation(name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value) const
{
   glUniform1i(getUniformLocation(name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
   glUniform1f(getUniformLocation(name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2 &value) const
{
   glUniform2fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
   glUniform2f(getUniformLocation(name.c_str()), x, y);
}

void Shader::setVec3(const std::string& name, const glm::vec3 &value) const
{
   glUniform3fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
   glUniform3f(getUniformLocation(name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4 &value) const
{
   glUniform4fv(getUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
   glUniform4f(getUniformLocation(name.c_str()), x, y, z, w);
}

void Shader::setMat2(const std::string& name, const glm::mat2& value) const
{
   glUniformMatrix2fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const
{
   glUniformMatrix3fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
   glUniformMatrix4fv(getUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

int Shader::getUniformLocation(const std::string& name) const
{
   int uniformLoc = glGetUniformLocation(mShaderProgID, name.c_str());

   if (uniformLoc == -1)
   {
      std::cout << "Error - Shader::getUniformLocation - The following uniform does not exist: " << name << "\n";
   }

   return uniformLoc;
}
