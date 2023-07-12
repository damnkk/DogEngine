#pragma once
#include "common.h"
#include "allocateObject.h"

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
    void createPipeline();

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

    vk::Pipeline m_pipeline;
    uint32_t graphicIndex;
    uint32_t computeIndex;
    uint32_t presentIndex;
    vk::Queue m_graphicQueue;
    vk::Queue m_computeQueue;
    vk::Queue m_presentQueue;
    std::set<uint32_t> m_queueFamilyIdx;

    Texture m_depthImage;
    std::array<vk::CommandBuffer,3> m_commandBuffers;
    vk::SurfaceKHR m_surface;
    GLFWwindow* m_window;
};