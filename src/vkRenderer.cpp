#include "vkRenderer.h"
#include "GLFW/glfw3.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <stdexcept>

void VulkanRenderer::createInstance(){
    vk::ApplicationInfo appInfo("YF",1,"YFENG",1,VK_API_VERSION_1_3);
    vk::InstanceCreateInfo insInfo;
    uint32_t glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);//这里要分清楚那些是实例扩展，那些是设备扩展，可以通过vkEnumerateInstance/deviceExtensionProperties函数来列举
    insInfo.setEnabledExtensionCount(glfwExtensionCount);
    insInfo.setPpEnabledExtensionNames(glfwExtensions);

#ifdef DEBUG
    std::cout<<"Debug is on"<<std::endl;
    insInfo.setEnabledLayerCount(InstanceLayers.size());
    insInfo.setPpEnabledLayerNames(InstanceLayers.data());
#endif //DEBUG
    vk::Result res = vk::createInstance(&insInfo,nullptr,&m_instance);
    if(res!= vk::Result::eSuccess)
    {
        throw  std::runtime_error("ERROR: Failed to create instance!");
    }
}

void VulkanRenderer::deviceInit(){
    m_physicalDevice = m_instance.enumeratePhysicalDevices().front();
    std::vector<vk::QueueFamilyProperties> QueueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
    auto propertyIterator = std::find_if(QueueFamilyProperties.begin(),QueueFamilyProperties.end(),[](vk::QueueFamilyProperties& queueProp){return queueProp.queueFlags&vk::QueueFlagBits::eGraphics&vk::QueueFlagBits::eCompute;});
     m_queueFamilyIdx = std::distance(QueueFamilyProperties.begin(),propertyIterator);
    assert(m_queueFamilyIdx<QueueFamilyProperties.size());
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(m_queueFamilyIdx), 1,&queuePriority);
    vk::DeviceCreateInfo deviceCInfo(vk::DeviceCreateFlags(),deviceQueueCreateInfo);
    m_device = m_physicalDevice.createDevice(deviceCInfo);
}

void VulkanRenderer::createCommandBuffer(){
    vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlags(),m_queueFamilyIdx);
    vk::Result res = m_device.createCommandPool(&commandPoolCreateInfo,nullptr,&m_commandPool);
    if(res != vk::Result::eSuccess){
        throw std::runtime_error("ERROR: CommandPool created failed!");
    }
    vk::CommandBufferAllocateInfo CBAllocInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, m_commandBuffers.size());
    res = m_device.allocateCommandBuffers(&CBAllocInfo, m_commandBuffers.data());
    if(res != vk::Result::eSuccess){
        throw std::runtime_error("ERROR: CommandBuffer create failed!");
    }
    vk::Buffer m_buffer;
}

void VulkanRenderer::createSourface(){
    //glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
}

void VulkanRenderer::initWindow(){
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WIDTH,HEIGHT,"VulkanRenderer", nullptr, nullptr);
}

void VulkanRenderer::initVulkan(){
    createInstance();
    deviceInit();
    createCommandBuffer();
    
}


