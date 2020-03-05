#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <array>
#include <iostream>

#include "model_loader.h"
#include "texture_loader.h"

std::shared_ptr<Model> ModelLoader::loadResource(const std::string& modelFilePath) const
{
   Assimp::Importer importer;
   const aiScene* scene = importer.ReadFile(modelFilePath, aiProcess_Triangulate | aiProcess_FlipUVs);

   if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
   {
      std::cout << "Error - ModelLoader::loadResource - The error below occurred while importing this model: " << modelFilePath << "\n" << importer.GetErrorString() << "\n";
      return nullptr;
   }

   ResourceManager<Texture> texManager;
   std::vector<Mesh>        meshes;
   processNodeHierarchyRecursively(scene->mRootNode,
                                   scene,
                                   modelFilePath.substr(0, modelFilePath.find_last_of('/')),
                                   texManager,
                                   meshes);

   return std::make_shared<Model>(std::move(meshes), std::move(texManager));
}

void ModelLoader::processNodeHierarchyRecursively(const aiNode*             node,
                                                  const aiScene*            scene,
                                                  const std::string&        modelDir,
                                                  ResourceManager<Texture>& texManager,
                                                  std::vector<Mesh>&        meshes) const
{
   // Create a Mesh object for each mesh referenced by the current node
   for (unsigned int i = 0; i < node->mNumMeshes; i++)
   {
      // Note that nodes do not store meshes directly
      // All the meshes are stored in the scene struct
      // Nodes only contain indices that can be used to access meshes from said struct
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.emplace_back(processVertices(mesh),                                                           // Vertices
                          processIndices(mesh),                                                            // Indices
                          processMaterial(scene->mMaterials[mesh->mMaterialIndex], modelDir, texManager)); // Material textures and constants
   }

   // After we have processed all the meshes referenced by the current node, we recursively process its children
   for (unsigned int i = 0; i < node->mNumChildren; i++)
   {
      processNodeHierarchyRecursively(node->mChildren[i],
                                      scene,
                                      modelDir,
                                      texManager,
                                      meshes);
   }
}

std::vector<Vertex> ModelLoader::processVertices(const aiMesh* mesh) const
{
   std::vector<Vertex> vertices;
   vertices.reserve(mesh->mNumVertices);

   // Loop over the vertices of the mesh
   for (unsigned int i = 0; i < mesh->mNumVertices; i++)
   {
      // Store the position, the normal and the texture coordinates of the current vertex
      // Note that a vertex can contain up to 8 different sets of texture coordinates
      // We make the assumption that we will only use models that have a single set of texture coordinates per vertex
      // For this reason, we only check for the existence of the first set
      vertices.emplace_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),                                          // Position
                            glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),                                             // Normal
                            mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f)); // Texture coordinates
   }

   return vertices;
}

std::vector<unsigned int> ModelLoader::processIndices(const aiMesh* mesh) const
{
   // We assume that the mesh is made out of triangles, which is why we multiply the number of faces by 3 when reserving space in the indices vector
   // This will always be true if the aiProcess_Triangulate flag continues to be used when loading the model
   std::vector<unsigned int> indices;
   indices.reserve(mesh->mNumFaces * 3);

   // Loop over the faces of the mesh
   for (unsigned int i = 0; i < mesh->mNumFaces; i++)
   {
      aiFace face = mesh->mFaces[i];

      // Store the indices of the current face
      for (unsigned int j = 0; j < face.mNumIndices; j++)
      {
         indices.push_back(face.mIndices[j]);
      }
   }

   return indices;
}

Material ModelLoader::processMaterial(const aiMaterial*         material,
                                      const std::string&        modelDir,
                                      ResourceManager<Texture>& texManager) const
{
   // Load the textures
   std::vector<MaterialTexture>                                        materialTextures;
   std::bitset<static_cast<unsigned int>(MaterialTextureTypes::count)> materialTextureAvailabilities;

   // The material can consist of many textures and constants of different types
   // We make the assumption that we will only use models that have ambient, emissive, diffuse and specular textures or constants
   // A constant is only used during rendering if its corresponding texture is not available
   std::array<aiTextureType, 4> texTypes = {aiTextureType_AMBIENT,
                                            aiTextureType_EMISSIVE,
                                            aiTextureType_DIFFUSE,
                                            aiTextureType_SPECULAR};

   for (aiTextureType texType : texTypes)
   {
      // Get the number of textures of the current type
      unsigned int texCount = material->GetTextureCount(texType);

      if (texCount > 0)
      {
         if (texCount > 1)
         {
            std::cout << "Warning - ModelLoader::processMaterial - Mesh uses more than one texture of the following type: " << texType << ". Only the first texture will be loaded." << "\n";
         }

         // Compose the name of the sampler2D uniform that should exist in the shader,
         // and set the availability of the current texture type to true so that a texture of said type is used during rendering instead of its corresponding constant
         std::string uniformName;
         switch (texType)
         {
         case aiTextureType_AMBIENT:
            uniformName = "ambientTex";
            materialTextureAvailabilities[static_cast<unsigned int>(MaterialTextureTypes::ambient)] = true;
            break;
         case aiTextureType_EMISSIVE:
            uniformName = "emissiveTex";
            materialTextureAvailabilities[static_cast<unsigned int>(MaterialTextureTypes::emissive)] = true;
            break;
         case aiTextureType_DIFFUSE:
            uniformName = "diffuseTex";
            materialTextureAvailabilities[static_cast<unsigned int>(MaterialTextureTypes::diffuse)] = true;
            break;
         case aiTextureType_SPECULAR:
            uniformName = "specularTex";
            materialTextureAvailabilities[static_cast<unsigned int>(MaterialTextureTypes::specular)] = true;
            break;
         }

         aiString texFilename;
         material->GetTexture(texType, 0, &texFilename);

         // Note that we assume that the textures are in the same directory as the model
         materialTextures.emplace_back(texManager.loadResource<TextureLoader>(texFilename.C_Str(), modelDir + '/' + texFilename.C_Str()), uniformName);
      }
   }

   // Load the constants
   aiColor3D color(0.0f, 0.0f, 0.0f);
   float     shininess = 0.0f;

   MaterialConstants materialConstants(((material->Get(AI_MATKEY_COLOR_AMBIENT, color)  == AI_SUCCESS) ? glm::vec3(color.r, color.g, color.b) : glm::vec3(0.0f)),
                                       ((material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) ? glm::vec3(color.r, color.g, color.b) : glm::vec3(0.0f)),
                                       ((material->Get(AI_MATKEY_COLOR_DIFFUSE, color)  == AI_SUCCESS) ? glm::vec3(color.r, color.g, color.b) : glm::vec3(0.0f)),
                                       ((material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) ? glm::vec3(color.r, color.g, color.b) : glm::vec3(0.0f)),
                                       ((material->Get(AI_MATKEY_SHININESS, shininess)  == AI_SUCCESS) ? shininess : 0.0f));

   // TODO: Could we take advantage of move semantics here?
   return Material(materialTextures,
                   materialTextureAvailabilities,
                   materialConstants);
}
