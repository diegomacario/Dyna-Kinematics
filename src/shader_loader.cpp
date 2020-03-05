#include <glad/glad.h>

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "shader_loader.h"

std::shared_ptr<Shader> ShaderLoader::loadResource(const std::string& vShaderFilePath,
                                                   const std::string& fShaderFilePath) const
{
   unsigned int vShaderID = createAndCompileShader(vShaderFilePath, GL_VERTEX_SHADER);
   unsigned int fShaderID = createAndCompileShader(fShaderFilePath, GL_FRAGMENT_SHADER);

   unsigned int shaderProgID = createAndLinkShaderProgram(vShaderID, fShaderID);

   glDetachShader(shaderProgID, vShaderID);
   glDetachShader(shaderProgID, fShaderID);

   glDeleteShader(vShaderID);
   glDeleteShader(fShaderID);

   return std::make_shared<Shader>(shaderProgID);
}

std::shared_ptr<Shader> ShaderLoader::loadResource(const std::string& vShaderFilePath,
                                                   const std::string& fShaderFilePath,
                                                   const std::string& gShaderFilePath) const
{
   unsigned int vShaderID = createAndCompileShader(vShaderFilePath, GL_VERTEX_SHADER);
   unsigned int fShaderID = createAndCompileShader(fShaderFilePath, GL_FRAGMENT_SHADER);
   unsigned int gShaderID = createAndCompileShader(gShaderFilePath, GL_GEOMETRY_SHADER);

   unsigned int shaderProgID = createAndLinkShaderProgram(vShaderID, fShaderID, gShaderID);

   glDetachShader(shaderProgID, vShaderID);
   glDetachShader(shaderProgID, fShaderID);
   glDetachShader(shaderProgID, gShaderID);

   glDeleteShader(vShaderID);
   glDeleteShader(fShaderID);
   glDeleteShader(gShaderID);

   return std::make_shared<Shader>(shaderProgID);
}

unsigned int ShaderLoader::createAndCompileShader(const std::string& shaderFilePath, GLenum shaderType) const
{
   std::ifstream shaderFile(shaderFilePath);

   if (shaderFile)
   {
      // Read the entire file into a string
      std::stringstream shaderStream;
      shaderStream << shaderFile.rdbuf();
      std::string shaderCodeStr = shaderStream.str();

      shaderFile.close();

      // Create and compile the shader
      unsigned int shaderID = glCreateShader(shaderType);
      const char* shaderCodeCStr = shaderCodeStr.c_str();
      glShaderSource(shaderID, 1, &shaderCodeCStr, nullptr);
      glCompileShader(shaderID);
      checkForCompilationErrors(shaderID, shaderType, shaderFilePath);
      return shaderID;
   }
   else
   {
      std::cout << "Error - ShaderLoader::createAndCompileShader - The following shader file could not be opened: " << shaderFilePath << "\n";
      return 0;
   }
}

unsigned int ShaderLoader::createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID) const
{
   unsigned int shaderProgID = glCreateProgram();

   glAttachShader(shaderProgID, vShaderID);
   glAttachShader(shaderProgID, fShaderID);

   glLinkProgram(shaderProgID);
   checkForLinkingErrors(shaderProgID);

   return shaderProgID;
}

unsigned int ShaderLoader::createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID, unsigned int gShaderID) const
{
   unsigned int shaderProgID = glCreateProgram();

   glAttachShader(shaderProgID, vShaderID);
   glAttachShader(shaderProgID, fShaderID);
   glAttachShader(shaderProgID, gShaderID);

   glLinkProgram(shaderProgID);
   checkForLinkingErrors(shaderProgID);

   return shaderProgID;
}

void ShaderLoader::checkForCompilationErrors(unsigned int shaderID, GLenum shaderType, const std::string& shaderFilePath) const
{
   int success;
   glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

   if (!success)
   {
      int infoLogLength = 0;
      glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

      std::vector<char> infoLog(infoLogLength);
      glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &infoLog[0]);

      std::cout << "Error - ShaderLoader::checkForCompilationErrors - The error below occurred while compiling this shader: " << shaderFilePath << "\n" << infoLog.data() << "\n";
   }
}

void ShaderLoader::checkForLinkingErrors(unsigned int shaderProgID) const
{
   int success;
   glGetProgramiv(shaderProgID, GL_LINK_STATUS, &success);

   if (!success)
   {
      int infoLogLength = 0;
      glGetProgramiv(shaderProgID, GL_INFO_LOG_LENGTH, &infoLogLength);

      std::vector<char> infoLog(infoLogLength);
      glGetProgramInfoLog(shaderProgID, infoLogLength, nullptr, &infoLog[0]);

      std::cout << "Error - ShaderLoader::checkForLinkingErrors - The following error occurred while linking a shader program:\n" << infoLog.data() << "\n";
   }
}
