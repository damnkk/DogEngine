#ifndef RENDERER_H
#define RENDERER_H
#include <chrono>
#include <unordered_map>
#include "GUI/gui.h"
#include "DeviceContext.h"
#include "Vertex.h"
#include "ModelLoader.h"

namespace dg{

void keycallback(GLFWwindow *window);
std::vector<char> readFile(const std::string& filename);

struct Renderer;
struct objLoader;
struct gltfLoader;
struct TextureResource:public Resource{
    u32                                 poolIdx;
    TextureHandle                       handle;
    static const std::string            k_type;
};

struct BufferResource:public Resource{
    u32                                 poolIdx;
    BufferHandle                        handle;
    static const std::string            k_type;
};


struct SamplerResource:public Resource{
    u32                                 poolIdx;
    SamplerHandle                       handle;
    static const std::string            k_type;
};


struct ProgramPass{
    PipelineHandle                      pipeline;
    DescriptorSetLayoutHandle           descriptorSetLayout;
};

struct Program{
    Program(){}
    Program(std::shared_ptr<DeviceContext> context,std::string name);
    ~Program();
    void                                addPass(pipelineCreateInfo pipeInfo);
    std::string                         name;
    std::vector<ProgramPass>            passes;
    std::shared_ptr<DeviceContext>      context;
};

struct MaterialCreateInfo{
    MaterialCreateInfo&                 setRenderOrder(u32 renderOrder);
    MaterialCreateInfo&                 setName(std::string name);
    std::string                         name;
    u32                                 renderOrder = -1;
    Renderer*                           renderer;
};

struct uniformRenderData{
    alignas(16)glm::vec3                lightDirection;
};

struct TextureBind{
    TextureHandle                       texture = {k_invalid_index};
    u32                                 bindIdx = -1;
};

struct Material:public Resource{

    struct UniformMaterial{
        alignas(16) glm::mat4                           modelMatrix = glm::mat4(1.0f);
        alignas(16) glm::vec4                           baseColorFactor = glm::vec4(glm::vec3(0.0f),1.0f);
        alignas(16) glm::vec3                           emissiveFactor = glm::vec3(0.0f);
        //transparnt, uvscale, envRotate
        alignas(16) glm::vec3                           tueFactor = glm::vec3(1.0f,1.0f,0.0f);
        //metallic, roughness, __undefined_placeholder__
        alignas(16) glm::vec3                           mrFactor = glm::vec3(1.0f,1.0f,0.0f);
    } uniformMaterial;
    Material();
    void                                addTexture(Renderer* renderer, std::string name, std::string path);
    void                                addLutTexture(Renderer* renderer, TextureHandle handle);
    void                                addDiffuseEnvMap(Renderer* renderer, TextureHandle handle);
    void                                addSpecularEnvMap(Renderer* renderer, TextureHandle handle);
    void                                updateProgram();
    void                                setDiffuseTexture(TextureHandle handle){
        textureMap["DiffuseTexture"].texture = handle;
    }
    void                                setNormalTexture(TextureHandle handle){
        textureMap["NormalTexture"].texture = handle;
    }
    void                                setMRTexture(TextureHandle handle){
        textureMap["MetallicRoughnessTexture"].texture = handle;
    }
    void                                setEmissiveTexture(TextureHandle handle){
        textureMap["EmissiveTexture"].texture = handle;
    }
    void                                setAoTexture(TextureHandle handle){
        textureMap["AOTexture"].texture = handle;
    }
    void                                setProgram(const std::shared_ptr<Program> &program);

    Renderer*                           renderer;
    u32                                 poolIdx;
    std::shared_ptr<Program>            program;
    u32                                 renderOrder;
    std::unordered_map<std::string, TextureBind> textureMap;


    //render state
    bool                                depthTest = true;
    bool                                depthWrite = true;
    VkCullModeFlagBits                  cullModel = VK_CULL_MODE_BACK_BIT;
    VkBlendOp                           blendOp;
};


struct ResourceCache{
    void                                             destroy(Renderer* renderer);

    std::unordered_map<std::string, TextureResource*> textures;
    std::unordered_map<std::string, SamplerResource*> samplers;
    std::unordered_map<std::string, BufferResource*>  buffers;
    std::unordered_map<std::string, Material*>        materials;

};
struct Renderer{
public:
    void                                        init(std::shared_ptr<DeviceContext> context);
    void                                        destroy();

    TextureResource*                            createTexture( TextureCreateInfo& textureInfo);
    BufferResource*                             createBuffer( BufferCreateInfo& bufferInfo);
    SamplerResource*                            createSampler( SamplerCreateInfo& samplerInfo);
    std::shared_ptr<Program>                    createProgram(const std::string& name, pipelineCreateInfo* pipCI);
    Material*                                   createMaterial( MaterialCreateInfo& matInfo);
    void                                        addSkyBox(std::string skyTexPath);

    TextureHandle                               upLoadTextureToGPU(std::string& texPath);
    template<typename T>
    BufferHandle                                upLoadBufferToGPU( std::vector<T>& bufferData, const char* meshName);


    void                                        destroyTexture(TextureResource* textureRes);
    void                                        destroyBuffer(BufferResource* bufferRes);
    void                                        destroySampler(SamplerResource* samplerRes);
    void                                        destroyMaterial(Material* material);

    std::shared_ptr<DeviceContext>              getContext(){return m_context;}
    void                                        setDefaultMaterial(Material* material);
    Material*                                   getDefaultMaterial();
    std::shared_ptr<objLoader>                  getObjLoader(){return m_objLoader;}
    std::shared_ptr<gltfLoader>                 getGltfLoader(){return m_gltfLoader;}
    void                                        newFrame();
    void                                        present();
    void                                        drawScene();
    void                                        drawUI();
    void                                        loadFromPath(const std::string& path);
    void                                        executeScene();
protected:
    void                                        executeSkyBox();
    void                                        makeDefaultMaterial();

private:
    ResourceCache                               m_resourceCache;
    RenderResourcePool<TextureResource>         m_textures;
    RenderResourcePool<BufferResource>          m_buffers;
    RenderResourcePool<Material>                m_materials;
    RenderResourcePool<SamplerResource>         m_samplers;
    std::vector<RenderObject>                   m_renderObjects;
    std::shared_ptr<DeviceContext>              m_context;
    std::shared_ptr<objLoader>                  m_objLoader;
    std::shared_ptr<gltfLoader>                 m_gltfLoader;
    Material*                                   m_defaultMaterial;

private:
    TextureHandle                               m_skyTexture;
    bool                                        have_skybox;

};


template<typename T>
BufferHandle Renderer::upLoadBufferToGPU( std::vector<T>& bufferData, const char* meshName){
    if(bufferData.empty()) return {k_invalid_index};
    VkDeviceSize BufSize = bufferData.size()*sizeof(T);
    BufferCreateInfo bufferInfo;
    bufferInfo.reset();
    bufferInfo.setData(bufferData.data());
    bufferInfo.setDeviceOnly(true);
    if(std::is_same<T, Vertex>::value){
        std::string vertexBufferName = "vt";
        vertexBufferName +=meshName;
        bufferInfo.setUsageSize(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, BufSize);
        bufferInfo.setName(vertexBufferName.c_str());
    }else if(std::is_same<T,u32>::value){
        std::string IndexBufferName = "id";
        IndexBufferName +=meshName;
        bufferInfo.setUsageSize(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, BufSize);
            bufferInfo.setName(IndexBufferName.c_str());
    }
    BufferHandle GPUHandle = createBuffer(bufferInfo)->handle;
    return GPUHandle;
}


}
#endif //RENDERER_H
