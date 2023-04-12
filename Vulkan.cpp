/*
***毁灭问题*** vulkan资源忘记释放的话,我怎么能知道是哪个未释放呢?
remember we have not implement the definition of the second pipeline creation.
for this project, we have to set a offScreen renderPass, and a present
renderPass if I want to implement the PBR, there will be many kinds of material,
may be I need an addition renderTarget, and stage different material properties
in different channel
*/
#include "Camera.h"
#include "CommandHandler.h"
#include "Common.h"
#include "DebugMessanger.h"
#include "DescriptorsHandler.h"
#include "GUI.h"
#include "GraphicsPipeline.h"
#include "Light.h"
#include "Mesh&Model.h"
#include "RenderPassHandler.h"
#include "SwapChainHandler.h"
#include "Utilities.h"
#include "Vertex.h"

constexpr std::size_t NUM_LIGHTS = 20;
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
std::unordered_map<std::string, Texture> textures;
Camera camera;
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif



struct UniformBufferObject {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::vec3 cameraPos;
};

float lastX = 300, lastY = 300;
bool click = true;

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  if (action == GLFW_PRESS)
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      click = false;
      break;
    }
  if (action == GLFW_RELEASE)
    click = true;
}
void mouseCallback(GLFWwindow *window, double xPos, double yPos) {
  std::cout << "xPos: " << xPos << "   yPos: " << yPos << "\r";
  if (click) {
    lastX = xPos;
    lastY = yPos;
  }
  float sensitive = 0.035;
  float yDif = yPos - lastY;
  float xDif = xPos - lastX;
  camera.pitch += sensitive * yDif;
  camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);
  camera.yaw += sensitive * xDif;
  lastX = xPos;
  lastY = yPos;
}

void keycallback(GLFWwindow *window) {
  float sensitivity = 0.007;
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera.pos += sensitivity * camera.direction;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera.pos -= sensitivity * camera.direction;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera.pos -=
        sensitivity * glm::normalize(glm::cross(camera.direction, camera.up));
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera.pos +=
        sensitivity * glm::normalize(glm::cross(camera.direction, camera.up));
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    camera.pos += sensitivity * glm::vec3(0, 1, 0);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    camera.pos -= sensitivity * glm::vec3(0, 1, 0);
  }
}

class VulkanApp {
public:
  void run() {
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
  // QueueFamilyIndices m_queueFamily_indices;// inited in device create
  // process.
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
  CommandHandler m_CommandHandler;
  CommandHandler m_OffScreenCommandHandler;
  // --------------pushConstant------------------
  VkPushConstantRange m_pushConstant;

  SettingsData m_SettingsData;

  VkSampler textureSampler;
  std::vector<Model> scene;

  std::vector<Texture> m_PositionBufferImages;
  std::vector<Texture> m_ColorBufferImages;
  std::vector<Texture> m_NormalBufferImages;
  std::vector<Texture> m_MaterialBufferImages;
  Texture depthImage;

  std::vector<VkFramebuffer> m_OffScreenFrameBuffer;

  std::vector<Buffer> m_ViewProjectUBO;

  std::vector<Buffer> m_LightUBO;
  glm::vec3 light_col = glm::vec3(1.0f);
  std::array<LightData, NUM_LIGHTS> m_LightData;

  std::vector<Buffer> m_SettingsUBO;

  std::vector<SubmissionSyncObjects> m_SyncObjects;
  VmaAllocator allocator;

  uint32_t currentFrame = 0;

  bool framebufferResized = false;

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
  }

  void initVulkan() {
    Utility::Setup(&m_device, &m_surface, &m_CommandHandler.GetCommandPool(),
                   &graphicsQueue, &allocator);
    m_descriptor = Descriptors(&m_device.logicalDevice);
    m_swapChain = SwapChain(&m_device, &m_surface, window,
                                       m_QueueFamilyIndices);
    m_pipelineHandler = GraphicPipeline(
        &m_device, &m_swapChain, &m_renderPassHandler);
    m_CommandHandler = CommandHandler(
        &m_device, &m_pipelineHandler, &m_renderPassHandler);
    m_OffScreenCommandHandler = CommandHandler(&m_device,&m_pipelineHandler,&m_renderPassHandler);
        
    initPushConstant();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createVmaAllocator();
    m_swapChain.CreateSwapChain();
    m_renderPassHandler = RenderPassHandler(&m_device, &m_swapChain);
    m_renderPassHandler.CreateRenderPass();
    m_renderPassHandler.CreateOffScreenRenderPass();
    m_CommandHandler.CreateCommandPool(m_QueueFamilyIndices);
    //--------------------Load models-----------------------
    loadComplexModel("./models/nanosuit/nanosuit.obj",
                     glm::vec3(0.0f, 1.0f, -30.0f),
                     glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.2f, 0.2f, 0.2f));
    loadComplexModel("./models/sponza/sponza.obj",
                     glm::vec3(3.0f, 1.0f, -30.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.02f, 0.02f, 0.02f));
    loadComplexModel(
        "./models/duck/12248_Bird_v1_L2.obj", glm::vec3(0.0f, 8.0f, -33.0f),
        glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.002f, 0.002f, 0.002f));
    // loadComplexModel("./models/the_twins__atomic_heart/scene.gltf",glm::vec3(5.0f,4.0f,0.0f),
    // glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.02f,0.02f,0.02f));
    //------------------------------------------------------
    m_descriptor.CreateSetLayouts();
    m_pipelineHandler.SetDescriptorSetLayouts(
        m_descriptor.GetViewProjectionSetLayout(),
        m_descriptor.GetTextureSetLayout(), m_descriptor.GetInputSetLayout(),
        m_descriptor.GetLightSetLayout(), m_descriptor.GetSettingsSetLayout());
    m_pipelineHandler.CreateGraphicPipeline();

    // Creation of the offscreen buffer images
    m_PositionBufferImages.resize(m_swapChain.SwapChainImagesSize());
    m_NormalBufferImages.resize(m_swapChain.SwapChainImagesSize());
    m_ColorBufferImages.resize(m_swapChain.SwapChainImagesSize());

    for (int i = 0; i < m_ColorBufferImages.size(); ++i) {
      Utility::CreatePositionBufferImage(m_PositionBufferImages[i],
                                         m_swapChain.GetExtent());
      Utility::CreatePositionBufferImage(m_ColorBufferImages[i],
                                         m_swapChain.GetExtent());
      Utility::CreatePositionBufferImage(m_NormalBufferImages[i],
                                         m_swapChain.GetExtent());
    }
    createDepthResources();
    m_swapChain.SetRenderPass(m_renderPassHandler.GetRenderPassReference());
    m_swapChain.CreateFrameBuffers(depthImage.textureImageView);
    CreateOffScreenFrameBuffer();

    m_OffScreenCommandHandler.CreateCommandPool(m_QueueFamilyIndices);
    m_OffScreenCommandHandler.CreateCommandBuffers(
        m_swapChain.FrameBufferSize());
    createTextureSampler();
    createUniformBuffers();
    m_descriptor.CreateDescriptorPools(m_swapChain.SwapChainImagesSize(), 3, 3,
                                       3);
    for (auto &i : textures) {
      VkDescriptorSetAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocInfo.descriptorPool = m_descriptor.GetTexturePool();
      allocInfo.descriptorSetCount = 1;
      allocInfo.pSetLayouts = &m_descriptor.GetTextureSetLayout();
      if (vkAllocateDescriptorSets(m_device.logicalDevice, &allocInfo,
                                   &i.second.textureDescriptor) != VK_SUCCESS) {
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
      descriptorWrite.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo = &imageInfo;

      vkUpdateDescriptorSets(m_device.logicalDevice, 1, &descriptorWrite, 0,
                             nullptr);
    }
    m_descriptor.CreateViewProjectionDescriptorSets(
        m_ViewProjectUBO, sizeof(UniformBufferObject),
        m_swapChain.SwapChainImagesSize());
    m_descriptor.CreateInputAttachmentsDescriptorSets(
        m_swapChain.SwapChainImagesSize(), m_PositionBufferImages,
        m_ColorBufferImages, m_NormalBufferImages);
    m_descriptor.CreateLightDescriptorSets(m_LightUBO, sizeof(LightData),
                                           m_swapChain.SwapChainImagesSize());
    m_descriptor.CreateSettingsDescriptorSets(
        m_SettingsUBO, sizeof(SettingsData), m_swapChain.SwapChainImagesSize());
    m_CommandHandler.CreateCommandBuffers(m_swapChain.FrameBufferSize());
    SetUniformDataStructures();
    createSyncObjects();
    createCamera();
  }

  void mainLoop() {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    //----------------------GUI-----------------------
    float lights_pos = 0.0f;
    float lights_speed = 0.0001f;
    // Light Colours
    int light_idx = 0;
    GUI::getInstance()->SetRenderData(GetRenderData(), window, &m_SettingsData,
                                      &lights_speed, &light_idx, &light_col);
    GUI::getInstance()->Init();
    GUI::getInstance()->LoadFontsToGPU();
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      keycallback(window);
      GUI::getInstance()->Render();
      auto draw_data = GUI::getInstance()->GetDrawData();
      const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f ||
                                 draw_data->DisplaySize.y <= 0.0f);
      if (!is_minimized)
        drawFrame(draw_data);
      UpdateLightColor(light_idx, light_col);
    }
    vkDeviceWaitIdle(m_device.logicalDevice);
  }

  void cleanup() {
    m_swapChain.CleanUpSwapChain();
    depthImage.destroy(m_device.logicalDevice, allocator);
    m_pipelineHandler.DestroyPipeline();
    m_renderPassHandler.DestroyRenderPass();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      m_ViewProjectUBO[i].destroy(m_device.logicalDevice, allocator);
      m_PositionBufferImages[i].destroy(m_device.logicalDevice, allocator);
      m_NormalBufferImages[i].destroy(m_device.logicalDevice, allocator);
      m_ColorBufferImages[i].destroy(m_device.logicalDevice, allocator);
      vkDestroyFramebuffer(m_device.logicalDevice, m_OffScreenFrameBuffer[i],
                           nullptr);
    }

    for (auto &i : m_LightUBO) {
      i.destroy(m_device.logicalDevice, allocator);
    }
    for (auto &i : m_SettingsUBO) {
      i.destroy(m_device.logicalDevice, allocator);
    }

    vkDestroySampler(m_device.logicalDevice, textureSampler, nullptr);
    m_descriptor.Destroy();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(m_device.logicalDevice,
                         m_SyncObjects[i].RenderFinished, nullptr);
      vkDestroySemaphore(m_device.logicalDevice,
                         m_SyncObjects[i].ImageAvailable, nullptr);
      vkDestroySemaphore(m_device.logicalDevice,
                         m_SyncObjects[i].OffScreenAvaliable, nullptr);
      vkDestroyFence(m_device.logicalDevice, m_SyncObjects[i].InFlight,
                     nullptr);
    }
    GUI::getInstance()->Destroy();
    m_CommandHandler.FreeCommandBuffers();
    m_CommandHandler.DestroyCommandPool();
    m_OffScreenCommandHandler.FreeCommandBuffers();
    m_OffScreenCommandHandler.DestroyCommandPool();
    // vkDestroyCommandPool(m_device.logicalDevice, commandPool, nullptr);
    clearScene(scene);
    for (auto &tex : textures) {
      tex.second.destroy(m_device.logicalDevice, allocator);
    }
    vmaDestroyAllocator(allocator);

    vkDestroyDevice(m_device.logicalDevice, nullptr);

    if (enableValidationLayers) {
      DebugMessanger::GetInstance()->Clear();
    }
    vkDestroySurfaceKHR(instance, m_surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
  }

  void recreateSwapChain() {
    m_swapChain.SetRecreationStatus(true);
    m_swapChain.ReCreateSwapChain();
    depthImage.destroy(m_device.logicalDevice, allocator);
    createDepthResources();
    m_swapChain.CreateFrameBuffers(depthImage.textureImageView);
  }

  void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
      throw std::runtime_error(
          "validation layers requested, but not available!");
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
    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
      createInfo.enabledLayerCount = 0;

      createInfo.pNext = nullptr;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
      throw std::runtime_error("failed to create instance!");
    }
    if (enableValidationLayers) {
      DebugMessanger::GetInstance()->SetupDebugMessenger(instance);
    }
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &m_surface) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
    }
  }

  void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
      if (isDeviceSuitable(device)) {
        m_device.physicalDevice = device;
        break;
      }
    }

    if (m_device.physicalDevice == VK_NULL_HANDLE) {
      throw std::runtime_error("failed to find a suitable GPU!");
    }
  }

  void createLogicalDevice() {
    Utility::findQueueFamilies(m_device.physicalDevice, m_QueueFamilyIndices);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_QueueFamilyIndices.graphicsFamily.value(),
        m_QueueFamilyIndices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
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

    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
      createInfo.enabledLayerCount = 0;
    }
    if (vkCreateDevice(m_device.physicalDevice, &createInfo, nullptr,
                       &m_device.logicalDevice) != VK_SUCCESS) {
      throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_device.logicalDevice,
                     m_QueueFamilyIndices.graphicsFamily.value(), 0,
                     &graphicsQueue);
    vkGetDeviceQueue(m_device.logicalDevice,
                     m_QueueFamilyIndices.presentFamily.value(), 0,
                     &presentQueue);
  }

  void createTextureImage(std::string texPath, Texture &texture) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(texPath.c_str(), &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    miplevels = static_cast<uint32_t>(
                    std::floor(std::log2(std::max(texWidth, texHeight)))) +
                1; // 这里是计算最多多少层mipmap
    if (!pixels) {
      throw std::runtime_error("failed to load texture image!");
    }

    Buffer stagingBuffer;
    Utility::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);
    stbi_image_free(pixels);
    Utility::createImage(texWidth, texHeight, miplevels,
                         VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                             VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture);
    transitionImageLayout(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, miplevels);
    Utility::CopyBufferToImage(stagingBuffer.buffer, texture.textureImage,
                               static_cast<uint32_t>(texWidth),
                               static_cast<uint32_t>(texHeight));
    generateMipmaps(texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth,
                    texHeight, miplevels);
    stagingBuffer.destroy(m_device.logicalDevice, allocator);
  }

  void generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t texWidth,
                       uint32_t texHeight, uint32_t miplevels) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(m_device.physicalDevice, imageFormat,
                                        &properties);
    if (!(properties.optimalTilingFeatures &
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
      throw std::runtime_error(
          "texture images format does not support linear bliting");
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
    for (uint32_t i = 1; i < miplevels; ++i) {
      barrier.subresourceRange.baseMipLevel = i - 1;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_2_TRANSFER_BIT, 0, 0, nullptr, 0,
                           nullptr, 1, &barrier); // 相当于使用了这个屏障

      VkImageBlit blit{};
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = i - 1;
      blit.srcSubresource.layerCount = 1;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.dstOffsets[0] = {0, 0, 0};
      blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                            mipHeight > 1 ? mipHeight / 2 : 1, 1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = i;
      blit.dstSubresource.layerCount = 1;
      blit.dstSubresource.baseArrayLayer = 0;
      vkCmdBlitImage(commandbuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                     VK_FILTER_LINEAR);
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &barrier);
      if (mipWidth > 1)
        mipWidth /= 2;
      if (mipHeight > 1)
        mipHeight /= 2;
    }
    barrier.subresourceRange.baseMipLevel = miplevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);
    Utility::endSingleTimeCommands(commandbuffer);
  }

  void createTextureImageView(Texture &textureImage, int miplevels) {
    textureImage.textureImageView = Utility::createImageView(
        textureImage.textureImage, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT, miplevels);
  }

  void createTextureSampler() {
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

    if (vkCreateSampler(m_device.logicalDevice, &samplerInfo, nullptr,
                        &textureSampler) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
  }

  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t miplevels) {
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
    barrier.subresourceRange.levelCount = miplevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    Utility::endSingleTimeCommands(commandBuffer);
  }

  void createUniformBuffers() {
    m_ViewProjectUBO.resize(m_swapChain.SwapChainImagesSize());
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    for (size_t i = 0; i < m_swapChain.SwapChainImagesSize(); ++i) {
      Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            m_ViewProjectUBO[i]);
    }

    bufferSize = NUM_LIGHTS * sizeof(LightData);
    m_LightUBO.resize(m_swapChain.SwapChainImagesSize());
    for (size_t i = 0; i < m_swapChain.SwapChainImagesSize(); ++i) {
      Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            m_LightUBO[i]);
    }

    bufferSize = sizeof(SettingsData);
    m_SettingsUBO.resize(m_swapChain.SwapChainImagesSize());
    for (size_t i = 0; i < m_swapChain.SwapChainImagesSize(); ++i) {
      Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            m_SettingsUBO[i]);
    }
  }

  void createSyncObjects() {
    m_SyncObjects.resize(MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateinfo = {};
    fenceCreateinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      VkResult offscreen_avaliable_sem =
          vkCreateSemaphore(m_device.logicalDevice, &semaphoreCreateInfo,
                            nullptr, &m_SyncObjects[i].OffScreenAvaliable);
      VkResult image_available_sem =
          vkCreateSemaphore(m_device.logicalDevice, &semaphoreCreateInfo,
                            nullptr, &m_SyncObjects[i].ImageAvailable);
      VkResult render_finished_sem =
          vkCreateSemaphore(m_device.logicalDevice, &semaphoreCreateInfo,
                            nullptr, &m_SyncObjects[i].RenderFinished);
      VkResult in_flight_fence =
          vkCreateFence(m_device.logicalDevice, &fenceCreateinfo, nullptr,
                        &m_SyncObjects[i].InFlight);

      if (offscreen_avaliable_sem != VK_SUCCESS ||
          image_available_sem != VK_SUCCESS ||
          render_finished_sem != VK_SUCCESS || in_flight_fence != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores and/or Fence!");
      }
    }
  }

  void updateUniformBuffer(uint32_t currentImage) {
    void *vp_data;
    UniformBufferObject ubo{};
    ubo.view = camera.getViewMatrix(true);
    ubo.proj = camera.getProjectMatrix(false);
    ubo.proj[1][1] *= -1;
    ubo.cameraPos = camera.pos;
    vmaMapMemory(allocator, m_ViewProjectUBO[currentImage].allocation,
                 &vp_data);
    memcpy(vp_data, &ubo, sizeof(ubo));
    vmaUnmapMemory(allocator, m_ViewProjectUBO[currentImage].allocation);

    void *light_data;
    auto light_data_size = m_LightData.size() * sizeof(LightData);
    vmaMapMemory(allocator, m_LightUBO[currentImage].allocation, &light_data);
    memcpy(light_data, m_LightData.data(), light_data_size);
    vmaUnmapMemory(allocator, m_LightUBO[currentImage].allocation);

    void *settings_data;
    auto settings_data_size = sizeof(SettingsData);
    vmaMapMemory(allocator, m_SettingsUBO[currentImage].allocation,
                 &settings_data);
    memcpy(settings_data, &m_SettingsData, settings_data_size);
    vmaUnmapMemory(allocator, m_SettingsUBO[currentImage].allocation);
  }

  clock_t t1, t2;
  void drawFrame(ImDrawData *draw_data) {
    t2 = clock();
    float dt = (double)(t2 - t1) / CLOCKS_PER_SEC; // 计时粒度为1000毫秒
    float fps = 1.0 / dt;
    std::cout << "\r";
    std::string title = "Vulkan  " + std::to_string(fps);
    glfwSetWindowTitle(window, title.c_str());
    t1 = t2; // 别忘了更新计时器

    vkWaitForFences(m_device.logicalDevice, 1,
                    &m_SyncObjects[currentFrame].InFlight, VK_TRUE,
                    std::numeric_limits<uint64_t>::max());
    vkResetFences(m_device.logicalDevice, 1,
                  &m_SyncObjects[currentFrame].InFlight);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_device.logicalDevice, m_swapChain.GetSwapChain(),
        std::numeric_limits<uint64_t>::max(),
        m_SyncObjects[currentFrame].ImageAvailable, VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_OffScreenCommandHandler.RecordOffScreenCommands(
        draw_data, imageIndex, m_swapChain.GetExtent(), m_OffScreenFrameBuffer,
        scene, m_descriptor.GetDescriptorSets());
    m_CommandHandler.RecordCommands(
        draw_data, currentFrame, m_swapChain.GetExtent(),
        m_swapChain.GetFrameBuffers(), m_descriptor.GetDescriptorSets(),
        m_descriptor.GetLightDescriptorSets(),
        m_descriptor.GetInputDescriptorSets(),
        m_descriptor.GetSettingsDescriptorSets());
    updateUniformBuffer(imageIndex);
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_SyncObjects[currentFrame].ImageAvailable;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers =
        &m_OffScreenCommandHandler.GetCommandBuffer(currentFrame);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores =
        &m_SyncObjects[currentFrame].OffScreenAvaliable;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to submit offscreen command!");
    }

    submitInfo.pWaitSemaphores =
        &m_SyncObjects[currentFrame].OffScreenAvaliable;
    submitInfo.pCommandBuffers = &m_CommandHandler.GetCommandBuffer(imageIndex);
    submitInfo.pSignalSemaphores = &m_SyncObjects[currentFrame].RenderFinished;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                      m_SyncObjects[currentFrame].InFlight) != VK_SUCCESS) {
      throw std::runtime_error("Failed to submit post process command!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_SyncObjects[currentFrame].RenderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = m_swapChain.GetSwapChainData();
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
      }
    }
    return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
      }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};

      actualExtent.width =
          std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                     capabilities.maxImageExtent.width);
      actualExtent.height =
          std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                     capabilities.maxImageExtent.height);
      return actualExtent;
    }
  }

  bool isDeviceSuitable(VkPhysicalDevice device) {
    Utility::findQueueFamilies(device, m_QueueFamilyIndices);
    bool extensionsSupported = Utility::checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport =
          m_swapChain.GetSwapChainDetails(device, m_surface);
      swapChainAdequate = !swapChainSupport.formats.empty() &&
                          !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    return m_QueueFamilyIndices.isComplete() && extensionsSupported &&
           swapChainAdequate && supportedFeatures.samplerAnisotropy;
  }

  std::vector<const char *> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions,
                                         glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
  }

  bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
      bool layerFound = false;

      for (const auto &layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) {
        return false;
      }
    }
    return true;
  }

  void createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    // depthImage.destroy(m_device.logicalDevice, allocator);
    Utility::createImage(m_swapChain.GetExtent().width,
                         m_swapChain.GetExtent().height, 1, depthFormat,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage);
    depthImage.textureImageView = Utility::createImageView(
        depthImage.textureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
  }

  VkFormat findDepthFormat() {
    return Utility::findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT,
         VK_FORMAT_D32_SFLOAT_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }

  bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           VK_FORMAT_D24_UNORM_S8_UINT;
  }

  glm::mat4 getTranslation(const glm::vec3 &translate, const glm::vec3 &rotate,
                           const glm::vec3 &scale) {

    glm::mat4 unit( // 单位矩阵
        glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0),
        glm::vec4(0, 0, 0, 1));
    glm::mat4 rotateMat;
    rotateMat = glm::rotate(unit, glm::radians(rotate.x), glm::vec3(1, 0, 0));
    rotateMat =
        glm::rotate(rotateMat, glm::radians(rotate.y), glm::vec3(0, 1, 0));
    rotateMat =
        glm::rotate(rotateMat, glm::radians(rotate.z), glm::vec3(0, 0, 1));
    glm::mat4 transMat = glm::translate(unit, translate);
    glm::mat4 scaleMat = glm::scale(unit, scale);
    return transMat * rotateMat * scaleMat;
  }

  void loadComplexModel(std::string filePath, const glm::vec3 &translate,
                        const glm::vec3 &rotate, const glm::vec3 &scale) {
    Assimp::Importer import;
    const aiScene *aiscene =
        import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs |
                                      aiProcess_GenSmoothNormals);
    if (!aiscene || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE ||
        !aiscene->mRootNode) {
      throw std::runtime_error("Fail to load model :" + filePath);
    }
    Model model;
    model.model = getTranslation(translate, rotate, scale);
    std::string rootPath = filePath.substr(0, filePath.find_last_of('/'));
    for (int i = 0; i < aiscene->mNumMeshes; ++i) {
      Mesh mesh;
      aiMesh *aimesh = aiscene->mMeshes[i];
      for (int j = 0; j < aimesh->mNumVertices; ++j) {
        Vertex vertex;
        vertex.pos.x = aimesh->mVertices[j].x;
        vertex.pos.y = aimesh->mVertices[j].y;
        vertex.pos.z = aimesh->mVertices[j].z;

        vertex.normal.x = aimesh->mNormals[j].x;
        vertex.normal.y = aimesh->mNormals[j].y;
        vertex.normal.z = aimesh->mNormals[j].z;

        if (aimesh->mTextureCoords[0]) {
          vertex.texCoord.x = aimesh->mTextureCoords[0][j].x;
          vertex.texCoord.y = aimesh->mTextureCoords[0][j].y;
        }
        mesh.vertices.push_back(vertex);
      }

      for (GLuint j = 0; j < aimesh->mNumFaces; ++j) {
        aiFace face = aimesh->mFaces[j];
        for (GLuint k = 0; k < face.mNumIndices; ++k) {
          mesh.indices.push_back(face.mIndices[k]);
        }
      }
      Mesh::createVertexBuffer(mesh.vertices, mesh.vertexBuffer, &allocator);
      Mesh::createIndexBuffer(mesh.indices, mesh.indexBuffer, &allocator);
      if (aimesh->mMaterialIndex >= 0) {
        aiMaterial *material = aiscene->mMaterials[aimesh->mMaterialIndex];
        aiString aiStr;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &aiStr);
        std::string texpath = aiStr.C_Str();
        texpath = rootPath + '/' + texpath;
        if (textures.find(texpath) == textures.end()) {
          Texture tempTexture;
          createTextureImage(texpath, tempTexture);
          createTextureImageView(tempTexture, miplevels);
          textures[texpath] = tempTexture;
        }
        mesh.textureIndex = texpath;
      }
      model.meshes.push_back(mesh);
    }
    scene.push_back(model);
  }

  void drawScene(std::vector<Model> &scene, VkCommandBuffer &commandBuffer,
                 VkDescriptorSet &descriptorSet, VkPipelineLayout &layout) {
    for (auto &model : scene) {
      updateUniformBuffer(currentFrame);
      model.draw(m_device.logicalDevice, commandBuffer, descriptorSet, layout);
    }
  }
  void createCamera() {
    camera.pos = glm::vec3(0.0, 0.0, -30.0);
    camera.aspect =
        m_swapChain.GetExtent().width / (float)m_swapChain.GetExtent().height;
  }

  void clearScene(std::vector<Model> &scene) {
    for (auto &model : scene) {
      for (auto &mesh : model.meshes) {
        mesh.vertexBuffer.destroy(m_device.logicalDevice, allocator);
        mesh.indexBuffer.destroy(m_device.logicalDevice, allocator);
      }
    }
  }

  void createVmaAllocator() {
    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.device = m_device.logicalDevice;
    allocInfo.instance = instance;
    allocInfo.physicalDevice = m_device.physicalDevice;

    if (vmaCreateAllocator(&allocInfo, &allocator) != VK_SUCCESS) {
      throw std::runtime_error("failed to create vulkan memory allocator!");
    }
  }

  const VulkanRenderData GetRenderData() {
    VulkanRenderData data = {};
    data.main_device = m_device;
    data.instance = instance;
    data.graphic_queue_index = m_QueueFamilyIndices.graphicsFamily.value();
    data.graphic_queue = graphicsQueue;
    data.imgui_descriptor_pool = m_descriptor.GetImguiDescriptorPool();
    data.min_image_count = 3;
    data.image_count = 3;
    data.render_pass = m_renderPassHandler.GetRenderPass();
    data.command_pool = m_CommandHandler.GetCommandPool();
    data.command_buffers = m_CommandHandler.GetCommandBuffers();
    data.texture_descriptor_layout = m_descriptor.GetTextureSetLayout();
    data.texture_descriptor_pool = m_descriptor.GetTexturePool();
    return data;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  void initPushConstant() {
    m_pushConstant.offset = 0;
    m_pushConstant.size = sizeof(constentData);
    m_pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    m_pipelineHandler.SetPushConstantRange(m_pushConstant);
  }

  void CreateOffScreenFrameBuffer() {
    m_OffScreenFrameBuffer.resize(m_swapChain.FrameBufferSize());
    for (uint32_t i = 0; i < m_swapChain.FrameBufferSize(); ++i) {
      std::array<VkImageView, 4> attachments = {
          m_PositionBufferImages[i].textureImageView,
          m_ColorBufferImages[i].textureImageView,
          m_NormalBufferImages[i].textureImageView,
          depthImage.textureImageView};

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferCreateInfo.renderPass =
          m_renderPassHandler.GetOffScreenRenderPass();
      frameBufferCreateInfo.attachmentCount =
          static_cast<uint32_t>(attachments.size());
      frameBufferCreateInfo.pAttachments = attachments.data();
      frameBufferCreateInfo.width = m_swapChain.GetExtent().width;
      frameBufferCreateInfo.height = m_swapChain.GetExtent().height;
      frameBufferCreateInfo.layers = 1;

      if (vkCreateFramebuffer(m_device.logicalDevice, &frameBufferCreateInfo,
                              nullptr,
                              &m_OffScreenFrameBuffer[i]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen framebuffer!");
      }
    }
  }

  void SetUniformDataStructures() {
    // View-Projection
    float const aspectRatio = static_cast<float>(m_swapChain.GetExtentWidth()) /
                              static_cast<float>(m_swapChain.GetExtentHeight());
    // Light
    pcg_extras::seed_seq_from<std::random_device> seed_source;
    pcg32 rng(seed_source);
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    float rnd_x = 0.0f;
    float y = -0.5f;
    float z = 0.0f;
    float rnd_r = 0.0f;
    float rnd_g = 0.0f;
    float rnd_b = 0.0f;

    for (int i = 0; i < NUM_LIGHTS / 2; i += 2) {
      rnd_r = uniform_dist(rng);
      rnd_g = uniform_dist(rng);
      rnd_b = uniform_dist(rng);
      Light l1 = Light(rnd_r, rnd_g, rnd_b, 1.0f);

      rnd_x = uniform_dist(rng);
      l1.SetLightPosition(glm::vec4(rnd_r, rnd_g, rnd_b, 1.0f));

      rnd_r = uniform_dist(rng);
      rnd_g = uniform_dist(rng);
      rnd_b = uniform_dist(rng);
      Light l2 = Light(rnd_r, rnd_g, rnd_b, 1.0f);

      rnd_x = uniform_dist(rng);
      l2.SetLightPosition(glm::vec4(rnd_r, rnd_g, rnd_b, 1.0f));

      m_LightData[i] = l1.GetUBOData();
      m_LightData[i + 1] = l2.GetUBOData();
    }

    m_SettingsData.render_target = 3;
  }

  void UpdateLightColor(int light_idx, const glm::vec3 &col) {
    if (light_idx >= m_LightData.size())
      return;
    m_LightData[light_idx].m_Color = col;
  }
};

int main() {
  VulkanApp app;
  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
