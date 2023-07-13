#pragma once
#include "common.h"
#include "allocateObject.h"
#include "Camera.h"
#include "DataStructures.h"
class VulkanRenderer{
public:
    void fun();
    void initWindow();
    void initVulkan();
    void rendering();
    void clear();
    void createInstance();
    void deviceInit();
    void createCommandBuffer();
    void createSurface();
    void createSwapChain();
    void setupDebugMessenger();
    void initDepthBuffer();
    void createUniformBuffers();
//----------------pipeline---------------

    void createPipeline();

    //--------------utils---------------------
    void createTexture(const vk::ImageCreateInfo& imgInfo, const vk::Format& format, Texture& texture );
    void createBuffer(const vk::BufferCreateInfo& bufferInfo, Buffer& buffer);

    vk::Instance m_instance;
    vk::ApplicationInfo info;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_device;
    vk::SwapchainKHR m_swapchain;
    std::vector<vk::Image> m_swapChainImage;
    vk::Format m_swapChainFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<vk::ImageView> m_swapChainImageViews;
    std::vector<vk::Framebuffer> m_swapChainFrameBuffers;
    vk::CommandPool m_commandPool;
    vk::DebugUtilsMessengerEXT m_debugMessenger;

    uint32_t graphicIndex;
    uint32_t computeIndex;
    uint32_t presentIndex;
    vk::Queue m_graphicQueue;
    vk::Queue m_computeQueue;
    vk::Queue m_presentQueue;
    std::set<uint32_t> m_queueFamilyIdx;

    vk::DescriptorSetLayout m_descriptorSetLayout;

    vk::Pipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;
    Texture m_depthImage;
    std::vector<Buffer> m_uniformBuffers;
    std::vector<void*> m_uniformMapped;
    Camera m_camera;
    std::array<vk::CommandBuffer,3> m_commandBuffers;
    vk::SurfaceKHR m_surface;
    GLFWwindow* m_window;
    
};