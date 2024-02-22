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

struct Mesh {
  //the first index of vertex index data of curr mesh
  u32 indexoffset = 0;
  //the first index of vertex data of curr mesh
  u32 vertexOffset = 0;

  u32 vertexCount = 0;
  u32 indexCount = 0;

  BufferHandle vertexBuffer;
  BufferHandle indexBuffer;
  BufferHandle matUniformBuffer;
  //for ray tracing
  BufferHandle primitiveMaterialIndexBuffer;
};

struct RenderObject {
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
  Mesh       convertAIMesh(aiMesh* mesh);

  // rasterization
  void executeScene(std::shared_ptr<SceneGraph> scene);
  //ray tracing
  void executeSceneRT(std::shared_ptr<SceneGraph> scene);

 public:
  const std::vector<RenderObject>& getRenderObjects() { return m_renderObjects; }
  std::shared_ptr<SceneGraph>      getSceneGraph() { return m_sceneGraph; }
  Material*                        getMaterialWidthIdx(int idx) { return m_materials[idx]; }
  Renderer*                        m_renderer;

 protected:
  void traverse(const aiScene* sourceScene, SceneGraph& sceneGraph, aiNode* node, int parent, int level);

 private:
  Assimp::Importer            m_modelImporter;
  std::shared_ptr<SceneGraph> m_sceneGraph;
  std::vector<Mesh>           m_meshes;
  int                         m_meshOffset = 0;
  std::vector<Material*>      m_materials;
  int                         m_materialOffset = 0;
  std::vector<BoundingBox>    m_boundingBoxes;
  int                         m_boundingBoxOffset = 0;

 private:
  std::vector<RenderObject> m_renderObjects;
};

}// namespace dg

#endif//MODELLOADER_H