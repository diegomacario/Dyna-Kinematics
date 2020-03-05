#ifndef MESH_H
#define MESH_H

#include <assimp/scene.h>

#include <memory>
#include <vector>
#include <bitset>

#include "shader.h"
#include "texture.h"

struct Vertex
{
   Vertex(const glm::vec3& position,
          const glm::vec3& normal,
          const glm::vec2& texCoords)
      : position(position)
      , normal(normal)
      , texCoords(texCoords)
   {

   }

   ~Vertex() = default;

   Vertex(const Vertex&) = default;
   Vertex& operator=(const Vertex&) = default;

   Vertex(Vertex&& rhs) noexcept
      : position(std::exchange(rhs.position, glm::vec3(0.0f)))
      , normal(std::exchange(rhs.normal, glm::vec3(0.0f)))
      , texCoords(std::exchange(rhs.texCoords, glm::vec2(0.0f)))
   {

   }

   Vertex& operator=(Vertex&& rhs) noexcept
   {
      position  = std::exchange(rhs.position, glm::vec3(0.0f));
      normal    = std::exchange(rhs.normal, glm::vec3(0.0f));
      texCoords = std::exchange(rhs.texCoords, glm::vec2(0.0f));
      return *this;
   }

   glm::vec3 position;
   glm::vec3 normal;
   glm::vec2 texCoords;
};

struct MaterialTexture
{
   MaterialTexture(const std::shared_ptr<Texture>& texture, const std::string& uniformName)
      : texture(texture)
      , uniformName(uniformName)
   {

   }

   ~MaterialTexture() = default;

   MaterialTexture(const MaterialTexture&) = default;
   MaterialTexture& operator=(const MaterialTexture&) = default;

   MaterialTexture(MaterialTexture&& rhs) = default;
   MaterialTexture& operator=(MaterialTexture&& rhs) = default;

   std::shared_ptr<Texture> texture;
   std::string              uniformName;
};

enum class MaterialTextureTypes : unsigned int
{
   ambient  = 0,
   emissive = 1,
   diffuse  = 2,
   specular = 3,
   count    = 4
};

struct MaterialConstants
{
   MaterialConstants(const glm::vec3& ambientColor,
                     const glm::vec3& emissiveColor,
                     const glm::vec3& diffuseColor,
                     const glm::vec3& specularColor,
                     float            shininess)
      : ambientColor(ambientColor)
      , emissiveColor(emissiveColor)
      , diffuseColor(diffuseColor)
      , specularColor(specularColor)
      , shininess(shininess)
   {

   }

   ~MaterialConstants() = default;

   MaterialConstants(const MaterialConstants&) = default;
   MaterialConstants& operator=(const MaterialConstants&) = default;

   MaterialConstants(MaterialConstants&& rhs) noexcept
      : ambientColor(std::exchange(rhs.ambientColor, glm::vec3(0.0f)))
      , emissiveColor(std::exchange(rhs.emissiveColor, glm::vec3(0.0f)))
      , diffuseColor(std::exchange(rhs.diffuseColor, glm::vec3(0.0f)))
      , specularColor(std::exchange(rhs.specularColor, glm::vec3(0.0f)))
      , shininess(std::exchange(rhs.shininess, 0.0f))
   {

   }

   MaterialConstants& operator=(MaterialConstants&& rhs) noexcept
   {
      ambientColor  = std::exchange(rhs.ambientColor, glm::vec3(0.0f));
      emissiveColor = std::exchange(rhs.emissiveColor, glm::vec3(0.0f));
      diffuseColor  = std::exchange(rhs.diffuseColor, glm::vec3(0.0f));
      specularColor = std::exchange(rhs.specularColor, glm::vec3(0.0f));
      shininess     = std::exchange(rhs.shininess, 0.0f);
      return *this;
   }

   glm::vec3 ambientColor;
   glm::vec3 emissiveColor;
   glm::vec3 diffuseColor;
   glm::vec3 specularColor;
   float     shininess;
};

struct Material
{
   Material(const std::vector<MaterialTexture>&                                 materialTextures,
            std::bitset<static_cast<unsigned int>(MaterialTextureTypes::count)> materialTextureAvailabilities,
            const MaterialConstants&                                            materialConstants)
      : textures(materialTextures)
      , textureAvailabilities(materialTextureAvailabilities)
      , constants(materialConstants)
   {

   }

   ~Material() = default;

   Material(const Material&) = default;
   Material& operator=(const Material&) = default;

   Material(Material&& rhs) noexcept
      : textures(std::move(rhs.textures))
      , textureAvailabilities(std::exchange(rhs.textureAvailabilities, std::bitset<static_cast<unsigned int>(MaterialTextureTypes::count)>())) // TODO: Investigate what happens when you move a std::bitset
      , constants(std::move(rhs.constants))
   {

   }

   Material& operator=(Material&& rhs) noexcept
   {
      textures              = std::move(rhs.textures);
      textureAvailabilities = std::exchange(rhs.textureAvailabilities, std::bitset<static_cast<unsigned int>(MaterialTextureTypes::count)>()); // TODO: Investigate what happens when you move a std::bitset
      constants             = std::move(rhs.constants);
      return *this;
   }

   std::vector<MaterialTexture>                                        textures;
   std::bitset<static_cast<unsigned int>(MaterialTextureTypes::count)> textureAvailabilities;
   MaterialConstants                                                   constants;
};

class Mesh
{
public:

   Mesh(const std::vector<Vertex>&       vertices,
        const std::vector<unsigned int>& indices,
        const Material&                  material);
   ~Mesh();

   Mesh(const Mesh&) = delete;
   Mesh& operator=(const Mesh&) = delete;

   Mesh(Mesh&& rhs) noexcept;
   Mesh& operator=(Mesh&& rhs) noexcept;

   void render(const Shader& shader) const;

private:

   void configureVAO(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

   void bindMaterialTextures(const Shader& shader) const;
   void setMaterialTextureAvailabilities(const Shader& shader) const;
   void setMaterialConstants(const Shader& shader) const;

   unsigned int mNumIndices;
   Material     mMaterial;
   unsigned int mVAO;
   unsigned int mVBO;
   unsigned int mEBO;
};

#endif
