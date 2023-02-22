/*
目前的内容是我想加GUI界面的时候发现自己的render不能获取render data,在实现getRenderData的函数的时候,发现有个队列索引的东西没有,然后现在是看见了
队列索引的东西是在创建设备的时候,调用了Utility.h里的一个函数初始化的,回头加这个. 其实之间command相关的地方已经用到了,但是我临时那么改了一下
绕了过去,现在又出现了这个问题。

目前是拆分swapchain handler遇到了一点困难,因为使用swapchain的函数里面使用了renderpass相关的东西,所以明天再实现完renderpass再接着搞。
***毁灭问题*** vulkan资源忘记释放的话,我怎么能知道是哪个未释放呢?
*/
#include "Common.h"
#include "Camera.h"
#include "Vertex.h"
#include "Mesh&Model.h"
#include "DebugMessanger.h"
#include "Utilities.h"
#include "DescriptorsHandler.h"
#include "GUI.h"
#include "SwapChainHandler.h"
#include "RenderPassHandler.h"
#include "GraphicsPipeline.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
int textureNum = 0;
std::unordered_map<std::string, Texture> textures;
Camera camera;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct UniformBufferObject
{
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 cameraPos;
};

float lastX = 300, lastY = 300;
bool firstCall = true;
void mouseCallback(GLFWwindow* window, double xPos,double yPos)
{
  std::cout<<"xPos: "<< xPos<<"   yPos: "<< yPos<<"\r";
  if(firstCall){
    lastX = xPos;
    lastY = yPos;
    firstCall = false;
    return;
  }
  float sensitive = 0.035;
  float yDif = yPos-lastY;
  float xDif = xPos-lastX;
  camera.pitch +=sensitive*yDif;
  camera.pitch = glm::clamp(camera.pitch,-89.0f,89.0f);
  camera.yaw +=sensitive*xDif;
  lastX = xPos;
  lastY = yPos;
}

void keycallback(GLFWwindow* window){
  float sensitivity = 0.007;
  if(glfwGetKey(window,GLFW_KEY_ESCAPE) == GLFW_PRESS){
    glfwSetWindowShouldClose(window, true);
  }
  if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS){
    camera.pos +=sensitivity*camera.direction;
  }
  if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS){
    camera.pos -=sensitivity*camera.direction;
  }
  if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS){
    camera.pos -=sensitivity*glm::normalize(glm::cross(camera.direction,camera.up));
  }
  if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS){
    camera.pos +=sensitivity*glm::normalize(glm::cross(camera.direction,camera.up));
  }
  if(glfwGetKey(window,GLFW_KEY_SPACE) == GLFW_PRESS){
    camera.pos +=sensitivity*glm::vec3(0,1,0);
  }
  if(glfwGetKey(window,GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
    camera.pos -=sensitivity*glm::vec3(0,1,0);
  }
}

class VulkanApp
{
public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  friend class Mesh;
  // this window should be a window class
  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR m_surface;
  MainDevice m_device;

  //------------------queueFamily etc.-------------------------
  //QueueFamilyIndices m_queueFamily_indices;// inited in device create process.
  QueueFamilyIndices m_QueueFamilyIndices;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  Descriptors m_descriptor;

//-----------swapchain_Handler--------------------------
  SwapChain m_swapChain;

//------------renderPass_handler----------------
  RenderPassHandler m_renderPassHandler;
// ---------------pipeline_handler---------------
  GraphicPipeline m_pipelineHandler;
//---------------command_handler------------------
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
// --------------pushConstant------------------
  VkPushConstantRange m_pushConstant;

  VkSampler textureSampler;
  Texture depthImage;
  std::vector<Model> scene;

  std::vector<Buffer> uniformBuffers;
  std::vector<void *> uniformBuffersMapped;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  VmaAllocator allocator;

  uint32_t currentFrame = 0;

  bool framebufferResized = false;

  void initWindow()
  {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
  {
    auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  void initVulkan()
  {
    Utility::Setup(&m_device, &m_surface, &commandPool, &graphicsQueue, &allocator);
    m_descriptor = Descriptors::Descriptors(&m_device.logicalDevice);
    m_swapChain = SwapChain::SwapChain(&m_device, &m_surface, window, m_QueueFamilyIndices);
    m_pipelineHandler = GraphicPipeline::GraphicPipeline(&m_device,&m_swapChain, &m_renderPassHandler);
    initPushConstant();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createVmaAllocator();
    m_swapChain.CreateSwapChain();
    m_renderPassHandler = RenderPassHandler::RenderPassHandler(&m_device,&m_swapChain);
    m_renderPassHandler.CreateRenderPass();
    createCommandPool();
//--------------------Load models-----------------------
    loadComplexModel("./models/nanosuit/nanosuit.obj",glm::vec3(0.0f,1.0f,0.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.2f,0.2f,0.2f));
    //loadComplexModel("./models/sponza/sponza.obj",glm::vec3(3.0f,1.0f,0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.02f,0.02f,0.02f));
    loadComplexModel("./models/duck/12248_Bird_v1_L2.obj",glm::vec3(0.0f,8.0f,3.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.002f,0.002f,0.002f));
    textureNum = textures.size();
//------------------------------------------------------
    m_descriptor.CreateSetLayouts();
    m_pipelineHandler.SetDescriptorSetLayouts(m_descriptor.GetViewProjectionSetLayout(),m_descriptor.GetTextureSetLayout(), m_descriptor.GetInputSetLayout(), m_descriptor.GetLightSetLayout(),m_descriptor.GetSettingsSetLayout());
    m_pipelineHandler.CreateGraphicPipeline();
    createDepthResources();
    m_swapChain.SetRenderPass(&m_renderPassHandler.GetRenderPass());
    m_swapChain.CreateFrameBuffers(depthImage.textureImageView);
    createTextureSampler();
    createUniformBuffers();
    m_descriptor.CreateDescriptorPools(3,3,3,3);
    for(auto& i: textures){
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = m_descriptor.GetTexturePool();
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &m_descriptor.GetTextureSetLayout();
      if(vkAllocateDescriptorSets(m_device.logicalDevice, &allocInfo,&i.second.textureDescriptor)!=VK_SUCCESS){
        throw std::runtime_error("failed to create texture descriptor set!");
      }

      VkDescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = i.second.textureImageView;
      imageInfo.sampler = textureSampler;

      VkWriteDescriptorSet descriptorWrite{};
      descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrite.dstSet = i.second.textureDescriptor;
      descriptorWrite.dstBinding = 0;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(m_device.logicalDevice, 1, &descriptorWrite, 0, nullptr); 
    }
    m_descriptor.CreateViewProjectionDescriptorSets(uniformBuffers,sizeof(UniformBufferObject),2);
    createCommandBuffers();
    createSyncObjects();
    createCamera();
  }

  void mainLoop()
  {
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    //----------------------GUI-----------------------
    	float lights_pos	= 0.0f;
      float lights_speed	= 0.0001f;
      // Light Colours
      glm::vec3 light_col(1.0f);
      int light_idx = 0;

      //GUI::getInstance()->SetRenderData()

    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();
      keycallback(window);
      drawFrame();
    }
    vkDeviceWaitIdle(m_device.logicalDevice);
  }

  void cleanup()
  {
    m_swapChain.CleanUpSwapChain();
    depthImage.destroy(m_device.logicalDevice, allocator);
    m_pipelineHandler.DestroyPipeline();
    m_renderPassHandler.DestroyRenderPass();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vmaUnmapMemory(allocator, uniformBuffers[i].allocation);
      uniformBuffers[i].destroy(m_device.logicalDevice, allocator);
    }

    vkDestroySampler(m_device.logicalDevice, textureSampler, nullptr);
    m_descriptor.Destroy();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vkDestroySemaphore(m_device.logicalDevice, renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(m_device.logicalDevice, imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(m_device.logicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_device.logicalDevice, commandPool, nullptr);
    clearScene(scene);
    for(auto & tex:textures){
      tex.second.destroy(m_device.logicalDevice, allocator);
    }
    vmaDestroyAllocator(allocator);

    vkDestroyDevice(m_device.logicalDevice, nullptr);

    if (enableValidationLayers)
    {
      DebugMessanger::GetInstance()->Clear();
    }
    vkDestroySurfaceKHR(instance, m_surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
  }

  void recreateSwapChain()
  {
    m_swapChain.ReCreateSwapChain();
    createDepthResources();
    m_swapChain.CreateFrameBuffers(depthImage.textureImageView);
  }

  void createInstance()
  {
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
      throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
      createInfo.enabledLayerCount = 0;

      createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create instance!");
    }
    if(enableValidationLayers){
      DebugMessanger::GetInstance()->SetupDebugMessenger(instance);
    }
  }

  void createSurface()
  {
    if (glfwCreateWindowSurface(instance, window, nullptr, &m_surface) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create window surface!");
    }
  }

  void pickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device : devices)
    {
      if (isDeviceSuitable(device))
      {
        m_device.physicalDevice = device;
        break;
      }
    }

    if (m_device.physicalDevice == VK_NULL_HANDLE)
    {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }

  void createLogicalDevice()
  {
    Utility::findQueueFamilies(m_device.physicalDevice, m_QueueFamilyIndices);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_device.physicalDevice, &createInfo, nullptr, &m_device.logicalDevice) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_device.logicalDevice, m_QueueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(m_device.logicalDevice, m_QueueFamilyIndices.presentFamily.value(), 0, &presentQueue);
  }

  void createCommandPool()
  {
    Utility::findQueueFamilies(m_device.physicalDevice, m_QueueFamilyIndices);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(m_device.logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create graphics command pool!");
    }
  }

  void createTextureImage(std::string texPath,Texture& texture)
  {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(texPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth,texHeight))))+1;  //这里是计算最多多少层mipmap
    if (!pixels)
    {
      throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer;
    Utility::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    stbi_image_free(pixels);

    Utility::createImage(texWidth, texHeight, miplevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);

    transitionImageLayout(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,miplevels);
    Utility::CopyBufferToImage(stagingBuffer.buffer, texture.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(texture.textureImage,VK_FORMAT_R8G8B8A8_SRGB,texWidth,texHeight,miplevels);

    stagingBuffer.destroy(m_device.logicalDevice,allocator);
  }

  void generateMipmaps(VkImage image, VkFormat imageFormat , uint32_t texWidth, uint32_t texHeight, uint32_t miplevels){
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(m_device.physicalDevice,imageFormat,&properties);
    if(!(properties.optimalTilingFeatures& VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)){
      throw std::runtime_error("texture images format does not support linear bliting");
    }
    VkCommandBuffer commandbuffer = Utility::beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for(uint32_t i = 1;i<miplevels;++i)
    {
      barrier.subresourceRange.baseMipLevel = i-1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(commandbuffer,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_PIPELINE_STAGE_2_TRANSFER_BIT, 0,
            0,nullptr,
            0,nullptr,
            1,&barrier);//相当于使用了这个屏障

      VkImageBlit blit{};
      blit.srcOffsets[0] = {0,0,0};
      blit.srcOffsets[1] = {mipWidth,mipHeight,1};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = i-1;
      blit.srcSubresource.layerCount  = 1;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.dstOffsets[0] = {0,0,0};
      blit.dstOffsets[1] = {mipWidth>1?mipWidth/2:1,mipHeight>1?mipHeight/2:1,1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = i;
      blit.dstSubresource.layerCount = 1;
      blit.dstSubresource.baseArrayLayer = 0;
      vkCmdBlitImage(commandbuffer,
              image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              1,&blit,
              VK_FILTER_LINEAR);
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(commandbuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
      if(mipWidth>1) mipWidth/=2;
      if(mipHeight>1) mipHeight /=2; 
    }
    barrier.subresourceRange.baseMipLevel = miplevels-1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandbuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,0,nullptr,0,nullptr,1,&barrier);
    Utility::endSingleTimeCommands(commandbuffer);
  }

  void createTextureImageView(Texture& textureImage,int miplevels)
  {
    textureImage.textureImageView = Utility::createImageView(textureImage.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, miplevels);
  }


  void createTextureSampler()
  {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_device.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(miplevels);
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_device.logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture sampler!");
    }
  }

  void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,uint32_t miplevels)
  {
    VkCommandBuffer commandBuffer = Utility::beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = miplevels ;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    Utility::endSingleTimeCommands(commandBuffer);
  }

  void createVertexBuffer(std::vector<Vertex>& vertices, Buffer& vertexBuffer)
  {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    Buffer stagingBuffer;
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation,&data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);

    Utility::CopyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);

    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  }

  void createIndexBuffer(std::vector<uint32_t> indices, Buffer& indexBuffer)
  {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    Buffer stagingBuffer;
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);

    Utility::CopyBuffer(stagingBuffer.buffer, indexBuffer.buffer, bufferSize);

    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  }

  void createUniformBuffers()
  {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i]);
      vmaMapMemory(allocator,uniformBuffers[i].allocation,&uniformBuffersMapped[i]);
    }
  }

  void createCommandBuffers()
  {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to allocate command buffers!");
    }
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPassHandler.GetRenderPass();
    renderPassInfo.framebuffer = m_swapChain.GetFrameBuffer(imageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain.GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineHandler.getPipline());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_swapChain.GetExtent().width;
    viewport.height = (float) m_swapChain.GetExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChain.GetExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    drawScene(scene,commandBuffer, m_descriptor.GetDescriptorSets()[currentFrame],m_pipelineHandler.getLayout());

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
  }

  void createSyncObjects()
  {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      if (vkCreateSemaphore(m_device.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(m_device.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(m_device.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
      }
    }
  }

  void updateUniformBuffer(uint32_t currentImage)
  {
    UniformBufferObject ubo{};
    ubo.view = camera.getViewMatrix(true);
    ubo.proj = camera.getProjectMatrix(false);
    ubo.proj[1][1] *= -1;
    ubo.cameraPos = camera.pos;
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
  }

  clock_t t1, t2;
  void drawFrame()
  {
    t2 = clock();
    float dt = (double)(t2 - t1) / CLOCKS_PER_SEC;//计时粒度为1000毫秒
    float fps = 1.0 / dt;
    std::cout << "\r";
    std::string title = "Vulkan  " + std::to_string(fps);
    glfwSetWindowTitle(window, title.c_str());
    t1 = t2;//别忘了更新计时器

    vkWaitForFences(m_device.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device.logicalDevice, m_swapChain.GetSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      recreateSwapChain();
      return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_device.logicalDevice, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain.GetSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
      framebufferResized = false;
      recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  VkShaderModule createShaderModule(const std::vector<char> &code)
  {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
  {
    for (const auto &availableFormat : availableFormats)
    {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
  {
    for (const auto &availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return availablePresentMode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
    else
    {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = {
          static_cast<uint32_t>(width),
          static_cast<uint32_t>(height)};

      actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
      return actualExtent;
    }
  }

  bool isDeviceSuitable(VkPhysicalDevice device)
  {
    Utility::findQueueFamilies(device, m_QueueFamilyIndices);
    bool extensionsSupported = Utility::checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
      SwapChainSupportDetails swapChainSupport = m_swapChain.GetSwapChainDetails(device, m_surface);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    return m_QueueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
  }

  std::vector<const char *> getRequiredExtensions()
  {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers)
    {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
  }

  bool checkValidationLayerSupport()
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers)
    {
      bool layerFound = false;

      for (const auto &layerProperties : availableLayers)
      {
        if (strcmp(layerName, layerProperties.layerName) == 0)
        {
          layerFound = true;
          break;
        }
      }

      if (!layerFound)
      {
        return false;
      }
    }
    return true;
  }

  void createDepthResources()
  {
    VkFormat depthFormat = findDepthFormat();
    Utility::createImage(m_swapChain.GetExtent().width, m_swapChain.GetExtent().height, 1,depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage);
    depthImage.textureImageView = Utility::createImageView(depthImage.textureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
  }

  VkFormat findDepthFormat()
  {
    return Utility::findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT},
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  bool hasStencilComponent(VkFormat format)
  {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || VK_FORMAT_D24_UNORM_S8_UINT;
  }

  glm::mat4 getTranslation(glm::vec3& translate, glm::vec3& rotate, glm::vec3& scale){

    glm::mat4 unit(    // 单位矩阵
            glm::vec4(1, 0, 0, 0),
            glm::vec4(0, 1, 0, 0),
            glm::vec4(0, 0, 1, 0),
            glm::vec4(0, 0, 0, 1)
        );
    glm::mat4 rotateMat;
    rotateMat = glm::rotate(unit, glm::radians(rotate.x), glm::vec3(1,0,0));
    rotateMat = glm::rotate(rotateMat, glm::radians(rotate.y), glm::vec3(0,1,0));
    rotateMat = glm::rotate(rotateMat, glm::radians(rotate.z), glm::vec3(0,0,1));
    glm::mat4 transMat = glm::translate(unit,translate);
    glm::mat4 scaleMat = glm::scale(unit, scale);
    return transMat*rotateMat*scaleMat;
  }

  // In the future, we can stage model path , translate, rotate, scale etc. in json file.
  void loadComplexModel(const char* file_path, glm::vec3& translate, glm::vec3& rotate, glm::vec3& scale){
    std::string model_path = file_path;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path=model_path.substr(0,model_path.find_last_of("/\\"))+"/";
    tinyobj::ObjReader reader;
    if(!reader.ParseFromFile(model_path,reader_config)){
      if(!reader.Error().empty()){
        std::cout<<"TinyObjReader";
        throw std::runtime_error(reader.Error());
      }
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    Model model;
    model.model = getTranslation(translate, rotate, scale);

    for(size_t s = 0;s<shapes.size();++s)
    {
      Mesh mesh;
      size_t index_offset = 0;
      for(size_t f = 0;f<shapes[s].mesh.num_face_vertices.size();++f){
        size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
        for(size_t v = 0;v<fv;++v){
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset+v];
          mesh.indices.push_back(static_cast<uint32_t>(mesh.indices.size()));
          Vertex vertex;
          vertex.pos.x = attrib.vertices[3*size_t(idx.vertex_index)+0];
          vertex.pos.y = attrib.vertices[3*size_t(idx.vertex_index)+1];
          vertex.pos.z = attrib.vertices[3*size_t(idx.vertex_index)+2];

          if(idx.normal_index>=0){//没有的话是-1
            vertex.normal.x = attrib.normals[3*size_t(idx.normal_index)+0];
            vertex.normal.y = attrib.normals[3*size_t(idx.normal_index)+1];
            vertex.normal.z = attrib.normals[3*size_t(idx.normal_index)+2];
          }

          if(idx.texcoord_index>=0){
            vertex.texCoord.x = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
            vertex.texCoord.y = -attrib.texcoords[2*size_t(idx.texcoord_index)+1];
          }
          mesh.vertices.push_back(vertex);
        }
        index_offset+=fv;
      }
      createVertexBuffer(mesh.vertices,mesh.vertexBuffer);
      createIndexBuffer(mesh.indices,mesh.indexBuffer);
      // Read this mesh's texture
      std::string texturePath = reader_config.mtl_search_path+materials[shapes[s].mesh.material_ids[0]].diffuse_texname;
      if(textures.find(texturePath) == textures.end()) {
        Texture tempTexture;
        createTextureImage(texturePath, tempTexture);
        createTextureImageView(tempTexture,miplevels);
        textures[texturePath] = tempTexture;
      }
      mesh.textureIndex = texturePath;
      model.meshes.push_back(mesh);
    }
    scene.push_back(model);
  }

  void drawScene(std::vector<Model>& scene, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& layout) {
    for(auto & model: scene){
      updateUniformBuffer(currentFrame);
      model.draw(m_device.logicalDevice, commandBuffer,descriptorSet,layout, textureSampler);
    }
  }
  void createCamera(){

    camera.aspect = m_swapChain.GetExtent().width
     / (float)m_swapChain.GetExtent().height;
  }

  void clearScene(std::vector<Model>& scene){
    for(auto& model:scene){
      for(auto& mesh :model.meshes){
        mesh.vertexBuffer.destroy(m_device.logicalDevice,allocator);
        mesh.indexBuffer.destroy(m_device.logicalDevice,allocator);
      }
    }  
  }

  void createVmaAllocator(){
    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.device = m_device.logicalDevice;
    allocInfo.instance = instance;
    allocInfo.physicalDevice = m_device.physicalDevice;

    if(vmaCreateAllocator(&allocInfo,&allocator)!=VK_SUCCESS){
      throw std::runtime_error("failed to create vulkan memory allocator!");
    }
  }

  const VulkanRenderData GetRenderData(){
    VulkanRenderData data = {};
    data.main_device = m_device;
    data.instance = instance;
    data.graphic_queue_index = m_QueueFamilyIndices.graphicsFamily.value();
    data.graphic_queue = graphicsQueue;
    data.imgui_descriptor_pool = m_descriptor.GetImguiDescriptorPool();
    data.min_image_count = 3;
    data.image_count = 3;
    // data.render_pass = 
    // data.command_pool = ;
    // data.command_buffer = ;
    data.texture_descriptor_layout = m_descriptor.GetTextureSetLayout();
    data.texture_descriptor_pool = m_descriptor.GetTexturePool();
    return data;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  void initPushConstant(){
    m_pushConstant.offset = 0;
    m_pushConstant.size = sizeof(constentData);
    m_pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    m_pipelineHandler.SetPushConstantRange(m_pushConstant);
  }
};

int main()
{
  VulkanApp app;
  try
  {
    app.run();
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}