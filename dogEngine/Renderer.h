#ifndef RENDERER_H
#define RENDERER_H
#include "DeviceContext.h"
#include "ModelLoader.h"
#include "Vertex.h"
#include <chrono>
#include <unordered_map>
#include "dgResource.h"
namespace dg {
    std::vector<char> readFile(const std::string &filename);

    struct Renderer;
    struct objLoader;
    struct gltfLoader;

    struct ResourceCache {
        void destroy(Renderer *renderer);

        TextureResource* queryTexture(const std::string& name);
        SamplerResource* querySampler(const std::string& name);
        BufferResource* QueryBuffer(const std::string& name);
        Material* QueryMaterial(const std::string& name);


        std::unordered_map<std::string, TextureResource *> textures;
        std::unordered_map<std::string, SamplerResource *> samplers;
        std::unordered_map<std::string, BufferResource *> buffers;
        std::unordered_map<std::string, Material *> materials;
    };
    struct Renderer {
    public:
        void init(std::shared_ptr<DeviceContext> context);
        void destroy();

        TextureResource *createTexture(TextureCreateInfo &textureInfo);
        BufferResource *createBuffer(BufferCreateInfo &bufferInfo);
        SamplerResource *createSampler(SamplerCreateInfo &samplerInfo);
        std::shared_ptr<Program> createProgram(const std::string &name, ProgramPassCreateInfo progPassCI);
        Material *createMaterial(MaterialCreateInfo &matInfo);
        void addSkyBox(std::string skyTexPath);

        TextureHandle upLoadTextureToGPU(std::string &texPath, TextureCreateInfo &texInfo);
        void addImageBarrier(VkCommandBuffer cmdBuffer, Texture *texture, ResourceState oldState, ResourceState newState, u32 baseMipLevel, u32 mipCount, bool isDepth);
        template<typename T>
        BufferHandle upLoadBufferToGPU(std::vector<T> &bufferData, const char *meshName);


        void destroyTexture(TextureResource *textureRes);
        void destroyBuffer(BufferResource *bufferRes);
        void destroySampler(SamplerResource *samplerRes);
        void destroyMaterial(Material *material);

        std::shared_ptr<DeviceContext> getContext() { return m_context; }
        void setDefaultMaterial(Material *material);
        Material *getDefaultMaterial();
        std::shared_ptr<objLoader> getObjLoader() { return m_objLoader; }
        std::shared_ptr<gltfLoader> getGltfLoader() { return m_gltfLoader; }
        ResourceCache &getResourceCache() { return this->m_resourceCache; }
        RenderResourcePool<TextureResource> &getTextures() { return m_textures; }
        RenderResourcePool<BufferResource> &getBuffers() { return m_buffers; }
        RenderResourcePool<Material> &getMaterials() { return m_materials; }
        RenderResourcePool<SamplerResource> &getSamplers() { return m_samplers; }
        TextureHandle getSkyTexture() { return m_skyTexture; }
        void newFrame();
        void present();
        void drawScene();
        void drawUI();
        void loadFromPath(const std::string &path);
        void executeScene();

    protected:
        void executeSkyBox();
        void destroySkyBox();
        void makeDefaultMaterial();

    private:
        ResourceCache m_resourceCache;
        RenderResourcePool<TextureResource> m_textures;
        RenderResourcePool<BufferResource> m_buffers;
        RenderResourcePool<Material> m_materials;
        RenderResourcePool<SamplerResource> m_samplers;
        std::vector<RenderObject> m_renderObjects;
        std::shared_ptr<DeviceContext> m_context;
        std::shared_ptr<objLoader> m_objLoader;
        std::shared_ptr<gltfLoader> m_gltfLoader;
        Material *m_defaultMaterial;

    private:
        TextureHandle m_skyTexture;
        bool have_skybox;
    };


    template<typename T>
    BufferHandle Renderer::upLoadBufferToGPU(std::vector<T> &bufferData, const char *meshName) {
        if (bufferData.empty()) return {k_invalid_index};
        VkDeviceSize BufSize = bufferData.size() * sizeof(T);
        BufferCreateInfo bufferInfo;
        bufferInfo.reset();
        bufferInfo.setData(bufferData.data());
        bufferInfo.setDeviceOnly(true);
        if (std::is_same<T, Vertex>::value) {
            std::string vertexBufferName = "vt";
            vertexBufferName += meshName;
            bufferInfo.setUsageSize(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, BufSize);
            bufferInfo.setName(vertexBufferName.c_str());
        } else if (std::is_same<T, u32>::value) {
            std::string IndexBufferName = "id";
            IndexBufferName += meshName;
            bufferInfo.setUsageSize(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, BufSize);
            bufferInfo.setName(IndexBufferName.c_str());
        }
        BufferHandle GPUHandle = createBuffer(bufferInfo)->handle;
        return GPUHandle;
    }
}// namespace dg
#endif//RENDERER_H
