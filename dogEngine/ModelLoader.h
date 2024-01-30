#ifndef MODELLOADER_H
#define MODELLOADER_H
#include "Vertex.h"
#include "gpuResource.hpp"
#include "json.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include "tiny_gltf.h"

//assimp
#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/pbrmaterial.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "dgSceneGraph.h"

namespace dg {
struct DeviceContext;
struct Renderer;
struct Material;

struct BoundingBox {
  glm::vec3 m_min = glm::vec3(0.0f);
  glm::vec3 m_max = glm::vec3(0.0f);
  BoundingBox() = default;
  BoundingBox(glm::vec3 min, glm::vec3 max) : m_min(glm::min(min, max)), m_max(glm::max(min, max)) {}
  BoundingBox(glm::vec3* point, u32 pointCount) {
    glm::vec3 vmax(std::numeric_limits<float>::max());
    glm::vec3 vmin(std::numeric_limits<float>::min());
    for (u32 i = 0; i < pointCount; ++i) {
      vmin = glm::min(vmin, point[i]);
      vmax = glm::max(vmax, point[i]);
    }
    m_min = vmin;
    m_max = vmax;
  }

  glm::vec3 getSize() const {
    return glm::vec3(m_max.x - m_min.x, m_max.y - m_min.y, m_max.z - m_min.z);
  }

  glm::vec3 getCenter() const {
    return 0.5f * glm::vec3(m_max.x + m_min.x, m_max.y + m_min.y, m_max.z + m_min.z);
  }

  void transform(const glm::mat4& t) {
    glm::vec3 corners[] = {
        glm::vec3(m_min.x, m_min.y, m_min.z),
        glm::vec3(m_min.x, m_max.y, m_min.z),
        glm::vec3(m_min.x, m_min.y, m_max.z),
        glm::vec3(m_min.x, m_max.y, m_max.z),
        glm::vec3(m_max.x, m_min.y, m_min.z),
        glm::vec3(m_max.x, m_max.y, m_min.z),
        glm::vec3(m_max.x, m_min.y, m_max.z),
        glm::vec3(m_max.x, m_max.y, m_max.z),
    };
    for (auto& v : corners) {
      v = glm::vec3(t * glm::vec4(v, 1.0f));
    }
    *this = BoundingBox(corners, 8);
  }

  BoundingBox getTransformed(const glm::mat4& t) {
    BoundingBox b = *this;
    b.transform(t);
    return b;
  }

  void combinePoint(const glm::vec3& p) {
    m_max = glm::max(m_max, p);
    m_min = glm::min(m_min, p);
  }
};

struct SceneNode {
  bool                     m_isUpdate = false;
  SceneNode*               m_parentNodePtr = nullptr;
  glm::mat4                m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
  int                      m_meshIndex = -1;
  std::vector<SceneNode>   m_subNodes;
  BoundingBox              m_AABB;
  std::array<glm::vec3, 2> getAABB();
  void                     update();
};

struct Mesh {
  Material*           m_meshMaterial = nullptr;
  std::string         name;
  std::vector<Vertex> m_vertices;
  std::vector<u32>    m_indices;
};

struct RenderObject {
  RenderPassHandle                 m_renderPass;
  Material*                        m_material = nullptr;
  std::vector<DescriptorSetHandle> m_descriptors;
  //模型变换矩阵可以删掉,直接根据索引去场景图的数组里面拿,这样方便
  glm::mat4    m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
  BufferHandle m_vertexBuffer;
  BufferHandle m_indexBuffer;
  BufferHandle m_GlobalUniform;
  BufferHandle m_MaterialUniform;
  u32          m_vertexCount = 0;
  u32          m_indexCount = 0;
};

class BaseLoader {
 public:
  BaseLoader(){};
  BaseLoader(Renderer* renderer);
  virtual void destroy();
  virtual ~BaseLoader(){};
  virtual void                       loadFromPath(std::string path){};
  virtual std::vector<RenderObject>  Execute(const SceneNode& rootNode, glm::mat4 modelMatrix = glm::mat4(1.0f));
  virtual bool                       haveContent() { return m_haveContent; }
  virtual SceneNode&                 getSceneRoot() { return m_sceneRoot; }
  virtual std::vector<RenderObject>& getRenderObject() { return m_renderObjects; }
  virtual std::vector<Mesh>&         getMeshes() { return m_meshes; }

 protected:
  SceneNode                 m_sceneRoot;
  std::vector<RenderObject> m_renderObjects;
  std::vector<Mesh>         m_meshes;
  Renderer*                 m_renderer;
  bool                      m_haveContent = false;
};

class objLoader : public BaseLoader {
 public:
  objLoader();
  objLoader(Renderer* renderer);
  ~objLoader(){};
  void loadFromPath(std::string path) override;
  void destroy() override;
};

class gltfLoader : public BaseLoader {
 public:
  gltfLoader();
  gltfLoader(Renderer* renderer);
  ~gltfLoader(){};
  void loadNode(tinygltf::Node& inputNode, tinygltf::Model& model, SceneNode* parent);
  void loadFromPath(std::string path) override;
  void destroy() override;
};

struct modiMesh {
  //the first index of vertex index data of curr mesh
  u32 indexoffset = 0;
  //the first index of vertex data of curr mesh
  u32 vertexOffset = 0;

  u32 vertexCount = 0;
  u32 indexCount = 0;

  BufferHandle vertexBufferHandle;
  BufferHandle indexBufferHandle;
  BufferHandle matUniformBufferHandle;
};

struct modiRenderObject {
  u32                              meshIndex;
  u32                              transformIndex;
  u32                              vertexIndicesCount = 0;
  Material*                        materialPtr = nullptr;
  std::vector<DescriptorSetHandle> descriptors;
  RenderPassHandle                 renderpass;
  BufferHandle                     vertexBuffer;
  BufferHandle                     indexBuffer;
  BufferHandle                     globalUniform;
  BufferHandle                     materialUniform;
};

class ResourceLoader {
 public:
  ResourceLoader() {}
  ResourceLoader(Renderer* renderer) : m_renderer(renderer) {}
  SceneGraph loadModel(std::string modelPath);
  void       loadSceneGraph(std::string sceneGraphPath);
  Material*  convertAIMaterialToDescription(const aiMaterial* aiMat, std::string basePath);
  modiMesh   convertAIMesh(aiMesh* mesh);
  void       executeScene(std::shared_ptr<SceneGraph> scene);

 public:
  const std::vector<modiRenderObject>& getRenderObjects() { return m_renderObjects; }
  std::shared_ptr<SceneGraph>          getSceneGraph() { return m_sceneGraph; }
  Renderer*                            m_renderer;

 protected:
  void traverse(const aiScene* sourceScene, SceneGraph& sceneGraph, aiNode* node, int parent, int level);

 private:
  Assimp::Importer            m_modelImporter;
  std::shared_ptr<SceneGraph> m_sceneGraph;
  std::vector<modiMesh>       m_meshes;
  int                         m_meshOffset = 0;
  std::vector<Material*>      m_materials;
  int                         m_materialOffset = 0;
  std::vector<BoundingBox>    m_boundingBoxes;
  int                         m_boundingBoxOffset = 0;

 private:
  std::vector<modiRenderObject> m_renderObjects;
};

}// namespace dg

#endif//MODELLOADER_H