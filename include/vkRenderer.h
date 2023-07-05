#include "common.h"

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
    void createSourface();
    vk::Instance m_instance;
    vk::ApplicationInfo info;
    vk::PhysicalDevice m_physicalDevice;
    vk::Device m_device;
    vk::CommandPool m_commandPool;
    uint32_t m_queueFamilyIdx;
    std::array<vk::CommandBuffer,3> m_commandBuffers;
    vk::SurfaceKHR m_surface;
    GLFWwindow* window;
    vk::Pipeline m_pipeline;
};