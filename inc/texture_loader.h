#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <string>
#include <memory>

#include "texture.h"

class TextureLoader
{
public:

   TextureLoader() = default;
   ~TextureLoader() = default;

   TextureLoader(const TextureLoader&) = default;
   TextureLoader& operator=(const TextureLoader&) = default;

   TextureLoader(TextureLoader&&) = default;
   TextureLoader& operator=(TextureLoader&&) = default;

   std::shared_ptr<Texture> loadResource(const std::string& texFilePath,
                                         unsigned int       wrapS     = GL_REPEAT,
                                         unsigned int       wrapT     = GL_REPEAT,
                                         unsigned int       minFilter = GL_LINEAR_MIPMAP_LINEAR,
                                         unsigned int       magFilter = GL_LINEAR,
                                         bool               genMipmap = true) const;

private:

   unsigned int generateTexture(const std::unique_ptr<unsigned char, void(*)(void*)>& texData,
                                int          width,
                                int          height,
                                int          numComponents,
                                unsigned int wrapS,
                                unsigned int wrapT,
                                unsigned int minFilter,
                                unsigned int magFilter,
                                bool         genMipmap) const;
};

#endif
