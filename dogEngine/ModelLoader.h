#ifndef MODELLOADER_H
#define MODELLOADER_H
#include "gpuResource.hpp"
#include "Vertex.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "json.hpp"
#include "tiny_gltf.h"

namespace dg{
struct DeviceContext;
struct Renderer;
struct Material;

struct AABBbox{
    glm::vec3 aa = glm::vec3(0.0f);
    glm::vec3 bb = glm::vec3(0.0f);
};

struct SceneNode{
    bool                                m_isUpdate = false;
    SceneNode*                          m_parentNodePtr = nullptr;
    glm::mat4                           m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
    int                                 m_meshIndex = -1;
    std::vector<SceneNode>              m_subNodes;
    AABBbox                             m_AABB;
    std::array<glm::vec3,2>             getAABB();
    void                                update();
};

struct Mesh{
    Material*                           m_meshMaterial = nullptr;
    std::string                         name;
    std::vector<Vertex>                 m_vertices;
    std::vector<u32>                    m_indices;
    std::string                         m_diffuseTexturePath;
    std::string                         m_MRTexturePath;
    std::string                         m_normalTexturePath;
    std::string                         m_emissiveTexturePath;
    std::string                         m_occlusionTexturePath;
};

struct RenderObject{
    RenderPassHandle                    m_renderPass;
    Material*                           m_material = nullptr;
    std::vector<DescriptorSetHandle>    m_descriptors;
    glm::mat4                           m_modelMatrix = {glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1)};
    BufferHandle                        m_vertexBuffer;
    BufferHandle                        m_indexBuffer;
    BufferHandle                        m_uniformBuffer;
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
    BaseLoader(Renderer* renderer);
    virtual void destroy();
    virtual ~BaseLoader() {};
    virtual void                        loadFromPath(std::string path){}; 
    virtual std::vector<RenderObject>   Execute(const SceneNode& rootNode);
    virtual bool                        haveContent(){return m_haveContent;}
    virtual SceneNode&                  getSceneNode(){return m_sceneRoot;}
    virtual std::vector<RenderObject>&  getRenderObject(){return m_renderObjects;}
    virtual std::vector<Mesh>&          getMeshes(){return m_meshes;}

protected:
    SceneNode                           m_sceneRoot;
    std::vector<RenderObject>           m_renderObjects;
    std::vector<Mesh>                   m_meshes;
    Renderer*                           m_renderer;
    bool                                m_haveContent = false;
};

class objLoader: public BaseLoader{
public:
    objLoader();
    objLoader(Renderer* renderer);
    ~objLoader(){};
    void                                loadFromPath(std::string path) override;
    void                                destroy() override;
};

class gltfLoader: public BaseLoader{
public:
    enum LoadType{
        MODEL,
        SKYBOX
    };
    gltfLoader();
    gltfLoader(Renderer* renderer);
    ~gltfLoader(){};
    void loadNode(tinygltf::Node& inputNode, tinygltf::Model& model, SceneNode* parent,LoadType LoadType=LoadType::MODEL);
    void                                loadFromPath(std::string path, LoadType ldType=LoadType::MODEL);
    void                                destroy() override;
    void                                setSkyBox(std::string path);
    void                                setEnvMap(std::string path);

};

}

#endif //MODELLOADER_H