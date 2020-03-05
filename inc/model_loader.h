#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <unordered_map>

#include "model.h"
#include "resource_manager.h"

class ModelLoader
{
public:

   ModelLoader() = default;
   ~ModelLoader() = default;

   ModelLoader(const ModelLoader&) = default;
   ModelLoader& operator=(const ModelLoader&) = default;

   ModelLoader(ModelLoader&&) = default;
   ModelLoader& operator=(ModelLoader&&) = default;

   std::shared_ptr<Model>    loadResource(const std::string& modelFilePath) const;

private:

   void                      processNodeHierarchyRecursively(const aiNode*             node,
                                                             const aiScene*            scene,
                                                             const std::string&        modelDir,
                                                             ResourceManager<Texture>& texManager,
                                                             std::vector<Mesh>&        meshes) const;

   std::vector<Vertex>       processVertices(const aiMesh* mesh) const;

   std::vector<unsigned int> processIndices(const aiMesh* mesh) const;

   Material                  processMaterial(const aiMaterial*         material,
                                             const std::string&        modelDir,
                                             ResourceManager<Texture>& texManager) const;
};

#endif
