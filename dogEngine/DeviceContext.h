#ifndef CONTEXT_H
#define CONTEXT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "dgpch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLFW_INCLUDE_VULKAN
#include "Camera.h"
#include "DebugMessanger.h"
#include "ResourcePool.hpp"
#include "dgAssert.h"
#include "gpuResource.hpp"
#include "renderdoc/renderdoc_app.h"
#include <GLFW/glfw3.h>
namespace dg {


    struct CommandBuffer;

    const int MAX_FRAMES_IN_FLIGHT = 3;

    struct UniformData {
        alignas(16) glm::vec3 cameraPos;
        alignas(16) glm::vec3 cameraDirectory;
        alignas(16) glm::mat4 viewMatrix;
        alignas(16) glm::mat4 projectMatrix;
    };

    struct ContextCreateInfo {
        GLFWwindow *m_window;
        u16 m_windowWidth = 1280;
        u16 m_windowHeight = 720;
        u16 m_gpu_time_queries_per_frame = 32;
        bool m_enable_GPU_time_queries = false;
        bool m_debug = false;
        const char *m_applicatonName;
        ContextCreateInfo &set_window(u32 width, u32 height, void *windowHandle);
    };

    struct DeviceContext {
        void init(const ContextCreateInfo &createinfo);
        void Destroy();

        TextureHandle createTexture(TextureCreateInfo &tcf);
        BufferHandle createBuffer(BufferCreateInfo &bcf);
        PipelineHandle createPipeline(PipelineCreateInfo &pcf);
        DescriptorSetLayoutHandle createDescriptorSetLayout(DescriptorSetLayoutCreateInfo &dcf);
        DescriptorSetHandle createDescriptorSet(DescriptorSetCreateInfo &dcf);
        RenderPassHandle createRenderPass(RenderPassCreateInfo &rcf);
        ShaderStateHandle createShaderState(ShaderStateCreation &scf);
        SamplerHandle createSampler(SamplerCreateInfo &scf);
        FrameBufferHandle createFrameBuffer(FrameBufferCreateInfo &fcf);

        void DestroyTexture(TextureHandle &texIndex);
        void DestroyBuffer(BufferHandle &bufferIndex);
        void DestroyPipeline(PipelineHandle &pipelineIndex);
        void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle &dslIndex);
        void DestroyDescriptorSet(DescriptorSetHandle &dsIndex);
        void DestroyRenderPass(RenderPassHandle &rdIndex);
        void DestroyShaderState(ShaderStateHandle &texIndex);
        void DestroySampler(SamplerHandle &sampIndex);
        void DestroyFrameBuffer(FrameBufferHandle &fboIndex, bool destroyTexture = true);

        //query description

        //swapchain
        void createSwapChain();
        void destroySwapChain();
        void reCreateSwapChain();

        void resizeOutputTextures(FrameBufferHandle frameBuffer, u32 width, u32 height);
        void updateDescriptorSet(DescriptorSetHandle set);
        void addImageBarrier(VkCommandBuffer cmdBuffer, Texture *texture, ResourceState oldState, ResourceState newState, u32 baseMipLevel, u32 mipCount, bool isDepth);

        //Rendering
        void newFrame();
        void present();
        void resize(u16 width, u16 height);
        bool setPresentMode(VkPresentModeKHR presentMode);
        void linkTextureSampler(TextureHandle texture, SamplerHandle sampler);

        //names and markers
        //中间的这个handle说白了就是你要命名的那个Vulkan对象, Vulkan对象可以直接隐式地转为uint64,
        void setResourceName(VkObjectType objectType, uint64_t handle, const char *name);
        void pushMarker(VkCommandBuffer command_buffer, const char *name);
        void popMarker(VkCommandBuffer command_buffer);

        CommandBuffer *getCommandBuffer(QueueType::Enum type, bool begin);
        CommandBuffer *getInstantCommandBuffer();
        void queueCommandBuffer(CommandBuffer *cmdbuffer);

        //instance methods(真真正正的通过Vulkan去完成的东西)
        void destroyBufferInstant(ResourceHandle buffer);
        void destroyTextureInstant(ResourceHandle texture);
        void destroyPipelineInstant(ResourceHandle pipeline);
        void destroySamplerInstant(ResourceHandle sampler);
        void destroyDescriptorSetLayoutInstant(ResourceHandle descriptorSetLayout);
        void destroyDescriptorInstant(ResourceHandle descriptor);
        void destroyRenderpassInstant(ResourceHandle renderPass);
        void destroyShaderStateInstant(ResourceHandle shader);
        void destroyFrameBufferInstant(ResourceHandle fbo);

        void updateDescriptorSetInstant(const DescriptorSetUpdate &update);
        bool getFamilyIndex(VkPhysicalDevice &device);


        ResourcePool<Buffer> m_buffers;
        ResourcePool<Texture> m_textures;
        ResourcePool<Pipeline> m_pipelines;
        ResourcePool<Sampler> m_samplers;
        ResourcePool<DescriptorSetLayout> m_descriptorSetLayouts;
        ResourcePool<DescriptorSet> m_descriptorSets;
        ResourcePool<RenderPass> m_renderPasses;
        ResourcePool<ShaderState> m_shaderStates;
        ResourcePool<FrameBuffer> m_frameBuffers;
        //RssourcePool<Shader>                        m_shaders;
        BufferHandle m_FullScrVertexBuffer;
        RenderPassHandle m_swapChainPass;
        TextureHandle m_depthTexture;
        TextureHandle m_DummyTexture;
        SamplerHandle m_defaultSampler;
        DescriptorSetLayoutCreateInfo m_defaultSetLayoutCI;
        std::vector<CommandBuffer *> m_queuedCommandBuffer;
        std::vector<ResourceUpdate> m_delectionQueue;
        std::vector<DescriptorSetUpdate> m_descriptorSetUpdateQueue;

        VkSemaphore m_render_complete_semaphore[k_max_swapchain_images];
        VkSemaphore m_image_acquired_semaphore;
        VkFence m_render_queue_complete_fence[k_max_swapchain_images];
        BufferHandle m_viewProjectUniformBuffer;

        u32 m_swapChainWidth;
        u32 m_swapChainHeight;
        u32 m_swapchainImageCount = 3;
        u32 m_currentSwapchainImageIndex;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        std::vector<VkFramebuffer> m_swapchainFbos;
        bool m_resized = false;

        u32 m_graphicQueueIndex = -1;
        u32 m_computeQueueIndex = -1;
        u32 m_transferQueueIndex = -1;
        u32 m_currentFrame;
        u32 m_previousFrame;
        bool m_debugUtilsExtentUsed = true;


        VkPhysicalDevice m_physicalDevice;
        VkDevice m_logicDevice;
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkSurfaceFormatKHR m_surfaceFormat;
        VkPresentModeKHR m_presentMode;
        VkSwapchainKHR m_swapchain;
        VkPhysicalDeviceProperties m_phyDeviceProperty;
        static GLFWwindow *m_window;
        VkDescriptorPool m_descriptorPool;
        VkDescriptorPool m_bindlessDescriptorPool;
        VkDescriptorSet m_VulkanBindlessDescriptorSet;
        DescriptorSetLayoutHandle m_bindlessDescriptorSetLayout;
        DescriptorSetHandle m_bindlessDescriptorSet;
        std::vector<ResourceUpdate> m_texture_to_update_bindless;
        VkQueue m_computeQueue;
        VkQueue m_graphicsQueue;
        VkQueue m_transferQueue;
        static std::shared_ptr<Camera> m_camera;

        VmaAllocator m_vma;
        RENDERDOC_API_1_3_0 *m_renderDoc_api = NULL;


        Sampler *accessSampler(SamplerHandle samplerhandle);
        const Sampler *accessSampler(SamplerHandle samplerhandle) const;

        Texture *accessTexture(TextureHandle texturehandle);
        const Texture *accessTexture(TextureHandle texturehandle) const;

        RenderPass *accessRenderPass(RenderPassHandle renderpasshandle);
        const RenderPass *accessRenderPass(RenderPassHandle renderpasshandle) const;

        DescriptorSetLayout *accessDescriptorSetLayout(DescriptorSetLayoutHandle layout);
        const DescriptorSetLayout *accessDescriptorSetLayout(DescriptorSetLayoutHandle layout) const;

        DescriptorSet *accessDescriptorSet(DescriptorSetHandle set);
        const DescriptorSet *accessDescriptorSet(DescriptorSetHandle set) const;

        Buffer *accessBuffer(BufferHandle bufferhandle);
        const Buffer *accessBuffer(BufferHandle bufferhandle) const;

        ShaderState *accessShaderState(ShaderStateHandle handle);
        const ShaderState *accessShaderState(ShaderStateHandle handle) const;
        Pipeline *accessPipeline(PipelineHandle pipeline);

        FrameBuffer *accessFrameBuffer(FrameBufferHandle handle);
        const FrameBuffer *accessFrameBuffer(FrameBufferHandle handle) const;
    };

    static GLFWwindow* createGLFWWindow(u32 width,u32 height) {
        DG_INFO("Window Initiation")
        if(!glfwInit()){
            exit(-1);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(width, height, "DogEngine", nullptr, nullptr);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);
        return window;
    }

}//namespace dg

#endif//CONTEXT_H
