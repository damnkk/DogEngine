#ifndef MODELLOADER_H
#define MODELLOADER_H
#include "dgpch.h"
#include "glm/glm.hpp"
#include "gpuResource.hpp"
#include "Vertex.h"

namespace dg{
struct DeviceContext;

struct AABBbox{
    glm::vec3 aa = glm::vec3(0.0f);
    glm::vec3 bb = glm::vec3(0.0f);
};

struct SceneNode{
    bool                                m_isUpdate = false;
    glm::mat4                           m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
    int                                 m_meshIndex = -1;
    std::vector<SceneNode>              m_subNodes;
    AABBbox                             m_AABB;
    std::array<glm::vec3,2>             getAABB();
    void                                update();
};

struct Mesh{
    std::vector<Vertex>                 m_vertices;
    std::vector<u32>                    m_indeices;
    std::string                         m_diffuseTexturePath;
    std::string                         m_MRTexturePath;
    std::string                         m_normalTexturePath;
    std::string                         m_emissiveTexturePath;
    std::string                         m_occlusionTexturePath;
};

struct RenderObject{
    RenderPassHandle                    m_renderPass;
    PipelineHandle                      m_pipeline;
    std::vector<DescriptorSetHandle>    m_descriptors = std::vector<DescriptorSetHandle>(k_max_swapchain_images);
    glm::mat4                           m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
    BufferHandle                        m_vertexBuffer;
    BufferHandle                        m_indexBuffer;
    std::vector<BufferHandle>           m_uniformBuffers;
    u32                                 m_vertexCount = 0;
    u32                                 m_indexCount = 0;
    TextureHandle                       m_diffuseTex = {k_invalid_index};
    TextureHandle                       m_MRTtex = {k_invalid_index};
    TextureHandle                       m_normalTex = {k_invalid_index};
    TextureHandle                       m_emissiveTex = {k_invalid_index};
    TextureHandle                       m_occlusionTex = {k_invalid_index};
};

class BaseLoader{
public:
    BaseLoader(){};
    BaseLoader(std::shared_ptr<DeviceContext> context);
    virtual void destroy();
    virtual ~BaseLoader() {};
    virtual void                        loadFromPath(std::string path) = 0; 
    virtual std::vector<RenderObject>   Execute(const SceneNode& rootNode);
    virtual bool                        haveContent(){return m_haveContent;}
    virtual SceneNode&                  getSceneNode(){return m_sceneRoot;}
    virtual std::vector<RenderObject>&  getRenderObject(){return m_renderObjects;}

protected:
    SceneNode                           m_sceneRoot;
    std::vector<RenderObject>           m_renderObjects;
    std::vector<Mesh>                   m_meshes;
    std::shared_ptr<DeviceContext>      m_context;
    bool                                m_haveContent = false;
};

class objLoader: public BaseLoader{
public:
    objLoader();
    objLoader(std::shared_ptr<DeviceContext> context);
    ~objLoader(){};
    void                                loadFromPath(std::string path);
};

class gltfLoader: public BaseLoader{
public:
    gltfLoader();
    gltfLoader(std::shared_ptr<DeviceContext> context);
    ~gltfLoader(){};
    void                                loadFromPath(std::string path);
};

}

#endif //MODELLOADER_H