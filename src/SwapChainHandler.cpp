#include "common.h"
#include "SwapChainHandler.h"

SwapChain::SwapChain(){
    m_SwapChain = 0;
    m_MainDevice  = {};
    m_VulkanSurface = 0;
    m_SwapChainExtent = {};
    m_SwapChainImageFormat = {};
}

SwapChain::SwapChain(MainDevice* mainDevice, VkSurfaceKHR* surface,
GLFWwindow* window, QueueFamilyIndices& QueueFamilyIndices){
    m_SwapChain = 0;
    m_MainDevice = mainDevice;
    m_QueueFamilyIndices = QueueFamilyIndices;
    m_VulkanSurface = surface;
    m_SwapChainExtent = {};
    m_SwapChainImageFormat = {};
}

VkSwapchainKHR* SwapChain::GetSwapChainData(){
    return &m_SwapChain;
}

VkSwapchainKHR& SwapChain::GetSwapChain(){
    return m_SwapChain;
}

SwapChainImage* SwapChain::GetImage(uint32_t index){
    return &m_SwapChainImages[index];
}

void SwapChain::PushImage(SwapChainImage swapChainImage){
    m_SwapChainImages.push_back(swapChainImage);
}

VkImageView& SwapChain::GetSwapChainImageView(uint32_t index){
    return m_SwapChainImages[index].imageView;
}

size_t SwapChain::SwapChainImagesSize() const{
    return m_SwapChainImages.size();
}

VkFramebuffer& SwapChain::GetFrameBuffer(uint32_t index){
    return m_SwapChainFrameBuffers[index];
}

std::vector<VkFramebuffer>& SwapChain::GetFrameBuffers(){
    return m_SwapChainFrameBuffers;
}

void SwapChain::PushFrameBuffer(VkFramebuffer frameBuffer){
    m_SwapChainFrameBuffers.push_back(frameBuffer);
}

void SwapChain::SetRenderPass(VkRenderPass* renderPass){
    m_RenderPass = renderPass;
}

void SwapChain::SetRecreationStatus(bool const status){
    m_IsRecreateing = status;
}

void SwapChain::ResizeFrameBuffers(){
    m_SwapChainFrameBuffers.resize(m_SwapChainImages.size());
}

size_t SwapChain::FrameBufferSize() const{ 
    return m_SwapChainFrameBuffers.size();
}

void SwapChain::CleanUpSwapChain(){
    
    DestroyFrameBuffers();
    DestroySwapChainImageViews();
    DestroySwapChain();
}

void SwapChain::DestroyFrameBuffers(){
    for(auto framebuffer : m_SwapChainFrameBuffers){
        vkDestroyFramebuffer(m_MainDevice->logicalDevice,framebuffer,nullptr);
    }
}

void SwapChain::DestroySwapChainImageViews(){
    for(auto image: m_SwapChainImages){
        vkDestroyImageView(m_MainDevice->logicalDevice, image.imageView,nullptr);
    }
}

void SwapChain::DestroySwapChain(){
    vkDestroySwapchainKHR(m_MainDevice->logicalDevice, m_SwapChain,nullptr);
}

uint32_t SwapChain::GetExtentWidth()const {
    return m_SwapChainExtent.width;
}

uint32_t SwapChain::GetExtentHeight() const{
    return m_SwapChainExtent.height;
}

VkExtent2D& SwapChain::GetExtent(){
    return m_SwapChainExtent;
}

VkFormat& SwapChain::GetSwapChainImageFormat(){
    return m_SwapChainImageFormat;
}

void SwapChain::CreateSwapChain(){
    SwapChainSupportDetails swapChainDetails = GetSwapChainDetails(m_MainDevice->physicalDevice,*m_VulkanSurface);
    VkExtent2D extent = ChooseSwapExtent(swapChainDetails.capabilities);
    VkSurfaceFormatKHR surfaceFormat = ChooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode = ChooseBestPresentationMode(swapChainDetails.presentModes);

    uint32_t imageCount = swapChainDetails.capabilities.minImageCount+1;

    if(swapChainDetails.capabilities.maxImageCount>0&&
       swapChainDetails.capabilities.maxImageCount<imageCount){
        imageCount = swapChainDetails.capabilities.maxImageCount;
       }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = *m_VulkanSurface;
    
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapChainCreateInfo.preTransform = swapChainDetails.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;

    if(m_QueueFamilyIndices.graphicsFamily !=m_QueueFamilyIndices.presentFamily){
        uint32_t queueFamilyIndices[] {
            static_cast<uint32_t>(m_QueueFamilyIndices.graphicsFamily.value()),
            static_cast<uint32_t>(m_QueueFamilyIndices.presentFamily.value())
        };

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 1;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    if(vkCreateSwapchainKHR(m_MainDevice->logicalDevice, &swapChainCreateInfo,nullptr,&m_SwapChain)!= VK_SUCCESS){
        throw std::runtime_error("failed to create swap chain!");
    }

    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;
    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(m_MainDevice->logicalDevice,m_SwapChain,&swapChainImageCount,nullptr);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(m_MainDevice->logicalDevice,m_SwapChain,& swapChainImageCount,images.data());
    if(!m_IsRecreateing){
        for(VkImage image:images){
            SwapChainImage swapChainImage = {};
            swapChainImage.image = image;
            swapChainImage.imageView = Utility::createImageView(image, m_SwapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT,1);
            m_SwapChainImages.push_back(swapChainImage);
        }
    }
    else {
        for(size_t  i = 0;i<m_SwapChainImages.size();++i){
            m_SwapChainImages[i].imageView = Utility::createImageView(images[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }
}

void SwapChain::ReCreateSwapChain(){
    vkDeviceWaitIdle(m_MainDevice->logicalDevice);
    CleanUpSwapChain();
    CreateSwapChain();
}

void SwapChain::CreateFrameBuffers(const VkImageView& depth_buffer){
    ResizeFrameBuffers();
    for(uint32_t i = 0;i<m_SwapChainFrameBuffers.size();++i){
        std::array<VkImageView,2> attachments = {
            m_SwapChainImages[i].imageView,
            depth_buffer
        };
        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.width = GetExtentWidth();
        frameBufferCreateInfo.height = GetExtentHeight();
        frameBufferCreateInfo.layers = 1;
        frameBufferCreateInfo.renderPass = *m_RenderPass;
        frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCreateInfo.pAttachments = attachments.data();
        if(vkCreateFramebuffer(m_MainDevice->logicalDevice,&frameBufferCreateInfo,nullptr,&m_SwapChainFrameBuffers[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to create frame buffer!");
        }
    }
}

SwapChainSupportDetails SwapChain::GetSwapChainDetails(VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface){
    SwapChainSupportDetails swapChainDetails;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,&swapChainDetails.capabilities);
    uint32_t formatCount= 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,surface, &formatCount,nullptr);
    if(formatCount>0){
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,&formatCount,swapChainDetails.formats.data());
    }

    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount,nullptr);
    if(presentationCount>0){
        swapChainDetails.presentModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentationCount,swapChainDetails.presentModes.data());
    }
    return swapChainDetails;
}

VkSurfaceFormatKHR SwapChain::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats){
    for (const auto &availableFormat : formats)
    {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return availableFormat;
      }
    }
    return formats[0];
}

VkPresentModeKHR SwapChain::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes){
    for(const auto& presentationMode: presentationModes){
        if(presentationMode == VK_PRESENT_MODE_MAILBOX_KHR){
            return presentationMode  ;
        }// triple buffering
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities){
    if(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max()){
        return surfaceCapabilities.currentExtent;
    }
    else{
        int width, height;
        glfwGetFramebufferSize(m_Window,&width,& height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        actualExtent.width = std::clamp(actualExtent.width,surfaceCapabilities.minImageExtent.width,surfaceCapabilities.maxImageExtent.height);
        actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height,surfaceCapabilities.maxImageExtent.height);
        return actualExtent;
    }
}


