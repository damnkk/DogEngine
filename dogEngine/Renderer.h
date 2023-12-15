#ifndef RENDERER_H
#define RENDERER_H
#include <chrono>
#include <unordered_map>
#include "GUI/gui.h"
#include "DeviceContext.h"

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

struct ProgramResource:public Resource{
    u32                                 poolIdx;
    std::vector<ProgramPass>            passes;
    static const std::string            k_type;
};

struct MaterialCreateInfo{
    MaterialCreateInfo&                 setProgram(ProgramResource* program);
    MaterialCreateInfo&                 setRenderOrder(u32 renderOrder);
    MaterialCreateInfo&                 setName(const char* name);
    ProgramResource*                    program;
    std::string                         name;
    u32                                 renderOrder = -1;
};

struct uniformRenderData{
    alignas(16)glm::vec3                lightDirection;
};

struct Material:public Resource{
    void                                addTexture(Renderer* renderer, std::string path);
    u32                                 poolIdx;
    ProgramResource*                    program;
    u32                                 renderOrder;
    TextureResource*                    LUTTexture;
    TextureResource*                    radianceTexture;
    TextureResource*                    iradianceTexture;
};

struct ProgramCreateInfo{
    ProgramCreateInfo&                  setName(const char* name);
    pipelineCreateInfo                  pipelineInfo;
    DescriptorSetLayoutCreateInfo       dslyaoutInfo;
    std::string                         name;
};

struct ResourceCache{
    void                                             destroy(Renderer* renderer);

    std::unordered_map<std::string, TextureResource*> textures;
    std::unordered_map<std::string, SamplerResource*> samplers;
    std::unordered_map<std::string, BufferResource*>  buffers;
    std::unordered_map<std::string, ProgramResource*> programs;
    std::unordered_map<std::string, Material*>        materials;

};
struct Renderer{
public:
    void                                        init(std::shared_ptr<DeviceContext> context);
    void                                        destroy();

    TextureResource*                            createTexture( TextureCreateInfo& textureInfo);
    BufferResource*                             createBuffer( BufferCreateInfo& bufferInfo);
    SamplerResource*                            createSampler( SamplerCreateInfo& samplerInfo);
    ProgramResource*                            createProgram( ProgramCreateInfo& programInfo);
    Material*                                   createMaterial( MaterialCreateInfo& matInfo);


    void                                        destroyTexture(TextureResource* textureRes);
    void                                        destroyBuffer(BufferResource* bufferRes);
    void                                        destroySampler(SamplerResource* samplerRes);
    void                                        destroyProgram(ProgramResource* progRes);
    void                                        destroyMaterial(Material* material);

    std::shared_ptr<DeviceContext>              getContext(){return m_context;}
    CommandBuffer*                              getCommandBuffer();
    void                                        setCurrentMaterial(Material* material);
    Material*                                   getCurrentMaterial();
    std::shared_ptr<objLoader>                  getObjLoader(){return m_objLoader;}
    std::shared_ptr<gltfLoader>                 getGltfLoader(){return m_gltfLoader;}
    void                                        newFrame();
    void                                        present();
    void                                        drawScene();
    void                                        drawUI();
    void                                        loadFromPath(const std::string& path);
    void                                        executeScene();

private:
    ResourceCache                               m_resourceCache;
    RenderResourcePool<TextureResource>         m_textures;
    RenderResourcePool<BufferResource>          m_buffers;
    RenderResourcePool<Material>                m_materials;
    RenderResourcePool<ProgramResource>         m_programs;
    RenderResourcePool<SamplerResource>         m_samplers;
    std::shared_ptr<DeviceContext>              m_context;
    std::shared_ptr<objLoader>                  m_objLoader;
    std::shared_ptr<gltfLoader>                 m_gltfLoader;
    Material*                                   m_currentMaterial = nullptr;

};

}
#endif //RENDERER_H
