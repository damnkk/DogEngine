#ifndef RENDERER_H
#define RENDERER_H
#include <chrono>
#include <unordered_map>
#include "GUI/gui.h"
#include "DeviceContext.h"

namespace dg{

void keycallback(GLFWwindow *window);

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

struct Material:public Resource{
    u32                                 poolIdx;
    ProgramResource*                    program;
    u32                                 renderOrder;
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

    TextureResource*                            createTexture(const TextureCreateInfo& textureInfo);
    BufferResource*                             createBuffer(const BufferCreateInfo& bufferInfo);
    SamplerResource*                            createSampler(const SamplerCreateInfo& samplerInfo);
    ProgramResource*                            createProgram(const ProgramCreateInfo& programInfo);
    Material*                                   createMaterial(const MaterialCreateInfo& matInfo);


    void                                        destroyTexture(TextureResource* textureRes);
    void                                        destroyBuffer(BufferResource* bufferRes);
    void                                        destroySampler(SamplerResource* samplerRes);
    void                                        destroyProgram(ProgramResource* progRes);
    void                                        destroyMaterial(Material* material);

    std::shared_ptr<DeviceContext>              getContext(){return m_context;}
    void                                        newFrame();
    void                                        present();
    void                                        drawScene();
    void                                        drawUI();
    void                                        loadFromPath(const std::string& path);
    void                                        executeScene();
    CommandBuffer*                              getCommandBuffer();

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

};

}
#endif //RENDERER_H
