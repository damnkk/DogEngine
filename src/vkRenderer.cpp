#include "vkRenderer.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <stdexcept>
#include "DataStructures.h"
#include "allocateObject.h"
#include "assert.h"
//-------------------------------Debug----------------------------------
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance,pCreateInfo,pAllocator,pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger,pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                                void* /*pUserData*/)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

std::vector<char> readFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate|std::ios::binary);
    if(!file.is_open()){
        throw std::runtime_error("filed to open shader file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(),fileSize);
    file.close();
    return buffer;
}



void VulkanRenderer::createInstance(){
    vk::ApplicationInfo appInfo("YF",1,"YFENG",1,VK_API_VERSION_1_3);
    vk::InstanceCreateInfo insInfo;
    uint32_t glfwExtensionCount=0;
   
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);//这里要分清楚那些是实例扩展，那些是设备扩展，可以通过vkEnumerateInstance/deviceExtensionProperties函数来列举
    std::cout<<glfwExtensionCount;
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions+glfwExtensionCount);
    for(auto& i : extensions){
        std::cout<<i<<std::endl;
    }
#ifdef DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif //DEBUG
    insInfo.setEnabledExtensionCount(extensions.size());
    insInfo.setPpEnabledExtensionNames(extensions.data());

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

void VulkanRenderer::setupDebugMessenger(){
    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(m_instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if(!pfnVkCreateDebugUtilsMessengerEXT){
        throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");
    }
    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( m_instance.getProcAddr( "vkDestroyDebugUtilsMessengerEXT" ) );
    if ( !pfnVkDestroyDebugUtilsMessengerEXT )
    {
      throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
    }
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags{vk::DebugUtilsMessageSeverityFlagBitsEXT::eError|vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning};
    vk::DebugUtilsMessageTypeFlagsEXT typeFlags{vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral|vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation|vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance };
    vk::DebugUtilsMessengerEXT messenger = m_instance.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT({},severityFlags,typeFlags,&debugMessageFunc));
}

uint32_t findQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties,vk::QueueFlagBits FamilyType){
    auto propertyIterator = std::find_if(queueFamilyProperties.begin(),queueFamilyProperties.end(),[&](vk::QueueFamilyProperties queueProp){return queueProp.queueFlags&FamilyType;});
    return std::distance(queueFamilyProperties.begin(),propertyIterator);
}

void VulkanRenderer::deviceInit(){
    m_physicalDevice = m_instance.enumeratePhysicalDevices().front();
    std::vector<vk::QueueFamilyProperties> QueueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
    graphicIndex = findQueueFamilyIndex(QueueFamilyProperties,vk::QueueFlagBits::eGraphics);
    computeIndex = findQueueFamilyIndex(QueueFamilyProperties, vk::QueueFlagBits::eCompute);
    int i = 0;
    presentIndex =static_cast<uint32_t>(m_physicalDevice.getSurfaceSupportKHR(graphicIndex, m_surface)?graphicIndex:QueueFamilyProperties.size());
    if(presentIndex == QueueFamilyProperties.size()){
        for (size_t i = 0;i<QueueFamilyProperties.size();++i){
            if((QueueFamilyProperties[i].queueFlags&vk::QueueFlagBits::eGraphics)&&
            m_physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),m_surface)){
                graphicIndex = i;
                presentIndex = i;
            }
        }
    }
    if(presentIndex == QueueFamilyProperties.size()){
        throw std::runtime_error("This device can not present images!");
    }
    m_queueFamilyIdx = {graphicIndex, computeIndex, presentIndex};
    
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for(auto idx:m_queueFamilyIdx){
        auto queueInfo =   vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), idx, 1, &queuePriority);
        queueCreateInfos.push_back(queueInfo);
    }
    vk::DeviceCreateInfo deviceCInfo(vk::DeviceCreateFlags(),queueCreateInfos.size(),queueCreateInfos.data(),InstanceLayers.size(),InstanceLayers.data(),deviceExtensions.size(), deviceExtensions.data());
    m_device = m_physicalDevice.createDevice(deviceCInfo);
    m_graphicQueue = m_device.getQueue(graphicIndex, 0);
    m_computeQueue = m_device.getQueue(computeIndex, 0);
    m_presentQueue = m_device.getQueue(presentIndex, 0);
}

void VulkanRenderer::createCommandBuffer(){
    vk::CommandPoolCreateInfo commandPoolCreateInfo(vk::CommandPoolCreateFlags(), graphicIndex);
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

void VulkanRenderer::createSurface(){
    //glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface);
    m_surface = vk::SurfaceKHR(surface);
}

void VulkanRenderer::initWindow(){
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WIDTH,HEIGHT,"VulkanRenderer", nullptr, nullptr);
}

void VulkanRenderer::createSwapChain(){
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);
    assert(!surfaceFormats.empty());
    m_swapChainFormat  = (surfaceFormats[0].format == vk::Format::eUndefined)? vk::Format::eB8G8R8A8Srgb:surfaceFormats[0].format;
    vk::SurfaceCapabilitiesKHR surfaceCapa = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

    if(surfaceCapa.currentExtent.width == std::numeric_limits<uint32_t>::max()){
        m_swapChainExtent.width = std::clamp(surfaceCapa.currentExtent.width, surfaceCapa.minImageExtent.width, surfaceCapa.maxImageExtent.width);
        m_swapChainExtent.height = std::clamp(surfaceCapa.currentExtent.height, surfaceCapa.minImageExtent.height, surfaceCapa.maxImageExtent.height);
    }
    else{
        m_swapChainExtent = surfaceCapa.currentExtent;
    }
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
    vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapa.supportedTransforms& vk::SurfaceTransformFlagBitsKHR::eIdentity)?vk::SurfaceTransformFlagBitsKHR::eIdentity:surfaceCapa.currentTransform;
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vk::SwapchainCreateInfoKHR swapchainCreateInfo(vk::SwapchainCreateFlagsKHR(),
                                                    m_surface,
                                                    surfaceCapa.minImageCount+1,
                                                    m_swapChainFormat,
                                                    vk::ColorSpaceKHR::eSrgbNonlinear,
                                                    m_swapChainExtent,
                                                    1,
                                                    vk::ImageUsageFlagBits::eColorAttachment,
                                                    vk::SharingMode::eExclusive,
                                                    {},
                                                    preTransform,
                                                    compositeAlpha,
                                                    swapchainPresentMode,
                                                    true,
                                                    nullptr
                                                    );
    uint32_t queueFamilyIndices[2] = {graphicIndex, presentIndex };
    if(graphicIndex!=presentIndex){
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    m_swapchain = m_device.createSwapchainKHR(swapchainCreateInfo);
    m_swapChainImage = m_device.getSwapchainImagesKHR(m_swapchain);
    m_swapChainImageViews.reserve(m_swapChainImage.size());
    vk::ImageViewCreateInfo viewInfo({},{},vk::ImageViewType::e2D,m_swapChainFormat,{},{vk::ImageAspectFlagBits::eColor,0,1,0,1});
    for(auto image:m_swapChainImage){
        viewInfo.setImage(image);
        m_swapChainImageViews.push_back(m_device.createImageView(viewInfo));
    }
}

void VulkanRenderer::createPipeline(){
    auto vertexShaderCode = readFile("shaders/vert.spv");
    auto fragmentShaderCode = readFile("shaders/frag.spv");
    vk::ShaderModuleCreateInfo vertexShaderInfo(vk::ShaderModuleCreateFlags(),vertexShaderCode.size(),reinterpret_cast<const uint32_t*>(vertexShaderCode.data()));
    vk::ShaderModule vertexShaderModule = m_device.createShaderModule(vertexShaderInfo);
    vk::ShaderModuleCreateInfo fragShaderInfo(vk::ShaderModuleCreateFlags(),fragmentShaderCode.size(),reinterpret_cast<const uint32_t*>(fragmentShaderCode.data()));
    vk::ShaderModule fragShaderModule = m_device.createShaderModule(fragShaderInfo);

    std::array<vk::PipelineShaderStageCreateInfo,2> shaderStageInfos = {
        vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),vk::ShaderStageFlagBits::eVertex, vertexShaderModule, "main"),
        vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main")
    };

}

void VulkanRenderer::initDepthBuffer(){
    vk::Format depthFormat = vk::Format::eD32Sfloat;
}

void VulkanRenderer::initVulkan(){

    //init
    initWindow();
    createInstance();
#ifdef DEBUG
    setupDebugMessenger();
#endif //DEBUG
    createSurface();
    deviceInit();
    createCommandBuffer();
    //presentation
    createSwapChain();
   
    //draw
    createPipeline();





     initDepthBuffer();
    m_instance.destroy();
    
}


