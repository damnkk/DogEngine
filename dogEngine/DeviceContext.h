#ifndef CONTEXT_H
#define CONTEXT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "gpuResource.hpp"
#include "ResourcePool.hpp"
#include "DebugMessanger.h"
#include "dgAssert.h"
#include "Camera.h"
namespace dg{

struct CommandBuffer;
struct constentData {
  glm::mat4 modelMatrix;
};

const int MAX_FRAMES_IN_FLIGHT = 3;

struct UniformData{
    alignas(16) glm::vec3                           cameraPos;
    alignas(16) glm::mat4                           modelMatrix;
    alignas(16) glm::mat4                           viewMatrix;
    alignas(16) glm::mat4                           projectMatrix;
};

struct ContextCreateInfo{
    GLFWwindow*             m_window;
    u16                     m_windowWidth = 1280;
    u16                     m_windowHeight = 720;
    u16                     m_gpu_time_queries_per_frame    = 32;
    bool                    m_enable_GPU_time_queries         = false;
    bool                    m_debug                           = false;
    const char*             m_applicatonName;
    ContextCreateInfo&      set_window(u32 width, u32 height, void* windowHandle);
};

struct DeviceContext{
    void                                        init(const ContextCreateInfo& createinfo);
    void                                        Destroy();

    TextureHandle                               createTexture(const TextureCreateInfo& tcf);
    BufferHandle                                createBuffer(const BufferCreateInfo& bcf);
    PipelineHandle                              createPipeline(const pipelineCreateInfo& pcf);
    DescriptorSetLayoutHandle                   createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& dcf);
    DescriptorSetHandle                         createDescriptorSet(DescriptorSetCreateInfo& dcf);
    RenderPassHandle                            createRenderPass(const RenderPassCreateInfo& rcf);
    ShaderStateHandle                           createShaderState(const ShaderStateCreation& scf);
    SamplerHandle                               createSampler(const SamplerCreateInfo& scf);
    FrameBufferHandle                           createFrameBuffer(const FrameBufferCreateInfo& fcf);

    void                                        DestroyTexture(TextureHandle  texIndex);
    void                                        DestroyBuffer(BufferHandle    bufferIndex);
    void                                        DestroyPipeline(PipelineHandle  pipelineIndex);
    void                                        DestroyDescriptorSetLayout(DescriptorSetLayoutHandle  dslIndex);
    void                                        DestroyDescriptorSet(DescriptorSetHandle  dsIndex);
    void                                        DestroyRenderPass(RenderPassHandle  rdIndex);
    void                                        DestroyShaderState(ShaderStateHandle  texIndex);
    void                                        DestroySampler(SamplerHandle sampIndex);
    void                                        DestroyFrameBuffer(FrameBufferHandle fboIndex);

    //query description


    //swapchain
    void                                        createSwapChain();
    void                                        destroySwapChain();
    void                                        reCreateSwapChain();

    void                                        resizeOutputTextures(FrameBufferHandle frameBuffer, u32 width, u32 height);
    void                                        updateDescriptorSet(DescriptorSetHandle set);

    //Rendering
    void                                        newFrame();
    void                                        present();
    void                                        resize(u16 width, u16 height);
    bool                                        setPresentMode(VkPresentModeKHR presentMode);
    void                                        linkTextureSampler(TextureHandle texture,SamplerHandle sampler);

    //names and markers
    //中间的这个handle说白了就是你要命名的那个Vulkan对象, Vulkan对象可以直接隐式地转为uint64,
    void                                        setResourceName(VkObjectType objectType, uint64_t handle, const char* name);
    void                                        pushMarker(VkCommandBuffer command_buffer, const char* name);
    void                                        popMarker(VkCommandBuffer command_buffer);

    CommandBuffer*                              getCommandBuffer(QueueType::Enum type, bool begin);
    CommandBuffer*                              getInstantCommandBuffer();
    void                                        queueCommandBuffer(CommandBuffer* cmdbuffer);

    //instance methods(真真正正的通过Vulkan去完成的东西)
    void                                        destroyBufferInstant(ResourceHandle buffer);
    void                                        destroyTextureInstant(ResourceHandle texture);
    void                                        destroyPipelineInstant(ResourceHandle pipeline);
    void                                        destroySamplerInstant(ResourceHandle sampler);
    void                                        destroyDescriptorSetLayoutInstant(ResourceHandle descriptorSetLayout);
    void                                        destroyDescriptorInstant(ResourceHandle descriptor);
    void                                        destroyRenderpassInstant(ResourceHandle renderPass);
    void                                        destroyShaderStateInstant(ResourceHandle shader);
    void                                        destroyFrameBufferInstant(ResourceHandle fbo);

    void                                        updateDescriptorSetInstant(const DescriptorSetUpdate& update);
    bool                                        getFamilyIndex(VkPhysicalDevice& device);



    ResourcePool<Buffer>                        m_buffers;
    ResourcePool<Texture>                       m_textures;
    ResourcePool<Pipeline>                      m_pipelines;
    ResourcePool<Sampler>                       m_samplers;
    ResourcePool<DescriptorSetLayout>           m_descriptorSetLayouts;
    ResourcePool<DescriptorSet>                 m_descriptorSets;
    ResourcePool<RenderPass>                    m_renderPasses;
    ResourcePool<ShaderState>                   m_shaderStates;
    ResourcePool<FrameBuffer>                   m_frameBuffers;
    //RssourcePool<Shader>                        m_shaders;
    SamplerHandle                               m_defaultSampler;
    BufferHandle                                m_FullScrVertexBuffer;
    RenderPassHandle                            m_swapChainPass;
    TextureHandle                               m_depthTexture;
    TextureHandle                               m_defaultTexture;
    std::vector<CommandBuffer*>                 m_queuedCommandBuffer;
    std::vector<ResourceUpdate>                 m_delectionQueue;
    std::vector<DescriptorSetUpdate>            m_descriptorSetUpdateQueue;

    VkSemaphore                                 m_render_complete_semaphore[k_max_swapchain_images];
    VkSemaphore                                 m_image_acquired_semaphore;
    VkFence                                     m_render_queue_complete_fence[k_max_swapchain_images];
    BufferHandle                                m_viewProjectUniformBuffer;

    u32                                         m_swapChainWidth;
    u32                                         m_swapChainHeight;
    u32                                         m_swapchainImageCount = 3;
    u32                                         m_currentSwapchainImageIndex;
    std::vector<VkImage>                        m_swapchainImages;
    std::vector<VkImageView>                    m_swapchainImageViews;
    std::vector<VkFramebuffer>                  m_swapchainFbos;
    bool                                        m_resized = false;

    u32                                         m_graphicQueueIndex     = -1;
    u32                                         m_computeQueueIndex     = -1;
    u32                                         m_transferQueueIndex    = -1;
    u32                                         m_currentFrame;
    u32                                         m_previousFrame;
    bool                                        m_debugUtilsExtentUsed = true;
    
    
    VkPhysicalDevice                            m_physicalDevice;
    VkDevice                                    m_logicDevice;
    VkInstance                                  m_instance;
    VkSurfaceKHR                                m_surface;
    VkSurfaceFormatKHR                          m_surfaceFormat;
    VkPresentModeKHR                            m_presentMode;
    VkSwapchainKHR                              m_swapchain;
    VkPhysicalDeviceProperties                  m_phyDeviceProperty;
    static GLFWwindow*                          m_window;
    VkDescriptorPool                            m_descriptorPool;
    VkQueue                                     m_computeQueue;
    VkQueue                                     m_graphicsQueue;
    VkQueue                                     m_transferQueue;
    static std::shared_ptr<Camera>              m_camera;

    VmaAllocator                                m_vma;
    PipelineHandle                              m_pbrPipeline{k_invalid_index};
    PipelineHandle                              m_nprPipeline{k_invalid_index};
    PipelineHandle                              m_rayTracingPipeline{k_invalid_index};


    Sampler* accessSampler(SamplerHandle samplerhandle);
    const Sampler* accessSampler(SamplerHandle samplerhandle) const;

    Texture* accessTexture(TextureHandle texturehandle);
    const Texture* accessTexture(TextureHandle texturehandle) const;

    RenderPass* accessRenderPass(RenderPassHandle renderpasshandle);
    const RenderPass* accessRenderPass(RenderPassHandle renderpasshandle) const;

    DescriptorSetLayout*       accessDescriptorSetLayout( DescriptorSetLayoutHandle layout );
    const DescriptorSetLayout* accessDescriptorSetLayout( DescriptorSetLayoutHandle layout ) const;

    DescriptorSet*             accessDescriptorSet( DescriptorSetHandle set );
    const DescriptorSet*       accessDescriptorSet( DescriptorSetHandle set ) const;

    Buffer* accessBuffer(BufferHandle bufferhandle);
    const Buffer* accessBuffer(BufferHandle bufferhandle) const;

    ShaderState* accessShaderState(ShaderStateHandle handle);
    const ShaderState* accessShaderState(ShaderStateHandle handle) const;
    Pipeline* accessPipeline(PipelineHandle pipeline);
    const Pipeline* accessPipeline(PipelineHandle pipeline) const;

    FrameBuffer* accessFrameBuffer(FrameBufferHandle handle);
    const FrameBuffer* accessFrameBuffer(FrameBufferHandle handle) const;

    
};

} //namespace dg

#endif //CONTEXT_H
