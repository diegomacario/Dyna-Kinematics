#include <utility>

#include "texture.h"

Texture::Texture(unsigned int texID)
   : mTexID(texID)
{

}

Texture::~Texture()
{
   glDeleteTextures(1, &mTexID);
}

Texture::Texture(Texture&& rhs) noexcept
   : mTexID(std::exchange(rhs.mTexID, 0))
{

}

Texture& Texture::operator=(Texture&& rhs) noexcept
{
   mTexID = std::exchange(rhs.mTexID, 0);
   return *this;
}

void Texture::bind() const
{
   glBindTexture(GL_TEXTURE_2D, mTexID);
}
