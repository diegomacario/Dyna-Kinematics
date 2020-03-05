#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H

#include <memory>

#include "shader.h"

class ShaderLoader
{
public:

   ShaderLoader() = default;
   ~ShaderLoader() = default;

   ShaderLoader(const ShaderLoader&) = default;
   ShaderLoader& operator=(const ShaderLoader&) = default;

   ShaderLoader(ShaderLoader&&) = default;
   ShaderLoader& operator=(ShaderLoader&&) = default;

   std::shared_ptr<Shader> loadResource(const std::string& vShaderFilePath,
                                        const std::string& fShaderFilePath) const;

   std::shared_ptr<Shader> loadResource(const std::string& vShaderFilePath,
                                        const std::string& fShaderFilePath,
                                        const std::string& gShaderFilePath) const;

private:

   unsigned int            createAndCompileShader(const std::string& shaderFilePath, GLenum shaderType) const;
   unsigned int            createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID) const;
   unsigned int            createAndLinkShaderProgram(unsigned int vShaderID, unsigned int fShaderID, unsigned int gShaderID) const;
   void                    checkForCompilationErrors(unsigned int shaderID, GLenum shaderType, const std::string& shaderFilePath) const;
   void                    checkForLinkingErrors(unsigned int shaderProgID) const;
};

#endif
