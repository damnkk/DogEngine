#include "DeviceContext.h"
#include "CommandBuffer.h"
#include "GUI.h"
#include "dgShaderCompiler.h"
#include "dgUtil.hpp"
#include "ktx.h"
#include "ktxvulkan.h"
#include "stb_image.h"

#define VULKAN_DEBUG
//#undef  VULKAN_DEBUG

//static member properties must be initialized, before static member func use them. And their
//initialization should be done in cpp file.
namespace dg {

GLFWwindow* DeviceContext::m_window = nullptr;

//std::shared_ptr<Camera> DeviceContext::m_camera = nullptr;

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto context = reinterpret_cast<DeviceContext*>(glfwGetWindowUserPointer(window));
  context->m_resized = true;
}

glm::mat4 getTranslation(const glm::vec3& translate, const glm::vec3& rotate,
                         const glm::vec3& scale) {
  glm::mat4 unit(// 单位矩阵
      glm::vec4(1, 0, 0, 0), glm::vec4(0, 1, 0, 0), glm::vec4(0, 0, 1, 0), glm::vec4(0, 0, 0, 1));
  glm::mat4 rotateMat;
  rotateMat = glm::rotate(unit, glm::radians(rotate.x), glm::vec3(1, 0, 0));
  rotateMat = glm::rotate(rotateMat, glm::radians(rotate.y), glm::vec3(0, 1, 0));
  rotateMat = glm::rotate(rotateMat, glm::radians(rotate.z), glm::vec3(0, 0, 1));
  glm::mat4 transMat = glm::translate(unit, translate);
  glm::mat4 scaleMat = glm::scale(unit, scale);
  return transMat * rotateMat * scaleMat;
}

ContextCreateInfo& ContextCreateInfo::setWindow(u32 width, u32 height, void* windowHandle) {
  if (!windowHandle) {
    DG_ERROR("Invalid window pointer")
    exit(-1);
  }
  m_window = (GLFWwindow*) windowHandle;
  m_windowWidth = width;
  m_windowHeight = height;
  return *this;
}

ContextCreateInfo& ContextCreateInfo::addInstanceLayer(const char* name, bool optional) {
  m_instancelayers.emplace_back(name, optional);
  return *this;
}

ContextCreateInfo& ContextCreateInfo::addInstanceExtension(const char* name, bool optional) {
  m_instanceExtensions.emplace_back(name, optional);
  return *this;
}

ContextCreateInfo& ContextCreateInfo::addDeviceExtension(const char* name, bool optional,
                                                         void* pFeatureStruct, uint32_t version) {
  m_deviceExtensions.emplace_back(name, optional, pFeatureStruct, version);
  return *this;
}

ContextCreateInfo& ContextCreateInfo::removeDeviceExtension(const char* name) {
  for (size_t i = 0; i < m_deviceExtensions.size(); ++i) {
    if (strcmp(m_deviceExtensions[i].name.c_str(), name) == 0) {
      m_deviceExtensions.erase(m_deviceExtensions.begin() + i);
    }
  }
  return *this;
}

ContextCreateInfo& ContextCreateInfo::removeInstanceExtension(const char* name) {
  for (size_t i = 0; i < m_instanceExtensions.size(); ++i) {
    if (strcmp(m_instanceExtensions[i].name.c_str(), name) == 0) {
      m_instanceExtensions.erase(m_instanceExtensions.begin() + i);
    }
  }
  return *this;
}

VkImageUsageFlags getImageUsage(const TextureCreateInfo& CI) {
  const bool isRenderTarget =
      (CI.m_flags & TextureFlags::RenderTarget_mask) == TextureFlags::RenderTarget_mask;
  const bool isComputeUsed =
      (CI.m_flags & TextureFlags::Compute_mask) == TextureFlags::Compute_mask;

  VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  usage |= isComputeUsed ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
  if (TextureFormat::has_depth_or_stencil(CI.m_imageFormat)) {
    usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  } else {
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage |= isRenderTarget ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
  }
  return usage;
}

struct CommandBufferRing {
  void init(DeviceContext* contex);
  void destroy();
  void resetPools(u32 frameIndex);

  CommandBuffer* getCommandBuffer(u32 frame, bool begin);
  CommandBuffer* get_command_buffer_instant(u32 frame);

  static u32       poolFromIndex(u32 index) { return index / k_buffer_per_pool; };
  static const u32 k_max_threads = 1;
  static const u32 k_max_pools = k_max_swapchain_images * k_max_threads;
  static const u32 k_buffer_per_pool = 4;
  static const u32 k_max_buffers = k_buffer_per_pool * k_max_pools;

  DeviceContext*             m_context;
  std::vector<VkCommandPool> m_commandPools;
  std::vector<CommandBuffer> m_commandBuffers;
  std::vector<u32>           m_nextFreePerThreadFrame;
};

std::vector<const char*> deviceExtensions = {};

static CommandBufferRing commandBufferRing;

void CommandBufferRing::init(DeviceContext* contex) {
  if (!contex) {
    DG_ERROR("Invalid contex pointer, check this again")
    exit(-1);
  }
  m_context = contex;
  m_commandPools.resize(k_max_pools);
  m_commandBuffers.resize(k_max_buffers);
  m_nextFreePerThreadFrame.resize(k_max_pools);
  for (int i = 0; i < k_max_pools; ++i) {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_context->m_graphicQueueIndex;
    VkResult res =
        vkCreateCommandPool(m_context->m_logicDevice, &poolInfo, nullptr, &m_commandPools[i]);
    DGASSERT(res == VK_SUCCESS)
  }

  for (int i = 0; i < k_max_buffers; ++i) {
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = m_commandPools[i / k_buffer_per_pool];
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    auto res = vkAllocateCommandBuffers(m_context->m_logicDevice, &cmdAllocInfo,
                                        &m_commandBuffers[i].m_commandBuffer);
    DGASSERT(res == VK_SUCCESS)
    m_commandBuffers[i].m_context = m_context;
    m_commandBuffers[i].m_handle = i;
    m_commandBuffers[i].reset();
  }
}

void CommandBufferRing::destroy() {
  for (u32 i = 0; i < m_commandPools.size(); ++i) {
    vkDestroyCommandPool(m_context->m_logicDevice, m_commandPools[i], nullptr);
  }
}

void CommandBufferRing::resetPools(uint32_t frameIndex) {
  for (int i = 0; i < k_max_threads; ++i) {
    vkResetCommandPool(m_context->m_logicDevice, m_commandPools[frameIndex * k_max_threads + i], 0);
  }
}

CommandBuffer* CommandBufferRing::getCommandBuffer(u32 frame, bool begin) {
  u32            bufferIndex = frame * k_buffer_per_pool;
  CommandBuffer* cmdBuffer = &m_commandBuffers[bufferIndex];
  if (begin) {
    cmdBuffer->reset();

    VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer->m_commandBuffer, &cmdBeginInfo);
    cmdBuffer->m_isBegin = true;
  }
  return cmdBuffer;
}

CommandBuffer* CommandBufferRing::get_command_buffer_instant(u32 frame) {
  CommandBuffer* cb = &m_commandBuffers[frame * k_buffer_per_pool + 1];
  return cb;
}

//-----------------all the access function-------------------------
Sampler* DeviceContext::accessSampler(SamplerHandle samplerHandle) {
  return m_samplers.accessResource(samplerHandle.index);
}

const Sampler* DeviceContext::accessSampler(SamplerHandle samplerHandle) const {
  return m_samplers.accessResource(samplerHandle.index);
}

Texture* DeviceContext::accessTexture(TextureHandle texturehandle) {
  return m_textures.accessResource(texturehandle.index);
}

const Texture* DeviceContext::accessTexture(TextureHandle texturehandle) const {
  return m_textures.accessResource(texturehandle.index);
}

RenderPass* DeviceContext::accessRenderPass(RenderPassHandle renderpassHandle) {
  return m_renderPasses.accessResource(renderpassHandle.index);
}

const RenderPass* DeviceContext::accessRenderPass(RenderPassHandle renderpassHandle) const {
  return m_renderPasses.accessResource(renderpassHandle.index);
}

Buffer* DeviceContext::accessBuffer(BufferHandle handle) {
  return m_buffers.accessResource(handle.index);
}

const Buffer* DeviceContext::accessBuffer(BufferHandle handle) const {
  return m_buffers.accessResource(handle.index);
}

DescriptorSetLayout* DeviceContext::accessDescriptorSetLayout(DescriptorSetLayoutHandle layout) {
  return m_descriptorSetLayouts.accessResource(layout.index);
}

const DescriptorSetLayout*
DeviceContext::accessDescriptorSetLayout(DescriptorSetLayoutHandle layout) const {
  return m_descriptorSetLayouts.accessResource(layout.index);
}

DescriptorSet* DeviceContext::accessDescriptorSet(DescriptorSetHandle set) {
  return m_descriptorSets.accessResource(set.index);
}

const DescriptorSet* DeviceContext::accessDescriptorSet(DescriptorSetHandle set) const {
  return m_descriptorSets.accessResource(set.index);
}

ShaderState* DeviceContext::accessShaderState(ShaderStateHandle handle) {
  return m_shaderStates.accessResource(handle.index);
}

const ShaderState* DeviceContext::accessShaderState(ShaderStateHandle handle) const {
  return m_shaderStates.accessResource(handle.index);
}

Pipeline* DeviceContext::accessPipeline(PipelineHandle handle) {
  return m_pipelines.accessResource(handle.index);
}

FrameBuffer* DeviceContext::accessFrameBuffer(FrameBufferHandle handle) {
  return m_frameBuffers.accessResource(handle.index);
}

const FrameBuffer* DeviceContext::accessFrameBuffer(FrameBufferHandle handle) const {
  return m_frameBuffers.accessResource(handle.index);
}

//Resource indirect destroy
void DeviceContext::DestroyTexture(TextureHandle& texIndex) {
  if (texIndex.index > m_textures.m_poolSize) {
    DG_WARN("You are trying to add an invalid texture in delection queue");
  }
  m_delectionQueue.push_back({ResourceUpdateType::Enum::Texture, texIndex.index, m_currentFrame});
  texIndex = {k_invalid_index};
}

void DeviceContext::DestroyBuffer(BufferHandle& bufferIndex) {
  if (bufferIndex.index > m_buffers.m_poolSize) {
    DG_WARN("You are trying to add an invalid buffer in delection queue");
  }
  m_delectionQueue.push_back({ResourceUpdateType::Enum::Buffer, bufferIndex.index, m_currentFrame});
  bufferIndex = {k_invalid_index};
}

void DeviceContext::DestroyRenderPass(RenderPassHandle& passIndex) {
  if (passIndex.index > m_renderPasses.m_poolSize) {
    DG_WARN("You are trying to add an invalid renderPass in delection queue");
  }
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::RenderPass, passIndex.index, m_currentFrame});
  passIndex = {k_invalid_index};
}

void DeviceContext::DestroyPipeline(PipelineHandle& pipelineIndex) {
  if (pipelineIndex.index > m_pipelines.m_poolSize) {
    DG_WARN("You are trying to add an invalid pipeline in deletion queue");
  }
  Pipeline* pipeline = accessPipeline(pipelineIndex);
  for (int i = 0; i < pipeline->m_activeLayouts; ++i) {
    if (pipeline->m_DescriptorSetLayout[i]->m_handle.index == m_bindlessDescriptorSetLayout.index)
      continue;
    DestroyDescriptorSetLayout(pipeline->m_DescriptorSetLayout[i]->m_handle);
  }
  if (pipeline->m_pipelineType == ShaderState::eRayTracing) {
    DestroyBuffer(pipeline->m_shaderBindingTableBuffer);
  }
  DestroyShaderState(pipeline->m_shaderState);
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::Pipeline, pipelineIndex.index, m_currentFrame, true});
  pipelineIndex = {k_invalid_index};
}

void DeviceContext::DestroyDescriptorSet(DescriptorSetHandle& dsIndex) {
  if (dsIndex.index > m_descriptorSets.m_poolSize) {
    DG_WARN("You are trying to add an invalid descriptor set in delection queue");
  }
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::DescriptorSet, dsIndex.index, m_currentFrame});
  dsIndex = {k_invalid_index};
}

void DeviceContext::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle& dsLayoutIndex) {
  if (dsLayoutIndex.index > m_descriptorSetLayouts.m_poolSize) {
    DG_WARN("You are trying to add an invalid descriptor set layout in delection queue");
  }
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::DescriptorSetLayout, dsLayoutIndex.index, m_currentFrame});
  dsLayoutIndex = {k_invalid_index};
}

void DeviceContext::DestroyShaderState(ShaderStateHandle& ssHandle) {
  if (ssHandle.index > m_shaderStates.m_poolSize) {
    DG_WARN("You are trying to add an invalid shader state in delection queue");
  }
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::ShaderState, ssHandle.index, m_currentFrame});
  ssHandle = {k_invalid_index};
}

void DeviceContext::DestroySampler(SamplerHandle& sampHandle) {
  if (sampHandle.index > m_samplers.m_poolSize) {
    DG_WARN("You are trying to add an invalid smapler to delection queue");
  }
  m_delectionQueue.push_back({ResourceUpdateType::Enum::Sampler, sampHandle.index, m_currentFrame});
  sampHandle = {k_invalid_index};
}

void DeviceContext::DestroyFrameBuffer(FrameBufferHandle& fboHandle, bool destroyTexture) {
  if (fboHandle.index == k_invalid_index) {
    DG_WARN("You are trying to add an invalid FramBuffer to delection queue");
    return;
  }
  if (!destroyTexture) {
    FrameBuffer* fbo = accessFrameBuffer(fboHandle);
    fbo->m_numRenderTargets = 0;
  }
  m_delectionQueue.push_back(
      {ResourceUpdateType::Enum::FrameBuffer, fboHandle.index, m_currentFrame});
  fboHandle = {k_invalid_index};
}

void DeviceContext::destroyBufferInstant(ResourceHandle buffer) {
  Buffer* bufferInstance = accessBuffer({buffer});
  if (bufferInstance && bufferInstance->m_parentHandle.index == k_invalid_index) {
    vmaDestroyBuffer(m_vma, bufferInstance->m_buffer, bufferInstance->m_allocation);
  }
  m_buffers.releaseResource(buffer);
}

void DeviceContext::destroyTextureInstant(ResourceHandle handle) {
  Texture* textureInstance = accessTexture({handle});
  if (textureInstance) {
    vmaDestroyImage(m_vma, textureInstance->m_image, textureInstance->m_vma);
    vkDestroyImageView(m_logicDevice, textureInstance->m_imageInfo.imageView, nullptr);
  }
  m_textures.releaseResource(handle);
}

void DeviceContext::destroyPipelineInstant(ResourceHandle handle) {
  Pipeline* pipelineInstance = accessPipeline({handle});
  if (pipelineInstance) {
    vkDestroyPipeline(m_logicDevice, pipelineInstance->m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, pipelineInstance->m_pipelineLayout, nullptr);
  }
  m_pipelines.releaseResource(handle);
}

void DeviceContext::destroySamplerInstant(ResourceHandle handle) {
  Sampler* samplerInstance = accessSampler({handle});
  if (samplerInstance) { vkDestroySampler(m_logicDevice, samplerInstance->m_sampler, nullptr); }
  m_samplers.releaseResource(handle);
}

void DeviceContext::destroyDescriptorSetLayoutInstant(ResourceHandle handle) {
  DescriptorSetLayout* setLayoutInstance = accessDescriptorSetLayout({handle});
  if (setLayoutInstance) {
    vkDestroyDescriptorSetLayout(m_logicDevice, setLayoutInstance->m_descriptorSetLayout, nullptr);
  }
  m_descriptorSetLayouts.releaseResource(handle);
}

void DeviceContext::destroyDescriptorInstant(ResourceHandle handle) {
  DescriptorSet* setInstance = accessDescriptorSet({handle});
  if (setInstance && handle != k_invalid_index) {
    vkFreeDescriptorSets(m_logicDevice, setInstance->m_parentPool, 1,
                         &setInstance->m_vkdescriptorSet);
    m_descriptorSets.releaseResource(handle);
  }
}

void DeviceContext::destroyRenderpassInstant(ResourceHandle handle) {
  RenderPass* renderpassInstance = accessRenderPass({handle});
  if (renderpassInstance) {
    vkDestroyRenderPass(m_logicDevice, renderpassInstance->m_renderPass, nullptr);
  }
  m_renderPasses.releaseResource(handle);
}

void DeviceContext::destroyShaderStateInstant(ResourceHandle handle) {
  ShaderState* stateInstance = accessShaderState({handle});
  if (stateInstance) {
    for (int i = 0; i < stateInstance->m_activeShaders; ++i) {
      vkDestroyShaderModule(m_logicDevice, stateInstance->m_shaderStageInfo[i].module, nullptr);
    }
  }
  m_shaderStates.releaseResource(handle);
}

void DeviceContext::destroyFrameBufferInstant(ResourceHandle fbo) {
  FrameBuffer* framebuffer = accessFrameBuffer({fbo});
  if (framebuffer) {
    for (int i = 0; i < framebuffer->m_numRenderTargets; ++i) {
      destroyTextureInstant(framebuffer->m_colorAttachments[i].index);
    }

    if (framebuffer->m_depthStencilAttachment.index != k_invalid_index) {
      destroyTextureInstant(framebuffer->m_depthStencilAttachment.index);
    }
    vkDestroyFramebuffer(m_logicDevice, framebuffer->m_frameBuffer, nullptr);
  }
  m_frameBuffers.releaseResource(fbo);
}
void DeviceContext::resize(u16 width, u16 height) {
  m_swapChainWidth = width;
  m_swapChainHeight = height;
  m_resized = true;
}

void getRequiredInstanceExtensions(std::vector<const char*>& extensions) {
  u32          extCount = 0;
  const char** extensionName;
  extensionName = glfwGetRequiredInstanceExtensions(&extCount);
  for (u32 i = 0; i < extCount; ++i) { extensions.push_back(extensionName[i]); }
#if defined(VULKAN_DEBUG)
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
}

VkPhysicalDeviceFeatures2 phy2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
void*                     getRequiredDeviceExtensions(VkPhysicalDevice                   phyDevice,
                                                      const ContextCreateInfo::ExtArray& deviceExtArray) {

  struct ExtensionHeader {
    VkStructureType sType;
    void*           pNext;
  };
  auto* header = reinterpret_cast<ExtensionHeader*>(&phy2);
  for (int i = 0; i < deviceExtArray.size(); ++i) {
    if (!deviceExtArray[i].pFeatureStruct) {
      DG_WARN("Device feature structure of name: {} is not provided, this device feature may not "
              "be enable!",
              deviceExtArray[i].name.c_str());
      continue;
    }
    header->pNext = deviceExtArray[i].pFeatureStruct;
    header = reinterpret_cast<ExtensionHeader*>(header->pNext);
  }
  vkGetPhysicalDeviceFeatures2(phyDevice, &phy2);
  return &phy2;
}

bool DeviceContext::getFamilyIndex(VkPhysicalDevice& device) {
  u32 queuefamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queuefamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> propers(queuefamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queuefamilyCount, propers.data());
  int      family_index = 0;
  VkBool32 surface_supported;
  for (; family_index < propers.size(); ++family_index) {
    auto queue_family = propers[family_index];
    if (queue_family.queueCount > 0 && (queue_family.queueFlags & (VK_QUEUE_GRAPHICS_BIT))) {
      vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index, m_surface, &surface_supported);
      if (surface_supported) {
        m_graphicQueueIndex = family_index;
        continue;
      }
    }
    if (queue_family.queueCount > 0 && (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
        && m_graphicQueueIndex != -1 && family_index > m_graphicQueueIndex) {
      if (m_computeQueueIndex == -1) m_computeQueueIndex = family_index;
    }
    if (queue_family.queueCount > 0 && (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
        && m_computeQueueIndex != -1 && family_index > m_computeQueueIndex) {
      if (m_transferQueueIndex == -1) m_transferQueueIndex = family_index;
    }
  }
  return surface_supported;
}

bool DeviceContext::setPresentMode(VkPresentModeKHR presentMode) {
  u32 supportedSize = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &supportedSize, nullptr);
  std::vector<VkPresentModeKHR> supportedMode(supportedSize);
  vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &supportedSize,
                                            supportedMode.data());
  bool modelFound = false;
  for (auto& i : supportedMode) {
    if (presentMode == i) {
      m_presentMode = i;
      modelFound = true;
      break;
    }
  }
  if (!modelFound) m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
  m_swapchainImageCount = 3;
  return modelFound;
}

void DeviceContext::linkTextureSampler(TextureHandle texture, SamplerHandle sampler) {
  Texture* tex = accessTexture(texture);
  Sampler* samp = accessSampler(sampler);
  tex->m_imageInfo.sampler = samp->m_sampler;
  return;
}

CommandBuffer* DeviceContext::getCommandBuffer(QueueType::Enum type, bool begin) {
  CommandBuffer* cmd = commandBufferRing.getCommandBuffer(m_currentFrame, begin);
  return cmd;
}

CommandBuffer* DeviceContext::getInstantCommandBuffer() {
  CommandBuffer* cmd = commandBufferRing.get_command_buffer_instant(m_currentFrame);
  return cmd;
}

void DeviceContext::queueCommandBuffer(CommandBuffer* cmdBuffer) {
  m_queuedCommandBuffer.push_back(cmdBuffer);
}

static void vulkanCreateTexture(DeviceContext* context, const TextureCreateInfo& tc,
                                TextureHandle handle, Texture* texture) {
  //create image
  texture->m_extent = tc.m_textureExtent;
  texture->m_format = tc.m_imageFormat;
  texture->m_mipLevel = tc.m_mipmapLevel;
  texture->m_name = tc.name;
  texture->m_type = tc.m_imageType;
  texture->m_handle = handle;
  texture->m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  Sampler* sampler = context->accessSampler(context->m_defaultSampler);
  texture->m_imageInfo.sampler = sampler->m_sampler;
  texture->m_usage = getImageUsage(tc);
  texture->m_flags = tc.m_flags;
  texture->bindless = tc.bindless;

  VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imgCreateInfo.imageType = to_vk_image_type(tc.m_imageType);
  imgCreateInfo.extent = tc.m_textureExtent;
  imgCreateInfo.format = tc.m_imageFormat;
  imgCreateInfo.mipLevels = tc.m_mipmapLevel;
  imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCreateInfo.arrayLayers = 1;
  imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgCreateInfo.usage |= texture->m_usage;
  imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo imageAllocInfo{};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  auto res = vmaCreateImage(context->m_vma, &imgCreateInfo, &imageAllocInfo, &texture->m_image,
                            &texture->m_vma, nullptr);
  DGASSERT(res == VK_SUCCESS)
  context->setResourceName(VK_OBJECT_TYPE_IMAGE, (uint64_t) texture->m_image, tc.name.c_str());
  //create image view
  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.viewType = to_vk_image_view_type(tc.m_imageType);
  viewInfo.image = texture->m_image;
  viewInfo.format = texture->m_format;
  if (TextureFormat::has_depth_or_stencil(viewInfo.format)) {
    viewInfo.subresourceRange.aspectMask =
        TextureFormat::has_depth(viewInfo.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
  } else {
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = tc.m_mipmapLevel;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;
  DGASSERT(
      vkCreateImageView(context->m_logicDevice, &viewInfo, nullptr, &texture->m_imageInfo.imageView)
      == VK_SUCCESS);
  context->setResourceName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) texture->m_imageInfo.imageView,
                           tc.name.c_str());
  if (tc.bindless) {
    ResourceUpdate resUpdate{ResourceUpdateType::Texture, texture->m_handle.index,
                             context->m_currentFrame, 0};
    context->m_texture_to_update_bindless.push_back(resUpdate);
  }
}

static void vulkanCreateSwapChainPass(DeviceContext* context, const RenderPassCreateInfo& ri,
                                      RenderPass* pass) {
  VkAttachmentDescription color_attach{};
  color_attach.format = context->m_surfaceFormat.format;
  color_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attach.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_ref;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color_ref.attachment = 0;

  VkAttachmentDescription depth_attach{};
  Texture*                depthTexture = context->accessTexture(context->m_depthTexture);
  depth_attach.format = depthTexture->m_format;
  depth_attach.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attach.initialLayout = depthTexture->m_imageInfo.imageLayout;
  depth_attach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_ref;
  depth_ref.attachment = 1;
  depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass_desc{};
  subpass_desc.colorAttachmentCount = 1;
  subpass_desc.pColorAttachments = &color_ref;
  subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass_desc.pDepthStencilAttachment = &depth_ref;

  std::array<VkAttachmentDescription, 2> attchs = {color_attach, depth_attach};
  VkSubpassDependency                    dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  passInfo.attachmentCount = attchs.size();
  passInfo.pAttachments = attchs.data();
  passInfo.subpassCount = 1;
  passInfo.pSubpasses = &subpass_desc;
  //这里是为了规避imgui docking 错误，增加了dependency这条，原本是没有的
  //      passInfo.dependencyCount =1;
  //      passInfo.pDependencies = &dependency;
  DGASSERT(vkCreateRenderPass(context->m_logicDevice, &passInfo, nullptr, &pass->m_renderPass)
           == VK_SUCCESS);
  pass->m_width = context->m_swapChainWidth;
  pass->m_height = context->m_swapChainHeight;
  context->setResourceName(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) pass->m_renderPass,
                           ri.name.c_str());

  VkFramebufferCreateInfo fboInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  fboInfo.width = pass->m_width;
  fboInfo.height = pass->m_height;
  fboInfo.renderPass = pass->m_renderPass;
  fboInfo.attachmentCount = attchs.size();
  fboInfo.layers = 1;

  std::array<VkImageView, 2> fboAttachs = {};
  fboAttachs[1] = depthTexture->m_imageInfo.imageView;
  context->m_swapchainFbos.resize(context->m_swapchainImageCount);
  for (int i = 0; i < context->m_swapchainImageCount; ++i) {
    fboAttachs[0] = context->m_swapchainImageViews[i];
    fboInfo.pAttachments = fboAttachs.data();
    vkCreateFramebuffer(context->m_logicDevice, &fboInfo, nullptr, &context->m_swapchainFbos[i]);
    context->setResourceName(VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t) context->m_swapchainFbos[i],
                             ri.name.c_str());
  }
}
static VkRenderPass vulkanCreateRenderPass(DeviceContext*              context,
                                           const RenderPassCreateInfo& passInfo) {
  std::vector<VkAttachmentDescription> color_attch(passInfo.m_outputTextures.size());
  std::vector<VkAttachmentReference>   color_ref(passInfo.m_outputTextures.size());
  VkAttachmentLoadOp                   colorOp, depthOp, stencilOp;
  VkImageLayout                        colorInitial, DepthInitial;
  switch (passInfo.m_color) {
    case RenderPassOperation::Clear: {
      colorOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorInitial = VK_IMAGE_LAYOUT_UNDEFINED;
      break;
    }
    case RenderPassOperation::Load: {
      colorOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      colorInitial = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      break;
    }
    default: {
      colorOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorInitial = VK_IMAGE_LAYOUT_UNDEFINED;
      break;
    }
  }

  switch (passInfo.m_depth) {
    case RenderPassOperation::Load: {
      depthOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      DepthInitial = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      break;
    }
    case RenderPassOperation::Clear: {
      depthOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      DepthInitial = VK_IMAGE_LAYOUT_UNDEFINED;
      break;
    }
    default: {
      depthOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      DepthInitial = VK_IMAGE_LAYOUT_UNDEFINED;
      break;
    }
  }

  switch (passInfo.m_stencil) {
    case RenderPassOperation::Load: {
      stencilOp = VK_ATTACHMENT_LOAD_OP_LOAD;
      break;
    }
    case RenderPassOperation::Clear: {
      stencilOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      break;
    }
    default: {
      stencilOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      break;
    }
  }

  for (int i = 0; i < passInfo.m_outputTextures.size(); ++i) {
    VkAttachmentDescription& attDesc = color_attch[i];
    Texture* tex = context->m_textures.accessResource(passInfo.m_outputTextures[i].index);
    attDesc.format = tex->m_format;
    attDesc.loadOp = colorOp;
    attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attDesc.stencilLoadOp = stencilOp;
    attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attDesc.initialLayout = tex->m_imageInfo.imageLayout;
    attDesc.finalLayout = passInfo.m_finalLayout;

    VkAttachmentReference& attref = color_ref[i];
    attref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attref.attachment = i;
  }
  std::vector<VkAttachmentDescription> attach = color_attch;
  VkAttachmentDescription              depthDesc{};
  VkAttachmentReference                depthRef{};
  if (passInfo.m_depthTexture.index != k_invalid_index) {
    Texture* depthTex = context->m_textures.accessResource(passInfo.m_depthTexture.index);
    depthDesc.format = depthTex->m_format;
    depthDesc.loadOp = colorOp;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    depthDesc.stencilLoadOp = stencilOp;
    depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthDesc.initialLayout = DepthInitial;
    depthDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthRef.attachment = passInfo.m_outputTextures.size();
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attach.push_back(depthDesc);
  }

  VkSubpassDescription subpassDesc{};
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDesc.colorAttachmentCount = color_ref.size();
  subpassDesc.pColorAttachments = color_ref.data();
  if (passInfo.m_depthTexture.index != k_invalid_index) {
    subpassDesc.pDepthStencilAttachment = &depthRef;
  }
  //subpassDesc.flags = 0;

  std::array<VkSubpassDependency, 2> dependencies{};
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = attach.size();
  renderPassInfo.pAttachments = attach.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependencies.data();
  VkRenderPass renderpass;
  DGASSERT(vkCreateRenderPass(context->m_logicDevice, &renderPassInfo, nullptr, &renderpass)
           == VK_SUCCESS);
  context->setResourceName(VK_OBJECT_TYPE_RENDER_PASS, (uint64_t) renderpass,
                           passInfo.name.c_str());
  return renderpass;
}

void DeviceContext::createSwapChain() {
  //swapchain
  VkSurfaceCapabilitiesKHR surfaceCap;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCap);

  VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainInfo.surface = m_surface;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.minImageCount = m_swapchainImageCount;
  swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageExtent = {surfaceCap.currentExtent.width, surfaceCap.currentExtent.height};
  swapchainInfo.imageFormat = m_surfaceFormat.format;
  swapchainInfo.queueFamilyIndexCount = 1;
  swapchainInfo.presentMode = m_presentMode;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
      | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = surfaceCap.currentTransform;

  auto result = vkCreateSwapchainKHR(m_logicDevice, &swapchainInfo, nullptr, &m_swapchain);
  DGASSERT(result == VK_SUCCESS);

  //swapchainImage
  u32 swapChainImageCount;
  vkGetSwapchainImagesKHR(m_logicDevice, m_swapchain, &swapChainImageCount, nullptr);
  m_swapchainImageCount = swapChainImageCount;
  m_swapchainImages.resize(swapChainImageCount);
  m_swapchainImageViews.resize(swapChainImageCount);
  vkGetSwapchainImagesKHR(m_logicDevice, m_swapchain, &swapChainImageCount,
                          m_swapchainImages.data());
  //swapchainImageView;
  for (u32 i = 0; i < m_swapchainImages.size(); ++i) {
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = m_swapchainImages[i];
    viewInfo.format = m_surfaceFormat.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    result = vkCreateImageView(m_logicDevice, &viewInfo, nullptr, &m_swapchainImageViews[i]);
    DGASSERT(result == VK_SUCCESS);
  }
}

void DeviceContext::destroySwapChain() {
  for (int i = 0; i < m_swapchainImageCount; ++i) {
    vkDestroyImageView(m_logicDevice, m_swapchainImageViews[i], nullptr);
    vkDestroyFramebuffer(m_logicDevice, m_swapchainFbos[i], nullptr);
    //vkDestroyImage(m_logicDevice,m_fbo)
  }
  vkDestroySwapchainKHR(m_logicDevice, m_swapchain, nullptr);
}

static void vulkanResizeTexture(DeviceContext* context, Texture* texture, Texture* textureToDelete,
                                u32 width, u32 height, u32 depth) {
  textureToDelete->m_image = texture->m_image;
  textureToDelete->m_imageInfo = texture->m_imageInfo;
  textureToDelete->m_vma = texture->m_vma;

  TextureCreateInfo texInfo;
  texInfo.setExtent({width, height, depth})
      .setFormat(texture->m_format)
      .setName(texture->m_name.c_str())
      .setMipmapLevel(texture->m_mipLevel);
  texInfo.m_imageType = texture->m_type;
  texInfo.m_flags = texture->m_flags;
  vulkanCreateTexture(context, texInfo, texture->m_handle, texture);
}

void DeviceContext::reCreateSwapChain() {
  vkDeviceWaitIdle(m_logicDevice);
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);
  VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;
  if (swapchainExtent.width == 0 || swapchainExtent.height == 0) { return; }
  m_swapChainWidth = swapchainExtent.width;
  m_swapChainHeight = swapchainExtent.height;
  RenderPass* swapChainPass = accessRenderPass(m_swapChainPass);
  vkDestroyRenderPass(m_logicDevice, swapChainPass->m_renderPass, nullptr);
  destroySwapChain();
  //一定要销毁surface吗?我觉得不用吧
  // vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  // if(glfwCreateWindowSurface(m_instance, m_window, nullptr , &m_surface)!=VK_SUCCESS){
  //     DG_ERROR("Can not create glfw window in swapchain recreation processing");
  //     return;
  // }
  createSwapChain();
  TextureHandle textureToDelete = {m_textures.obtainResource()};
  Texture*      textureToDeletePtr = accessTexture(textureToDelete);
  Texture*      depthTexture = accessTexture(m_depthTexture);
  vulkanResizeTexture(this, depthTexture, textureToDeletePtr, m_swapChainWidth, m_swapChainHeight,
                      1);
  DestroyTexture(textureToDelete);
  RenderPassCreateInfo renderPassCreation{};
  renderPassCreation.setType(RenderPassType::Enum::SwapChain).setName("SwapChain");
  vulkanCreateSwapChainPass(this, renderPassCreation, swapChainPass);
  vkDeviceWaitIdle(m_logicDevice);
}

void DeviceContext::updateDescriptorSet(DescriptorSetHandle set) {
  if (set.index != k_invalid_index && set.index < m_descriptorSets.m_poolSize) {
    DescriptorSetUpdate setUpdate{set, m_currentFrame};
    m_descriptorSetUpdateQueue.push_back(setUpdate);
  } else {
    DG_ERROR("Attempt to update a invalid Descriptor Set");
  }
}

//obtain一个对象,把索引也就是句柄返回出来
SamplerHandle DeviceContext::createSampler(SamplerCreateInfo& scf) {
  SamplerHandle sh = {m_samplers.obtainResource()};
  if (sh.index == k_invalid_index) { return sh; }
  Sampler* sampler = accessSampler(sh);
  sampler->name = scf.name;
  sampler->m_addressU = scf.m_addressU;
  sampler->m_addressV = scf.m_addressV;
  sampler->m_addressW = scf.m_addressW;
  sampler->m_maxFilter = scf.m_maxFilter;
  sampler->m_minFilter = scf.m_minFilter;
  sampler->m_minLod = scf.m_minLod;
  sampler->m_maxLod = scf.m_maxLod;
  sampler->m_LodBias = scf.m_LodBias;
  VkSamplerCreateInfo scInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  scInfo.addressModeU = scf.m_addressU;
  scInfo.addressModeV = scf.m_addressV;
  scInfo.addressModeW = scf.m_addressW;
  scInfo.mipmapMode = scf.m_mipmapMode;
  scInfo.magFilter = scf.m_maxFilter;
  scInfo.minFilter = scf.m_minFilter;
  scInfo.minLod = scf.m_minLod;
  scInfo.maxLod = scf.m_maxLod;
  scInfo.mipLodBias = scf.m_LodBias;
  scInfo.anisotropyEnable = VK_FALSE;
  scInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  scInfo.unnormalizedCoordinates = VK_FALSE;
  scInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
  vkCreateSampler(m_logicDevice, &scInfo, nullptr, &sampler->m_sampler);
  setResourceName(VK_OBJECT_TYPE_SAMPLER, (uint64_t) sampler->m_sampler, scf.name.c_str());
  return sh;
}

BufferHandle DeviceContext::createBuffer(BufferCreateInfo& bufferInfo) {
  BufferHandle handle = {m_buffers.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  Buffer* buffer = accessBuffer(handle);
  buffer->name = bufferInfo.name.c_str();
  buffer->m_bufferSize = bufferInfo.m_bufferSize;
  buffer->m_bufferUsage = bufferInfo.m_bufferUsage;
  buffer->m_handle = handle;
  buffer->m_globalOffset = 0;
  buffer->m_parentHandle = {k_invalid_index};
  VkBufferCreateInfo bufinf = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufinf.size = bufferInfo.m_bufferSize > 0 ? bufferInfo.m_bufferSize : 1;
  bufinf.usage = bufferInfo.m_bufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT
      | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufinf.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vmaInfo = {};
  vmaInfo.flags = bufferInfo.m_deviceOnly ? VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
                                          : VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  vmaInfo.usage = VMA_MEMORY_USAGE_AUTO;

  VmaAllocationInfo vmaAllocInfo;
  if (bufferInfo.m_alignment) {
    DGASSERT(vmaCreateBufferWithAlignment(m_vma, &bufinf, &vmaInfo, bufferInfo.m_alignment,
                                          &buffer->m_buffer, &buffer->m_allocation, &vmaAllocInfo)
             == VK_SUCCESS)
  } else {

    DGASSERT(vmaCreateBuffer(m_vma, &bufinf, &vmaInfo, &buffer->m_buffer, &buffer->m_allocation,
                             &vmaAllocInfo)
             == VK_SUCCESS);
  }
  setResourceName(VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer->m_buffer, bufferInfo.name.c_str());
  buffer->m_deviceMemory = vmaAllocInfo.deviceMemory;
  if (bufferInfo.m_sourceData) {
    //if not deviceOnly, then this buffer is staging buffer
    if (!bufferInfo.m_deviceOnly) {
      void* data;
      vmaMapMemory(m_vma, buffer->m_allocation, &data);
      memcpy(data, bufferInfo.m_sourceData, bufferInfo.m_bufferSize);
      vmaUnmapMemory(m_vma, buffer->m_allocation);
    } else {
      //staging buffer creation
      BufferCreateInfo stagingBufInfo;
      stagingBufInfo.setName("staging buffer")
          .setData(bufferInfo.m_sourceData)
          .setDeviceOnly(false)
          .setUsageSize(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        bufferInfo.m_bufferSize);
      BufferHandle staginghandle = createBuffer(stagingBufInfo);
      Buffer*      stagingbuf = accessBuffer(staginghandle);

      VkBufferCopy cpy{0, 0, bufferInfo.m_bufferSize};
      //commandbuffer get
      CommandBuffer*           cmd = getInstantCommandBuffer();
      VkCommandBufferBeginInfo begininfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(cmd->m_commandBuffer, &begininfo);
      vkCmdCopyBuffer(cmd->m_commandBuffer, stagingbuf->m_buffer, buffer->m_buffer, 1, &cpy);
      // vkEndCommandBuffer(cmd->m_commandBuffer);
      // VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
      // submitInfo.commandBufferCount = 1;
      // submitInfo.pCommandBuffers = &cmd->m_commandBuffer;
      // vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
      // vkQsueueWaitIdle(m_graphicsQueue);
      cmd->flush(m_graphicsQueue);
      DestroyBuffer(staginghandle);
      staginghandle.index = k_invalid_index;
    }
  }
  if (bufferInfo.m_presistent) { buffer->m_mappedMemory = vmaAllocInfo.pMappedData; }

  return handle;
}

//mipcount must bigger or equal 1
void DeviceContext::addImageBarrier(VkCommandBuffer cmdBuffer, Texture* texture,
                                    ResourceState oldState, ResourceState newState,
                                    u32 baseMipLevel, u32 mipCount, bool isDepth) {
  VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageBarrier.image = texture->m_image;
  imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.subresourceRange.aspectMask =
      isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.layerCount = 1;
  imageBarrier.subresourceRange.levelCount = mipCount;

  imageBarrier.subresourceRange.baseMipLevel = baseMipLevel;
  imageBarrier.oldLayout = util_to_vk_image_layout(oldState);
  imageBarrier.newLayout = util_to_vk_image_layout(newState);
  imageBarrier.srcAccessMask = util_to_vk_access_flags(oldState);
  imageBarrier.dstAccessMask = util_to_vk_access_flags(newState);

  const VkPipelineStageFlags sourceStageMask =
      util_determine_pipeline_stage_flags(imageBarrier.srcAccessMask, QueueType::Graphics);
  const VkPipelineStageFlags dstStageMask =
      util_determine_pipeline_stage_flags(imageBarrier.dstAccessMask, QueueType::Graphics);

  vkCmdPipelineBarrier(cmdBuffer, sourceStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                       &imageBarrier);
  texture->m_resourceState = newState;
  texture->m_imageInfo.imageLayout = imageBarrier.newLayout;
}

void DeviceContext::resizeGameViewPass(VkExtent2D extent) {
  //if (m_gameViewWidth == extent.width && m_gameViewHeight == extent.height) return;
  vkDeviceWaitIdle(m_logicDevice);
  m_gameViewWidth = extent.width;
  m_gameViewHeight = extent.height;
  RenderPass* pass = accessRenderPass(m_gameViewPass);
  pass->m_width = extent.width;
  pass->m_height = extent.height;
  std::vector<FrameBufferHandle> oldFbos = pass->m_frameBuffers;
  pass->m_frameBuffers.clear();
  for (int i = 0; i < oldFbos.size(); ++i) {
    //frame buffer and it's texture attachment is destroied;
    FrameBuffer* fbo = accessFrameBuffer(oldFbos[i]);
    if (i != 0) fbo->m_depthStencilAttachment = {k_invalid_index};
    destroyFrameBufferInstant(oldFbos[i].index);
    destroyDescriptorInstant(m_gameViewTextureDescs[i].index);
    TextureCreateInfo gameViewFrameTextureInfo{};
    gameViewFrameTextureInfo.setName("gameViewFrameTextureRecreate")
        .setExtent({m_gameViewWidth, m_gameViewHeight, 1})
        .setFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
        .setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask
                 | TextureFlags::Mask::Compute_mask)
        .setMipmapLevel(1)
        .setTextureType(TextureType::Texture2D);
    m_gameViewFrameTextures[i] = createTexture(gameViewFrameTextureInfo);
    Texture*       tex = accessTexture(m_gameViewFrameTextures[i]);
    CommandBuffer* cmd = getInstantCommandBuffer();
    cmd->begin();
    addImageBarrier(cmd->m_commandBuffer, tex, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_COMMON, 0,
                    1, false);
    cmd->flush(m_graphicsQueue);
    DescriptorSetCreateInfo descInfo{};
    descInfo.reset()
        .setName("gameViewFrameDesc")
        .setLayout(m_gameViewDescLayout)
        .texture(m_gameViewFrameTextures[i], 0);
    m_gameViewTextureDescs[i] = createDescriptorSet(descInfo);
  }
  TextureCreateInfo tc{nullptr,
                       {m_gameViewWidth, m_gameViewHeight, 1},
                       1,
                       VK_FORMAT_D32_SFLOAT,
                       TextureType::Enum::Texture2D,
                       "GameView_Depth_Texture",
                       TextureFlags::Mask::Default_mask};
  m_gameViewDepthTexture = createTexture(tc);
  for (int i = 0; i < m_gameViewImageCount; ++i) {
    FrameBufferCreateInfo gameViewFboInfo{};
    gameViewFboInfo.reset()
        .setName("gameViewFrameBuffer")
        .setExtent({m_gameViewWidth, m_gameViewHeight})
        .setDepthStencilTexture(m_gameViewDepthTexture)
        .addRenderTarget(m_gameViewFrameTextures[i])
        .setRenderPass(m_gameViewPass);
    m_gameViewFrameBuffers[i] = createFrameBuffer(gameViewFboInfo);
  }
  pass->m_depthTexture = m_gameViewDepthTexture;
  pass->m_outputTextures.clear();
  pass->m_outputTextures.push_back(m_gameViewFrameTextures[0]);
  pass->m_width = m_gameViewWidth;
  pass->m_height = m_gameViewHeight;
  vkDeviceWaitIdle(m_logicDevice);
}

static void transitionImageLayoutOverQueue(VkCommandBuffer cmdBuffer, VkImage image,
                                           VkFormat format, VkImageLayout oldLayout,
                                           VkImageLayout newLayout, bool isDepth, u32 srcQueueIdx,
                                           u32 dstQueueIdx) {
  VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageBarrier.image = image;
  imageBarrier.newLayout = newLayout;
  imageBarrier.oldLayout = oldLayout;
  imageBarrier.srcQueueFamilyIndex = srcQueueIdx;
  imageBarrier.dstQueueFamilyIndex = dstQueueIdx;
  imageBarrier.subresourceRange.aspectMask =
      isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  imageBarrier.subresourceRange.baseArrayLayer = 0;
  imageBarrier.subresourceRange.baseMipLevel = 0;
  imageBarrier.subresourceRange.layerCount = 1;
  imageBarrier.subresourceRange.levelCount = 1;

  VkPipelineStageFlags sourceFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    imageBarrier.srcAccessMask = VK_ACCESS_NONE;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    DG_ERROR("old layout or new layout is not supported!");
  }
  vkCmdPipelineBarrier(cmdBuffer, sourceFlag, dstFlag, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

static void vulkanCreateCubeTexture(DeviceContext* context, TextureCreateInfo& tc,
                                    TextureHandle& handle, Texture* tex) {
  tex->m_extent = tc.m_textureExtent;
  tex->m_format = tc.m_imageFormat;
  tex->m_mipLevel = tc.m_mipmapLevel;
  tex->m_name = tc.name;
  tex->m_type = tc.m_imageType;
  tex->m_handle = handle;
  tex->m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  Sampler* sampler = context->accessSampler(context->m_defaultSampler);
  tex->m_imageInfo.sampler = sampler->m_sampler;
  tex->m_usage = getImageUsage(tc);

  //create cube image
  VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = to_vk_image_type(TextureType::Enum::TextureCube);
  imageInfo.format = tc.m_imageFormat;
  imageInfo.mipLevels = tc.m_mipmapLevel;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.extent = tc.m_textureExtent;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.arrayLayers = 6;
  imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  VmaAllocationCreateInfo imageAllocInfo{};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  VkResult res = vmaCreateImage(context->m_vma, &imageInfo, &imageAllocInfo, &tex->m_image,
                                &tex->m_vma, nullptr);
  context->setResourceName(VK_OBJECT_TYPE_IMAGE, (u64) tex->m_image, tc.name.c_str());
  ktxTexture* ktxTexture = static_cast<::ktxTexture*>(tc.m_sourceData);

  VmaAllocationCreateInfo memory_info{};
  memory_info.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;
  memory_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  VmaAllocationInfo allocation_info{};

  VkBufferCreateInfo stagineInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagineInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagineInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  stagineInfo.size = ktxTexture_GetSize(ktxTexture);
  VkBuffer      stagingBuffer;
  VmaAllocation stagingAllocation;
  res = vmaCreateBuffer(context->m_vma, &stagineInfo, &memory_info, &stagingBuffer,
                        &stagingAllocation, nullptr);
  DGASSERT(res == VK_SUCCESS);

  void* destination_data;
  vmaMapMemory(context->m_vma, stagingAllocation, &destination_data);
  memcpy(destination_data, ktxTexture_GetData(ktxTexture), ktxTexture_GetSize(ktxTexture));
  vmaUnmapMemory(context->m_vma, stagingAllocation);

  auto                           cmd = context->getInstantCommandBuffer();
  std::vector<VkBufferImageCopy> bufferCopyResgions;
  uint32_t                       offset = 0;
  for (uint32_t face = 0; face < 6; ++face) {
    for (uint32_t level = 0; level < tc.m_mipmapLevel; ++level) {
      ktx_size_t     offset;
      KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
      DGASSERT(ret == KTX_SUCCESS)
      VkBufferImageCopy bufferCopyRegion{};
      bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      bufferCopyRegion.imageSubresource.baseArrayLayer = face;
      bufferCopyRegion.imageSubresource.layerCount = 1;
      bufferCopyRegion.imageSubresource.mipLevel = level;
      bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
      bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
      bufferCopyRegion.imageExtent.depth = 1;
      bufferCopyRegion.bufferOffset = offset;
      bufferCopyResgions.push_back(bufferCopyRegion);
    }
  }

  VkImageSubresourceRange subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subResource.baseArrayLayer = 0;
  subResource.baseMipLevel = 0;
  subResource.layerCount = 6;
  subResource.levelCount = tc.m_mipmapLevel;

  VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = tc.m_mipmapLevel;
  subresourceRange.layerCount = 6;
  context->addImageBarrier(cmd->m_commandBuffer, tex, tex->m_resourceState,
                           ResourceState::RESOURCE_STATE_COPY_DEST, 0, 1, false);
  vkCmdCopyBufferToImage(cmd->m_commandBuffer, stagingBuffer, tex->m_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyResgions.size(),
                         bufferCopyResgions.data());
  tex->m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  context->addImageBarrier(cmd->m_commandBuffer, tex, ResourceState::RESOURCE_STATE_COPY_DEST,
                           ResourceState ::RESOURCE_STATE_SHADER_RESOURCE, 0, 1, false);
  //        vkEndCommandBuffer(cmd->m_commandBuffer);
  //        VkSubmitInfo subInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  //        subInfo.commandBufferCount = 1;
  //        subInfo.pCommandBuffers = &cmd->m_commandBuffer;
  //        subInfo.signalSemaphoreCount = 0;
  //        subInfo.pSignalSemaphores = nullptr;
  //        subInfo.waitSemaphoreCount = 0;
  //        subInfo.pWaitSemaphores = nullptr;
  //        vkQueueSubmit(context->m_graphicsQueue, 1, &subInfo, VK_NULL_HANDLE);
  //        vkQueueWaitIdle(context->m_graphicsQueue);
  cmd->flush(context->m_graphicsQueue);
  //create image view
  VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  imageViewInfo.format = tc.m_imageFormat;
  imageViewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  imageViewInfo.subresourceRange.layerCount = 6;
  imageViewInfo.subresourceRange.levelCount = tc.m_mipmapLevel;
  imageViewInfo.image = tex->m_image;
  res = vkCreateImageView(context->m_logicDevice, &imageViewInfo, nullptr,
                          &tex->m_imageInfo.imageView);
  DGASSERT(res == VK_SUCCESS)
  vmaDestroyBuffer(context->m_vma, stagingBuffer, stagingAllocation);
}

void uploadTextureData(Texture* texture, void* uploadData, DeviceContext* context) {
  VkBufferCreateInfo stagingBufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkDeviceSize imageSize = texture->m_extent.width * texture->m_extent.height
      * format_to_pixel_buffer_size(texture->m_format);
  stagingBufferInfo.size = imageSize;

  VmaAllocationCreateInfo allocCreateInfo{};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  allocCreateInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT;

  VkBuffer          stagingBuffer;
  VmaAllocation     stagingAllocation;
  VmaAllocationInfo vmaInfo;
  VkResult          res = vmaCreateBuffer(context->m_vma, &stagingBufferInfo, &allocCreateInfo,
                                          &stagingBuffer, &stagingAllocation, &vmaInfo);
  DGASSERT(res == VK_SUCCESS);
  void* data;
  vmaMapMemory(context->m_vma, stagingAllocation, &data);
  memcpy(data, uploadData, imageSize);
  vmaUnmapMemory(context->m_vma, stagingAllocation);

  VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  CommandBuffer* tempCommandBuffer = context->getInstantCommandBuffer();
  vkBeginCommandBuffer(tempCommandBuffer->m_commandBuffer, &cmdBeginInfo);
  VkBufferImageCopy bufferImagecp;
  bufferImagecp.bufferOffset = 0;
  bufferImagecp.bufferImageHeight = 0;
  bufferImagecp.bufferRowLength = 0;
  bufferImagecp.imageExtent = texture->m_extent;
  bufferImagecp.imageOffset = {0, 0, 0};
  bufferImagecp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferImagecp.imageSubresource.baseArrayLayer = 0;
  bufferImagecp.imageSubresource.layerCount = 1;
  bufferImagecp.imageSubresource.mipLevel = 0;

  context->addImageBarrier(tempCommandBuffer->m_commandBuffer, texture, texture->m_resourceState,
                           ResourceState::RESOURCE_STATE_COPY_DEST, 0, 1, false);
  vkCmdCopyBufferToImage(tempCommandBuffer->m_commandBuffer, stagingBuffer, texture->m_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImagecp);
  //addImageBarrier(tempCommandBuffer->m_commandBuffer, texture, ResourceState::RESOURCE_STATE_COPY_DEST,ResourceState::RESOURCE_STATE_SHADER_RESOURCE,  0,1,false);
  if (texture->m_mipLevel > 1) {
    context->addImageBarrier(tempCommandBuffer->m_commandBuffer, texture,
                             ResourceState ::RESOURCE_STATE_COPY_DEST,
                             ResourceState ::RESOURCE_STATE_COPY_SOURCE, 0, 1, false);
  }

  int w = texture->m_extent.width;
  int h = texture->m_extent.height;

  for (int mipIdx = 1; mipIdx < texture->m_mipLevel; ++mipIdx) {
    context->addImageBarrier(tempCommandBuffer->m_commandBuffer, texture,
                             ResourceState ::RESOURCE_STATE_UNDEFINED,
                             ResourceState ::RESOURCE_STATE_COPY_DEST, mipIdx, 1, false);
    VkImageBlit blit_region{};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = mipIdx - 1;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcOffsets[0] = {0, 0, 0};
    blit_region.srcOffsets[1] = {w, h, 1};
    w /= 2;
    h /= 2;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = mipIdx;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstOffsets[0] = {0, 0, 0};
    blit_region.dstOffsets[1] = {w, h, 1};
    vkCmdBlitImage(tempCommandBuffer->m_commandBuffer, texture->m_image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->m_image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_LINEAR);
    context->addImageBarrier(tempCommandBuffer->m_commandBuffer, texture,
                             ResourceState::RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COPY_SOURCE,
                             mipIdx, 1, false);
  }

  context->addImageBarrier(tempCommandBuffer->m_commandBuffer, texture,
                           (texture->m_mipLevel > 1) ? ResourceState::RESOURCE_STATE_COPY_SOURCE
                                                     : ResourceState::RESOURCE_STATE_COPY_DEST,
                           ResourceState::RESOURCE_STATE_SHADER_RESOURCE, 0, texture->m_mipLevel,
                           false);
  //        vkEndCommandBuffer(tempCommandBuffer->m_commandBuffer);
  //        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  //        submitInfo.commandBufferCount = 1;
  //        submitInfo.pCommandBuffers = &tempCommandBuffer->m_commandBuffer;
  //        res = vkQueueSubmit(context->m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  //        DGASSERT(res == VK_SUCCESS);
  //        vkQueueWaitIdle(context->m_graphicsQueue);
  tempCommandBuffer->flush(context->m_graphicsQueue);
  vmaDestroyBuffer(context->m_vma, stagingBuffer, stagingAllocation);
  vkResetCommandBuffer(tempCommandBuffer->m_commandBuffer,
                       VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  texture->m_resourceState = RESOURCE_STATE_SHADER_RESOURCE;
  texture->m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void transitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkFormat format,
                                  VkImageLayout oldLayout, VkImageLayout newLayout,
                                  VkImageSubresourceRange subRange) {
  VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageBarrier.image = image;
  imageBarrier.newLayout = newLayout;
  imageBarrier.oldLayout = oldLayout;
  imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarrier.subresourceRange = subRange;

  VkPipelineStageFlags sourceFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    imageBarrier.srcAccessMask = VK_ACCESS_NONE;
    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
             && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    DG_ERROR("old layout or new layout is not supported!");
  }
  vkCmdPipelineBarrier(cmdBuffer, sourceFlag, dstFlag, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

TextureHandle DeviceContext::createTexture(TextureCreateInfo& tc) {
  TextureHandle handle = {m_textures.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  Texture* texture = accessTexture(handle);
  if (tc.m_imageType == TextureType::TextureCube) {
    vulkanCreateCubeTexture(this, tc, handle, texture);
    return handle;
  }
  vulkanCreateTexture(this, tc, handle, texture);
  if (tc.m_sourceData) { uploadTextureData(texture, tc.m_sourceData, this); }
  return handle;
}

static void vulkanCreateFrameBuffers(DeviceContext* context, FrameBuffer* framebuffer) {
  RenderPass*             renderpass = context->accessRenderPass(framebuffer->m_renderPassHandle);
  auto&                   outputTextures = framebuffer->m_colorAttachments;
  auto&                   depthStencilTex = framebuffer->m_depthStencilAttachment;
  VkFramebufferCreateInfo fboInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  fboInfo.renderPass = renderpass->m_renderPass;
  fboInfo.attachmentCount =
      framebuffer->m_numRenderTargets + (depthStencilTex.index == k_invalid_index ? 0 : 1);
  std::vector<VkImageView> attchs;
  for (int i = 0; i < framebuffer->m_numRenderTargets; ++i) {
    TextureHandle handle = outputTextures[i];
    Texture*      tex = context->accessTexture(handle);
    attchs.push_back(tex->m_imageInfo.imageView);
  }
  if (framebuffer->m_depthStencilAttachment.index != k_invalid_index) {
    Texture* depth = context->accessTexture(depthStencilTex);
    attchs.push_back(depth->m_imageInfo.imageView);
  }
  fboInfo.pAttachments = attchs.data();
  fboInfo.layers = 1;
  fboInfo.width = framebuffer->m_width;
  fboInfo.height = framebuffer->m_height;

  vkCreateFramebuffer(context->m_logicDevice, &fboInfo, nullptr, &framebuffer->m_frameBuffer);
  context->setResourceName(VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t) framebuffer->m_frameBuffer,
                           framebuffer->name.c_str());
}

//自己实现的
void DeviceContext::resizeOutputTextures(FrameBufferHandle frameBuffer, u32 width, u32 height) {
  FrameBuffer* fboPtr = accessFrameBuffer(frameBuffer);
  if (!fboPtr) { DG_ERROR("Can not get renderpass"); }
  if (width == fboPtr->m_width && height == fboPtr->m_height) {
    return;
  } else if (width != 0 && height != 0) {
    u32 newWidth = width * fboPtr->m_scalex;
    u32 newHeight = height * fboPtr->m_scaley;
    for (u32 i = 0; i < fboPtr->m_numRenderTargets; ++i) {
      TextureHandle rtHandle = fboPtr->m_colorAttachments[i];
      Texture*      rtTexture = accessTexture(rtHandle);
      TextureHandle deleteHandle = {m_textures.obtainResource()};
      Texture*      deleteTexture = accessTexture(deleteHandle);
      vulkanResizeTexture(this, rtTexture, deleteTexture, newWidth, newHeight, 1);
      deleteTexture->m_handle = deleteHandle;
      DestroyTexture(deleteHandle);
    }
    if (fboPtr->m_depthStencilAttachment.index != k_invalid_index) {
      TextureHandle depthHandle = fboPtr->m_depthStencilAttachment;
      Texture*      depthTexture = accessTexture(depthHandle);
      TextureHandle deletehandle = {m_textures.obtainResource()};
      Texture*      deleteTexture = accessTexture(deletehandle);
      vulkanResizeTexture(this, depthTexture, deleteTexture, newWidth, newHeight, 1);
      deleteTexture->m_handle = deletehandle;
      DestroyTexture(deletehandle);
    }
    FrameBufferHandle deleteHandle = {m_frameBuffers.obtainResource()};
    FrameBuffer*      deleteFramebuffer = accessFrameBuffer(deleteHandle);
    deleteFramebuffer->m_frameBuffer = fboPtr->m_frameBuffer;
    deleteFramebuffer->m_numRenderTargets = 0;
    DestroyFrameBuffer(deleteHandle);
    fboPtr->m_width = newWidth;
    fboPtr->m_height = newHeight;
    vulkanCreateFrameBuffers(this, fboPtr);
  } else {
    DG_ERROR("Attempt to resize renderpass with an invalid extent");
  }
}

RenderPassHandle DeviceContext::createRenderPass(RenderPassCreateInfo& ri) {
  RenderPassHandle handle = {m_renderPasses.obtainResource()};
  if (handle.index == k_invalid_index) return handle;

  RenderPass* renderPass = accessRenderPass(handle);
  renderPass->m_scalex = ri.m_scalex;
  renderPass->m_scaley = ri.m_scaley;
  renderPass->name = ri.name;
  renderPass->m_type = ri.m_renderPassType;
  renderPass->m_outputTextures = ri.m_outputTextures;
  renderPass->m_depthTexture = ri.m_depthTexture;
  renderPass->m_dispatchx = 0;
  renderPass->m_dispatchy = 0;
  renderPass->m_dispatchz = 0;
  renderPass->m_resize = ri.resize;
  renderPass->m_frameBuffers = {};
  renderPass->m_width = m_swapChainWidth;
  renderPass->m_height = m_swapChainHeight;
  u32 c = 0;
  for (; c < ri.m_outputTextures.size(); ++c) {
    Texture* texture = accessTexture(ri.m_outputTextures[c]);
    renderPass->m_width = texture->m_extent.width;
    renderPass->m_height = texture->m_extent.height;
    renderPass->m_outputTextures[c] = ri.m_outputTextures[c];
  }
  renderPass->m_depthTexture = ri.m_depthTexture;
  renderPass->m_numRenderTarget = ri.m_rtNums;
  switch (ri.m_renderPassType) {
    case RenderPassType::Enum::SwapChain: {
      vulkanCreateSwapChainPass(this, ri, renderPass);
      break;
    }
    case RenderPassType::Enum::Compute: {
      break;
    }
    case RenderPassType::Enum::Geometry: {
      renderPass->m_renderPass = vulkanCreateRenderPass(this, ri);
      //vulkanCreateFrameBuffers(this,renderPass->m_frameBuffer);
      break;
    }
  }
  return handle;
}

FrameBufferHandle DeviceContext::createFrameBuffer(FrameBufferCreateInfo& fcf) {
  FrameBufferHandle handle = {m_frameBuffers.obtainResource()};
  FrameBuffer*      fbo = accessFrameBuffer(handle);
  if (fcf.m_renderPassHandle.index != k_invalid_index) {
    fbo->m_renderPassHandle = fcf.m_renderPassHandle;
  } else {
    DG_ERROR("A valid renderPass handle must be set in FrameBufferCreateInfo");
    exit(-1);
  }
  fbo->m_depthStencilAttachment = fcf.m_depthStencilTexture;
  fbo->m_height = fcf.m_height;
  fbo->m_width = fcf.m_width;
  fbo->m_scalex = fcf.m_scaleX;
  fbo->m_scaley = fcf.m_scaleY;
  fbo->m_resize = fcf.resize;
  fbo->m_numRenderTargets = fcf.m_numRenderTargets;
  for (int i = 0; i < fcf.m_numRenderTargets; ++i) {
    fbo->m_colorAttachments[i] = fcf.m_outputTextures[i];
  }
  RenderPass* renderpass = this->accessRenderPass(fcf.m_renderPassHandle);
  renderpass->m_frameBuffers.push_back(handle);
  vulkanCreateFrameBuffers(this, fbo);
  return handle;
}

DescriptorSetLayoutHandle
DeviceContext::createDescriptorSetLayout(DescriptorSetLayoutCreateInfo& dcinfo) {
  DescriptorSetLayoutHandle handle = {m_descriptorSetLayouts.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  DescriptorSetLayout* setLayout = accessDescriptorSetLayout(handle);
  setLayout->m_numBindings = dcinfo.m_bindingNum;
  setLayout->m_bindings.resize(dcinfo.m_bindingNum);
  setLayout->m_vkBindings.resize(dcinfo.m_bindingNum);
  for (int i = 0; i < dcinfo.m_bindingNum; ++i) {
    setLayout->m_bindings[i].type = dcinfo.m_bindings[i].m_type;
    setLayout->m_vkBindings[i].descriptorType = dcinfo.m_bindings[i].m_type;
    setLayout->m_bindings[i].start = dcinfo.m_bindings[i].m_start;
    setLayout->m_vkBindings[i].binding = dcinfo.m_bindings[i].m_start;
    setLayout->m_bindings[i].count = dcinfo.m_bindings[i].m_count;
    setLayout->m_vkBindings[i].descriptorCount = dcinfo.m_bindings[i].m_count;
    setLayout->m_bindings[i].stageFlags = dcinfo.m_bindings[i].stageFlags;
    setLayout->m_vkBindings[i].stageFlags = dcinfo.m_bindings[i].stageFlags;
  }
  setLayout->m_setIndex = dcinfo.m_setIndex;
  setLayout->bindless = dcinfo.bindless ? dcinfo.bindless : false;
  for (int i = 0; i < dcinfo.m_bindingNum; ++i) {
    DescriptorBinding&                            tempBinding = setLayout->m_bindings[i];
    const DescriptorSetLayoutCreateInfo::Binding& input_binding = dcinfo.m_bindings[i];
    tempBinding.start = input_binding.m_start;
    tempBinding.count = 1;
    tempBinding.name = input_binding.name;
    tempBinding.type = input_binding.m_type;

    if (dcinfo.bindless
        && (tempBinding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
            || tempBinding.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)) {
      continue;
    }

    VkDescriptorSetLayoutBinding& vkBinding = setLayout->m_vkBindings[i];
    vkBinding.binding = tempBinding.start;
    vkBinding.descriptorCount = 1;
    vkBinding.descriptorType = input_binding.m_type;

    vkBinding.stageFlags = tempBinding.stageFlags;
    vkBinding.pImmutableSamplers = nullptr;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = dcinfo.m_bindingNum;
  layoutInfo.pBindings = setLayout->m_vkBindings.data();
  if (dcinfo.bindless) {
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
        | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;//|VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
    VkDescriptorBindingFlags binding_flags[16];
    for (u32 r = 0; r < dcinfo.m_bindingNum; ++r) { binding_flags[r] = bindless_flags; }
    VkDescriptorSetLayoutBindingFlagsCreateInfo extended_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, nullptr};
    extended_info.bindingCount = dcinfo.m_bindingNum;
    extended_info.pBindingFlags = binding_flags;
    layoutInfo.pNext = &extended_info;
    vkCreateDescriptorSetLayout(m_logicDevice, &layoutInfo, nullptr,
                                &setLayout->m_descriptorSetLayout);
  } else {
    vkCreateDescriptorSetLayout(m_logicDevice, &layoutInfo, nullptr,
                                &setLayout->m_descriptorSetLayout);
  }
  setResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t) setLayout->m_descriptorSetLayout,
                  dcinfo.name.c_str());
  setLayout->m_handle = handle;
  return handle;
}

VkWriteDescriptorSetAccelerationStructureKHR asInfo{
    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
static void vk_fill_write_descriptor_sets(
    DeviceContext* context, const DescriptorSetLayout* setLayout, DescriptorSet* descriptor_set,
    VkWriteDescriptorSet* descriptorWrite, VkDescriptorBufferInfo* bufferInfo,
    VkDescriptorImageInfo* imageInfo, VkSampler* sampler,
    const u32&
        num_resources) {//, const std::vector<ResourceHandle>& resources, std::vector<SamplerHandle>& samplers, const std::vector<u32>& bindings) {
  auto bindings = descriptor_set->m_bindings;
  auto resources = descriptor_set->m_resources;
  auto samplers = descriptor_set->m_samples;
  for (int i = 0; i < num_resources; ++i) {
    u32   layoutBindingIndex = bindings[i];
    auto& binding = setLayout->m_vkBindings[layoutBindingIndex];

    descriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[i].dstSet = descriptor_set->m_vkdescriptorSet;
    descriptorWrite[i].dstBinding = binding.binding;
    descriptorWrite[i].dstArrayElement = 0;
    descriptorWrite[i].descriptorCount = binding.descriptorCount;
    descriptorWrite[i].pNext = nullptr;
    switch (binding.descriptorType) {
      case (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER): {
        Texture* texture;
        auto&    iinfo = imageInfo[i];
        if (resources[i] == k_invalid_index) {
          texture = context->accessTexture(context->m_DummyTexture);
        } else {
          texture = context->accessTexture({resources[i]});
        }
        iinfo.sampler = *sampler;
        if (texture->m_imageInfo.sampler != VK_NULL_HANDLE) {
          iinfo.sampler = texture->m_imageInfo.sampler;
        } else if (samplers[i].index != k_invalid_index) {
          Sampler* samp = context->accessSampler(samplers[i]);
          iinfo.sampler = samp->m_sampler;
        } else {
          samplers[i] = context->m_defaultSampler;
        }
        iinfo.imageLayout = TextureFormat::has_depth_or_stencil(texture->m_format)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            : texture->m_imageInfo.imageLayout;
        iinfo.imageView = texture->m_imageInfo.imageView;
        descriptorWrite[i].descriptorType = binding.descriptorType;
        descriptorWrite[i].pImageInfo = &iinfo;
        break;
      }
      case (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE): {
        descriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        Texture* texture = context->accessTexture({resources[i]});
        imageInfo[i].sampler = *sampler;
        imageInfo[i].imageLayout = texture->m_imageInfo.imageLayout;
        imageInfo[i].imageView = texture->m_imageInfo.imageView;
        descriptorWrite[i].pImageInfo = &imageInfo[i];
        break;
      }
      case (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER): {
        descriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        Buffer* buffer = context->accessBuffer({resources[i]});

        if (buffer->m_parentHandle.index != k_invalid_index) {
          Buffer* parentBuffer = context->accessBuffer(buffer->m_parentHandle);
          bufferInfo[i].buffer = parentBuffer->m_buffer;
        } else {
          bufferInfo[i].buffer = buffer->m_buffer;
        }

        bufferInfo[i].offset = 0;
        bufferInfo[i].range = buffer->m_bufferSize;
        descriptorWrite[i].pBufferInfo = &bufferInfo[i];
        break;
      }
      case (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER): {
        descriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        Buffer* buffer = context->accessBuffer({resources[i]});
        if (buffer->m_parentHandle.index != k_invalid_index) {
          Buffer* parentBuffer = context->accessBuffer(buffer->m_parentHandle);
          bufferInfo[i].buffer = parentBuffer->m_buffer;
        } else {
          bufferInfo[i].buffer = buffer->m_buffer;
        }

        bufferInfo[i].offset = 0;
        bufferInfo[i].range = buffer->m_bufferSize;
        descriptorWrite[i].pBufferInfo = &bufferInfo[i];
        break;
      }
      case (VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR): {
        descriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        asInfo.accelerationStructureCount = 1;
        asInfo.pAccelerationStructures = &descriptor_set->m_accel.accel;
        descriptorWrite[i].pNext = &asInfo;
        break;
      }
      default: {
        DG_ERROR("Unsupported descriptor type: {}", binding.descriptorType);
        break;
      }
    }
  }
}

DescriptorSetHandle DeviceContext::createDescriptorSet(DescriptorSetCreateInfo& dcinfo) {
  DescriptorSetHandle handle = {m_descriptorSets.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  DescriptorSet*              set = accessDescriptorSet(handle);
  const DescriptorSetLayout*  layout = accessDescriptorSetLayout(dcinfo.m_layout);
  VkDescriptorSetAllocateInfo setAllocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  setAllocInfo.descriptorPool = m_descriptorPool;
  setAllocInfo.descriptorSetCount = 1;
  setAllocInfo.pSetLayouts = &layout->m_descriptorSetLayout;
  if (layout->bindless) {
    VkDescriptorSetVariableDescriptorCountAllocateInfo count_info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    u32 max_binding = k_max_bindless_resource - 1;
    count_info.descriptorSetCount = 1;
    count_info.pDescriptorCounts = &max_binding;
    setAllocInfo.descriptorPool = m_bindlessDescriptorPool;
    setAllocInfo.pNext = &count_info;
    set->m_parentPool = m_bindlessDescriptorPool;
    VkResult res = vkAllocateDescriptorSets(m_logicDevice, &setAllocInfo, &set->m_vkdescriptorSet);
    DGASSERT(res == VK_SUCCESS);
  } else {
    set->m_parentPool = m_descriptorPool;
    VkResult res = vkAllocateDescriptorSets(m_logicDevice, &setAllocInfo, &set->m_vkdescriptorSet);
    DGASSERT(res == VK_SUCCESS)
  }

  set->m_resources = dcinfo.m_resources;
  set->m_samples = dcinfo.m_samplers;
  set->m_bindings = dcinfo.m_bindings;
  set->m_resourceNums = dcinfo.m_resourceNums;
  set->m_layout = layout;
  set->m_accel = dcinfo.m_accel;

  Sampler*               sampler = accessSampler(m_defaultSampler);
  const u32              numResources = dcinfo.m_resourceNums;
  VkWriteDescriptorSet   descriptor_write[32];
  VkDescriptorBufferInfo buffer_info[32];
  VkDescriptorImageInfo  image_info[32];
  vk_fill_write_descriptor_sets(this, layout, set, descriptor_write, buffer_info, image_info,
                                &sampler->m_sampler, numResources);

  for (int i = 0; i < numResources; ++i) {
    set->m_resources[i] = dcinfo.m_resources[i];
    set->m_bindings[i] = dcinfo.m_bindings[i];
    set->m_samples[i] = dcinfo.m_samplers[i];
  }
  vkUpdateDescriptorSets(m_logicDevice, numResources, descriptor_write, 0, nullptr);
  setResourceName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) set->m_vkdescriptorSet,
                  dcinfo.name.c_str());
  return handle;
}

ShaderStateHandle DeviceContext::createShaderState(ShaderStateCreation& shaderInfo) {
  ShaderStateHandle handle = {m_shaderStates.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  if (shaderInfo.m_stageCount == 0 || shaderInfo.m_stages.empty()) {
    DG_ERROR("Shader {} does not contain shader stages.", shaderInfo.name);
  }
  ShaderState* state = accessShaderState(handle);
  state->m_pipelineType = ShaderState::eGraphics;
  state->m_activeShaders = 0;

  bool is_Fail = false;
  sort(shaderInfo.m_stages.begin(), shaderInfo.m_stages.end(), [](ShaderStage& a, ShaderStage& b) {
    return a.m_rayTracingShaderType < b.m_rayTracingShaderType;
  });

  std::vector<u32> rayTracingShaderCounter(ShaderStage::eCount);
  for (int i = 0; i < shaderInfo.m_stageCount; ++i) {
    const ShaderStage& stage = shaderInfo.m_stages[i];
    if (stage.m_rayTracingShaderType != ShaderStage::eCount) {
      rayTracingShaderCounter[stage.m_rayTracingShaderType]++;
    }
    if (stage.m_type == VK_SHADER_STAGE_COMPUTE_BIT) {
      state->m_pipelineType = ShaderState::eCompute;
    }
    VkShaderModuleCreateInfo  moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ShaderCompiler::SpvObject spvObject;
    if (shaderInfo.m_spvInput) {
      moduleInfo.codeSize = stage.m_codeSize;
      moduleInfo.pCode = (uint32_t*) stage.m_code;
    } else {
      spvObject = ShaderCompiler::compileShader(stage.m_shaderPath);
      moduleInfo.codeSize = spvObject.binarySize;
      moduleInfo.pCode = (uint32_t*) spvObject.spvData.data();
    }
    VkPipelineShaderStageCreateInfo& shaderStageInfo = state->m_shaderStageInfo[i];
    shaderStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStageInfo.stage = stage.m_type;
    shaderStageInfo.pName = "main";
    if (vkCreateShaderModule(m_logicDevice, &moduleInfo, nullptr,
                             &state->m_shaderStageInfo[i].module)
        != VK_SUCCESS) {
      is_Fail = true;
      break;
    }
    setResourceName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t) state->m_shaderStageInfo[i].module,
                    shaderInfo.name.c_str());

    VkRayTracingShaderGroupCreateInfoKHR group{
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;
    switch (stage.m_type) {
      case VK_SHADER_STAGE_RAYGEN_BIT_KHR: {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = i;
        state->m_shaderGroupInfo.push_back(group);
        state->m_pipelineType = ShaderState::eRayTracing;
        break;
      }
      case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        group.anyHitShader = i;
        state->m_shaderGroupInfo.push_back(group);
        state->m_pipelineType = ShaderState::eRayTracing;
        break;
      }
      case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        group.closestHitShader = i;
        state->m_shaderGroupInfo.push_back(group);
        state->m_pipelineType = ShaderState::eRayTracing;
        break;
      }
      case VK_SHADER_STAGE_MISS_BIT_KHR: {
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = i;
        state->m_shaderGroupInfo.push_back(group);
        state->m_pipelineType = ShaderState::eRayTracing;
        break;
      }
    }
  }

  state->m_rayAnyHitShaderNum = rayTracingShaderCounter[ShaderStage::eRayAHit];
  state->m_rayGenShaderNum = rayTracingShaderCounter[ShaderStage::eRayGen];
  state->m_rayMissShaderNum = rayTracingShaderCounter[ShaderStage::eRayMiss];
  state->m_rayClosestHitShaderNum = rayTracingShaderCounter[ShaderStage::eRayCHit];
  state->m_rayCallableShaderNum = rayTracingShaderCounter[ShaderStage::eRayCallable];

  if (is_Fail) {
    DestroyShaderState(handle);
    handle.index = k_invalid_index;
    DG_ERROR("ERROR in creation of shader{}. Dumping all informations.", shaderInfo.name);
    for (int i = 0; i < shaderInfo.m_stageCount; ++i) {
      auto& stage = shaderInfo.m_stages[i];
      DG_INFO("{} : {}", stage.m_type, stage.m_code);
    }
  }
  state->m_activeShaders = shaderInfo.m_stageCount;
  state->m_name = shaderInfo.name;
  return handle;
}

PipelineHandle DeviceContext::createPipeline(PipelineCreateInfo& pipelineInfo) {
  PipelineHandle handle = {m_pipelines.obtainResource()};
  if (handle.index == k_invalid_index) return handle;
  ShaderStateHandle shaderStateHandle = createShaderState(pipelineInfo.m_shaderState);
  if (shaderStateHandle.index == k_invalid_index) {
    m_pipelines.releaseResource(handle.index);
    handle.index = k_invalid_index;
    return handle;
  }
  Pipeline*    pipeline = accessPipeline(handle);
  ShaderState* shaderState = accessShaderState(shaderStateHandle);
  pipeline->m_shaderState = shaderStateHandle;
  VkDescriptorSetLayout dSetLayouts[k_max_descriptor_set_layouts];
  for (int i = 0; i < pipelineInfo.m_numActivateLayouts; ++i) {
    pipeline->m_DescriptorSetLayout[i] = accessDescriptorSetLayout(pipelineInfo.m_descLayout[i]);
    dSetLayouts[i] = pipeline->m_DescriptorSetLayout[i]->m_descriptorSetLayout;
  }

  VkPipelineLayoutCreateInfo pipLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipLayoutInfo.setLayoutCount = pipelineInfo.m_numActivateLayouts;
  pipLayoutInfo.pSetLayouts = dSetLayouts;
  pipLayoutInfo.pushConstantRangeCount = pipelineInfo.m_pushConstants.size();
  pipLayoutInfo.pPushConstantRanges = pipelineInfo.m_pushConstants.data();

  VkPipelineLayout pipelineLayout;
  DGASSERT(vkCreatePipelineLayout(m_logicDevice, &pipLayoutInfo, nullptr, &pipelineLayout)
           == VK_SUCCESS);

  pipeline->m_pipelineLayout = pipelineLayout;
  pipeline->m_activeLayouts = pipelineInfo.m_numActivateLayouts;

  if (shaderState->m_pipelineType == ShaderState::PipelineType::eGraphics) {
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stageCount = shaderState->m_activeShaders;
    pipelineCreateInfo.pStages = shaderState->m_shaderStageInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexAttributeDescriptionCount = pipelineInfo.m_vertexInput.Attrib.size();
    VkVertexInputAttributeDescription attribDescrib[8];
    VkVertexInputBindingDescription   bindingDescrib[8];
    if (pipelineInfo.m_vertexInput.Attrib.size()) {
      for (int i = 0; i < pipelineInfo.m_vertexInput.Attrib.size(); ++i) {
        attribDescrib[i] = pipelineInfo.m_vertexInput.Attrib[i];
      }
      vertexInputInfo.pVertexAttributeDescriptions = attribDescrib;
      vertexInputInfo.vertexBindingDescriptionCount = 1;
      bindingDescrib[0] = pipelineInfo.m_vertexInput.Binding;
      vertexInputInfo.pVertexBindingDescriptions = bindingDescrib;
    }
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    pipelineCreateInfo.pInputAssemblyState = &input_assembly;

    VkPipelineColorBlendAttachmentState blendState[8];
    if (pipelineInfo.m_BlendState.m_activeStates) {
      for (int i = 0; i < pipelineInfo.m_BlendState.m_activeStates; ++i) {
        auto& blend = blendState[i];
        auto& infoBlend = pipelineInfo.m_BlendState.m_blendStates[i];
        blend.alphaBlendOp = infoBlend.m_alphaOperation;
        blend.blendEnable = infoBlend.m_blendEnabled;
        blend.colorBlendOp = infoBlend.m_colorBlendOp;
        blend.srcColorBlendFactor = infoBlend.m_sourceColor;
        blend.dstColorBlendFactor = infoBlend.m_destinationColor;
        blend.srcAlphaBlendFactor = infoBlend.m_sourceAlpha;
        blend.dstAlphaBlendFactor = infoBlend.m_destinationAlpha;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (infoBlend.m_separateBlend) {
          blend.srcAlphaBlendFactor = infoBlend.m_sourceAlpha;
          blend.dstAlphaBlendFactor = infoBlend.m_destinationAlpha;
          blend.alphaBlendOp = infoBlend.m_alphaOperation;
        } else {
          blend.srcAlphaBlendFactor = infoBlend.m_sourceColor;
          blend.dstAlphaBlendFactor = infoBlend.m_destinationColor;
          blend.alphaBlendOp = infoBlend.m_colorBlendOp;
        }
      }
    } else {
      blendState[0] = {};
      blendState[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      blendState[0].blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo blendCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendCreateInfo.logicOpEnable = VK_FALSE;
    blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    blendCreateInfo.attachmentCount =
        pipelineInfo.m_BlendState.m_activeStates ? pipelineInfo.m_BlendState.m_activeStates : 1;
    blendCreateInfo.pAttachments = blendState;
    blendCreateInfo.blendConstants[0] = 0.0f;
    blendCreateInfo.blendConstants[1] = 0.0f;
    blendCreateInfo.blendConstants[2] = 0.0f;
    blendCreateInfo.blendConstants[3] = 0.0f;
    pipelineCreateInfo.pColorBlendState = &blendCreateInfo;
    //depth stencil
    VkPipelineDepthStencilStateCreateInfo depthCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthCreateInfo.depthTestEnable = pipelineInfo.m_depthStencil.m_depthEnable;
    depthCreateInfo.depthWriteEnable = pipelineInfo.m_depthStencil.m_depthWriteEnable;
    depthCreateInfo.depthCompareOp = pipelineInfo.m_depthStencil.m_depthCompare;
    if (pipelineInfo.m_depthStencil.m_stencilEnable) { DGASSERT(false); }
    pipelineCreateInfo.pDepthStencilState = &depthCreateInfo;

    //Multisample
    VkPipelineMultisampleStateCreateInfo MSCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    MSCreateInfo.sampleShadingEnable = VK_FALSE;
    MSCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    MSCreateInfo.minSampleShading = 1.0f;
    MSCreateInfo.pSampleMask = nullptr;
    MSCreateInfo.alphaToOneEnable = VK_FALSE;
    MSCreateInfo.alphaToCoverageEnable = VK_FALSE;
    pipelineCreateInfo.pMultisampleState = &MSCreateInfo;

    //Raster
    VkPipelineRasterizationStateCreateInfo RSCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    auto& rs = pipelineInfo.m_rasterization;
    RSCreateInfo.rasterizerDiscardEnable = false;
    RSCreateInfo.cullMode = rs.m_cullMode;
    RSCreateInfo.frontFace = rs.m_front;
    RSCreateInfo.depthClampEnable = VK_FALSE;
    RSCreateInfo.depthBiasEnable = VK_FALSE;
    RSCreateInfo.lineWidth = 1.0f;
    RSCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    RSCreateInfo.depthBiasConstantFactor = 0.0f;
    RSCreateInfo.depthBiasSlopeFactor = 0.0f;
    RSCreateInfo.depthBiasSlopeFactor = 0.0f;
    pipelineCreateInfo.pRasterizationState = &RSCreateInfo;

    VkPipelineViewportStateCreateInfo viewportInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportInfo.viewportCount = 1;
    //viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    //viewportInfo.pScissors = &scissor;
    pipelineCreateInfo.pViewportState = &viewportInfo;
    pipelineCreateInfo.renderPass = accessRenderPass(pipelineInfo.m_renderPassHandle)->m_renderPass;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicInfo.dynamicStateCount = 3;
    dynamicInfo.pDynamicStates = dynamicStates;
    pipelineCreateInfo.pDynamicState = &dynamicInfo;

    DGASSERT(vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                                       nullptr, &pipeline->m_pipeline)
             == VK_SUCCESS);
    pipeline->m_pipelineType = shaderState->m_pipelineType;
    pipeline->m_pipelineHandle = handle;
    pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pipeline->m_blendStateCrt = pipelineInfo.m_BlendState;
    pipeline->m_rasterizationCrt = pipelineInfo.m_rasterization;
    pipeline->m_depthStencilCrt = pipelineInfo.m_depthStencil;
    pipeline->m_shaderStateCrt = pipelineInfo.m_shaderState;
    pipeline->m_blendStateCrt = pipelineInfo.m_BlendState;
    pipeline->m_vertexInputCrt = pipelineInfo.m_vertexInput;
  } else if (shaderState->m_pipelineType == ShaderState::eRayTracing) {
    pipeline->m_pipelineType = shaderState->m_pipelineType;
    VkRayTracingPipelineCreateInfoKHR rtPipelineInfo{
        VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    rtPipelineInfo.stageCount = shaderState->m_activeShaders;
    rtPipelineInfo.pStages = shaderState->m_shaderStageInfo;
    rtPipelineInfo.groupCount = shaderState->m_shaderGroupInfo.size();
    rtPipelineInfo.pGroups = shaderState->m_shaderGroupInfo.data();
    rtPipelineInfo.maxPipelineRayRecursionDepth = pipelineInfo.m_rayBoundNum;
    rtPipelineInfo.pLibraryInfo = nullptr;
    rtPipelineInfo.pLibraryInterface = nullptr;
    rtPipelineInfo.pDynamicState = nullptr;
    rtPipelineInfo.layout = pipelineLayout;
    VkResult res = vkCreateRayTracingPipelinesKHR(m_logicDevice, {}, {}, 1, &rtPipelineInfo,
                                                  nullptr, &pipeline->m_pipeline);
    DGASSERT(res == VK_SUCCESS);
    pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

    //code follow is refered from https://github.com/damnkk/vk_raytracing_tutorial_KHR/blob/master/ray_tracing__simple/main.cpp
    //add the full shader groupcount
    auto groupHandleCount = shaderState->m_rayAnyHitShaderNum + shaderState->m_rayCallableShaderNum
        + shaderState->m_rayClosestHitShaderNum + shaderState->m_rayGenShaderNum
        + shaderState->m_rayMissShaderNum;
    //perHandleSize is used to calculate the handle data size without align, in this func ,it's "dataSize" arg.
    uint32_t perHandleSize = m_rayTracingPipelineProperties.shaderGroupHandleSize;
    uint32_t perHandleSizeAligned =
        align_up(perHandleSize, m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
    //calcu every binding table's stride and size, the "ray Gen" one is special,only have one for most situation,so it's stride = size, and aligned with Group Base Alignment
    pipeline->m_rayGenTable.stride =
        align_up(perHandleSizeAligned, m_rayTracingPipelineProperties.shaderGroupBaseAlignment);
    pipeline->m_rayGenTable.size = pipeline->m_rayGenTable.stride;

    pipeline->m_rayMissTable.stride = perHandleSizeAligned;
    pipeline->m_rayMissTable.size =
        align_up(shaderState->m_rayMissShaderNum * perHandleSizeAligned,
                 m_rayTracingPipelineProperties.shaderGroupBaseAlignment);

    pipeline->m_rayHitTable.stride = perHandleSizeAligned;
    pipeline->m_rayHitTable.size =
        align_up((shaderState->m_rayAnyHitShaderNum + shaderState->m_rayClosestHitShaderNum)
                     * perHandleSizeAligned,
                 m_rayTracingPipelineProperties.shaderGroupBaseAlignment);

    pipeline->m_rayCallableTable.stride = perHandleSizeAligned;
    pipeline->m_rayCallableTable.size =
        align_up(shaderState->m_rayCallableShaderNum * perHandleSizeAligned,
                 m_rayTracingPipelineProperties.shaderGroupBaseAlignment);

    uint32_t             dataSize = groupHandleCount * perHandleSize;
    std::vector<uint8_t> handles(dataSize);
    //get the binding table data from raytracing pipeline, the order of these handles is similar to the the order in shader group array input when create raytracing pipeline
    VkResult result = vkGetRayTracingShaderGroupHandlesKHR(m_logicDevice, pipeline->m_pipeline, 0,
                                                           shaderState->m_shaderGroupInfo.size(),
                                                           dataSize, handles.data());
    DGASSERT(result == VK_SUCCESS);

    //shader binding table GPU buffer with aligned size
    VkDeviceSize SBTSize = pipeline->m_rayGenTable.size + pipeline->m_rayMissTable.size
        + pipeline->m_rayHitTable.size + pipeline->m_rayCallableTable.size;
    BufferCreateInfo sbtBufferInfo;
    sbtBufferInfo.reset()
        .setDeviceOnly(false)
        .setUsageSize(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                          | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                      SBTSize)
        .setName("我就看是不是这个Buffer");
    pipeline->m_shaderBindingTableBuffer = createBuffer(sbtBufferInfo);
    Buffer* sbtBuffer = accessBuffer(pipeline->m_shaderBindingTableBuffer);
    setResourceName(VK_OBJECT_TYPE_BUFFER, (uint64_t) (sbtBuffer->m_buffer),
                    "ShaderBindingTableBuffer");
    VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr,
                                   sbtBuffer->m_buffer};
    //get sbtbuffer's GPU device
    VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_logicDevice, &info);
    pipeline->m_rayGenTable.deviceAddress = sbtAddress;
    pipeline->m_rayMissTable.deviceAddress = sbtAddress + pipeline->m_rayGenTable.size;
    pipeline->m_rayHitTable.deviceAddress =
        sbtAddress + pipeline->m_rayGenTable.size + pipeline->m_rayMissTable.size;
    pipeline->m_rayCallableTable.deviceAddress = sbtAddress + pipeline->m_rayGenTable.size
        + pipeline->m_rayMissTable.size + pipeline->m_rayHitTable.size;
    //get data from handles
    auto  getHandle = [&](int i) { return handles.data() + i * perHandleSize; };
    void* mapPointer;
    vmaMapMemory(m_vma, sbtBuffer->m_allocation, &mapPointer);

    //pSBTBuffer represent the GPU buffer that table data will copy to
    auto* pSBTBuffer = reinterpret_cast<uint8_t*>(mapPointer);
    //pData is offset pointer
    uint8_t* pData = pSBTBuffer;
    uint32_t handleIdx = 0;
    //save raygen graup table
    memcpy(pData, getHandle(handleIdx++), perHandleSize);
    //jump to head of the miss group array
    pData = pSBTBuffer + pipeline->m_rayGenTable.size;
    //save raymiss group table
    for (uint32_t c = 0; c < shaderState->m_rayMissShaderNum; ++c) {
      memcpy(pData, getHandle(handleIdx++), perHandleSize);
      pData += pipeline->m_rayMissTable.stride;
    }
    //jump to head of the hit group array
    pData = pSBTBuffer + pipeline->m_rayGenTable.size + pipeline->m_rayMissTable.size;
    //save rayhit group table
    for (uint32_t c = 0;
         c < (shaderState->m_rayAnyHitShaderNum + shaderState->m_rayClosestHitShaderNum); ++c) {
      memcpy(pData, getHandle(handleIdx++), perHandleSize);
      pData += pipeline->m_rayHitTable.stride;
    }
    //jump to head of the callable group array
    pData = pSBTBuffer + pipeline->m_rayGenTable.size + pipeline->m_rayMissTable.size
        + pipeline->m_rayHitTable.size;
    for (uint32_t c = 0; c < shaderState->m_rayCallableShaderNum; ++c) {
      memcpy(pData, getHandle(handleIdx++), perHandleSize);
      pData += pipeline->m_rayCallableTable.stride;
    }
    vmaUnmapMemory(m_vma, sbtBuffer->m_allocation);
  } else {
    VkComputePipelineCreateInfo computeInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    computeInfo.stage = shaderState->m_shaderStageInfo[0];
    computeInfo.layout = pipelineLayout;
    DGASSERT(vkCreateComputePipelines(m_logicDevice, VK_NULL_HANDLE, 1, &computeInfo, nullptr,
                                      &pipeline->m_pipeline)
             == VK_SUCCESS);
    pipeline->m_pipelineType = shaderState->m_pipelineType;
    pipeline->m_pipelineHandle = handle;
    pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
  }
  setResourceName(VK_OBJECT_TYPE_PIPELINE, (uint64_t) pipeline->m_pipeline,
                  pipelineInfo.name.c_str());
  return handle;
}

void DeviceContext::init(const ContextCreateInfo& contextInfo) {
  //spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
  DG_INFO("Vulkan Context is initing");
  VkResult          result;
  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               nullptr,
                               contextInfo.m_applicatonName,
                               VK_MAKE_VERSION(1, 0, 0),
                               "DogEngine",
                               VK_MAKE_VERSION(1, 0, 0),
                               VK_API_VERSION_1_3};
  u32               instanceExtensionCount;
  vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(instanceExtensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, extensions.data());
  std::vector<void*>       features;
  std::vector<const char*> instanceExtensions =
      fillFilterdNameArray(extensions, contextInfo.m_instanceExtensions, features);
  getRequiredInstanceExtensions(instanceExtensions);

  u32 instanceLayerCount;
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());
  std::vector<const char*> validationLayers =
      fillFilteredNameArray(instanceLayers, contextInfo.m_instancelayers);

  VkInstanceCreateInfo instanceInfo = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    nullptr,
    0,
    &appInfo,

#if defined(VULKAN_DEBUG)
    static_cast<u32>(validationLayers.size()),
    validationLayers.data(),
#else
    0,
    nullptr,
#endif//VULKAN_DEBUG
    static_cast<u32>(instanceExtensions.size()),
    instanceExtensions.data()
  };
  result = vkCreateInstance(&instanceInfo, nullptr, &m_instance);
  DGASSERT(result == VK_SUCCESS);
  DG_INFO("Instance created successfully");
#if defined(VULKAN_DEBUG)
  DebugMessanger::GetInstance()->SetupDebugMessenger(this);

  if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
    pRENDERDOC_GetAPI RENDERDOC_GetAPI =
        (pRENDERDOC_GetAPI) GetProcAddress(mod, "RENDERDOC_GetAPI");
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_3_0, (void**) &m_renderDoc_api);
    assert(ret == 1);
  }
#endif//VULKAN_DEBUG
  m_swapChainWidth = contextInfo.m_windowWidth;
  m_swapChainHeight = contextInfo.m_windowHeight;

  u32 physicalDeviceNums;
  vkEnumeratePhysicalDevices(m_instance, &physicalDeviceNums, nullptr);
  std::vector<VkPhysicalDevice> phyDevices(physicalDeviceNums);
  vkEnumeratePhysicalDevices(m_instance, &physicalDeviceNums, phyDevices.data());

  GLFWwindow* window = (GLFWwindow*) contextInfo.m_window;
  result = glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface);
  DGASSERT(result == VK_SUCCESS);

  m_window = window;
  glfwSetWindowUserPointer(m_window, this);
  glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

  VkPhysicalDevice discrete_gpu = VK_NULL_HANDLE;
  VkPhysicalDevice integrate_gpu = VK_NULL_HANDLE;
  for (u32 i = 0; i < physicalDeviceNums; ++i) {
    VkPhysicalDevice physicalDevice = phyDevices[i];
    vkGetPhysicalDeviceProperties(physicalDevice, &m_phyDeviceProperty);
    if (m_phyDeviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && getFamilyIndex(physicalDevice)) {
      discrete_gpu = physicalDevice;
      break;
    }
    if (m_phyDeviceProperty.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        && getFamilyIndex(physicalDevice)) {
      integrate_gpu = physicalDevice;
      break;
    }
  }

  if (discrete_gpu != VK_NULL_HANDLE) {
    std::vector<std::string> missingExtensions;
    u32                      devicePropCount;
    vkEnumerateDeviceExtensionProperties(discrete_gpu, nullptr, &devicePropCount, nullptr);
    std::vector<VkExtensionProperties> props(devicePropCount);
    vkEnumerateDeviceExtensionProperties(discrete_gpu, nullptr, &devicePropCount, props.data());

    for (const auto& reqDeviceExtension : contextInfo.m_deviceExtensions) {
      bool found = false;
      if (reqDeviceExtension.optional) continue;
      for (auto prop : props) {
        if (strcmp(prop.extensionName, reqDeviceExtension.name.c_str()) == 0) {
          found = true;
          deviceExtensions.push_back(reqDeviceExtension.name.c_str());
          break;
        }
      }
      if (!found) { missingExtensions.push_back(reqDeviceExtension.name); }
    }

    if (!missingExtensions.empty()) {
      DG_ERROR("Critical device feature names :", missingExtensions[0], " is not supported!");
      //exit(-1);
    }
    m_physicalDevice = discrete_gpu;
    m_supportRayTracing = true;
    DG_INFO("Discrete GPU is setted as physical device")
  } else if (integrate_gpu != VK_NULL_HANDLE) {
    m_physicalDevice = integrate_gpu;
    DG_WARN("Discrete GPU is not find in your device, you will get limited performence and less "
            "gpu features.")
    DG_INFO("Integrate GPU is setted as physical device")
  } else {
    DG_ERROR("Seems there is not an avaliable GPU Device");
    exit(-1);
  }
  VkDeviceQueueCreateFlags test;
  float                    queuePriority = 1.0f;
  u16                      queueCount = 0;
  queueCount += m_graphicQueueIndex != -1 ? 1 : 0;
  queueCount += m_computeQueueIndex != -1 ? 1 : 0;
  queueCount += m_transferQueueIndex != -1 ? 1 : 0;

  std::vector<VkDeviceQueueCreateInfo> queueInfos(queueCount);
  queueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueInfos[0].pQueuePriorities = &queuePriority;
  queueInfos[0].queueCount = 1;
  queueInfos[0].queueFamilyIndex = m_graphicQueueIndex;
  if (queueCount > 1) {
    queueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfos[1].pQueuePriorities = &queuePriority;
    queueInfos[1].queueCount = 1;
    queueInfos[1].queueFamilyIndex = m_computeQueueIndex;
  }
  if (queueCount > 2) {
    queueInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfos[2].pQueuePriorities = &queuePriority;
    queueInfos[2].queueCount = 1;
    queueInfos[2].queueFamilyIndex = m_transferQueueIndex;
  }

  void* FeatureLists =
      getRequiredDeviceExtensions(m_physicalDevice, contextInfo.m_deviceExtensions);

  VkDeviceCreateInfo deviceInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = queueCount;
  deviceInfo.pQueueCreateInfos = queueInfos.data();
  deviceInfo.enabledExtensionCount = deviceExtensions.size();
  deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
  deviceInfo.pNext = FeatureLists;

  result = vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_logicDevice);
  DGASSERT(result == VK_SUCCESS);
  DG_INFO("Logical device created successfully");
  load_VK_EXTENSIONS(m_instance, vkGetInstanceProcAddr, m_logicDevice, vkGetDeviceProcAddr);

  vkGetDeviceQueue(m_logicDevice, m_graphicQueueIndex, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_logicDevice, m_computeQueueIndex, 0, &m_computeQueue);
  vkGetDeviceQueue(m_logicDevice, m_transferQueueIndex, 0, &m_transferQueue);

  std::vector<VkFormat> surface_image_formats = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
                                                 VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
  VkColorSpaceKHR       surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  u32                   supportedCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &supportedCount, nullptr);
  std::vector<VkSurfaceFormatKHR> supported_formats(supportedCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &supportedCount,
                                       supported_formats.data());
  DG_INFO("Surface created successfully");
  bool formatFound = false;
  for (int i = 0; i < surface_image_formats.size(); ++i) {
    for (int j = 0; j < supported_formats.size(); ++j) {
      if (supported_formats[j].format == surface_image_formats[i]
          && supported_formats[j].colorSpace == surface_color_space) {
        m_surfaceFormat = supported_formats[j];
        formatFound = true;
        break;
      }
    }
    if (formatFound) break;
  }
  if (!formatFound) {
    DG_ERROR("Can not find property surface format");
    exit(-1);
  }
  setPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR);

  createSwapChain();
  DG_INFO("Swaphcain created successfully");
  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  if (m_supportRayTracing) {
    VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    prop2.pNext = &m_rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);
    if (m_rayTracingPipelineProperties.maxRayRecursionDepth <= 1) {
      DG_ERROR("Device fails to support ray recursion.");
      exit(-1);
    }
  }

  VmaAllocatorCreateInfo vmaInfo = {};
  vmaInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  vmaInfo.physicalDevice = m_physicalDevice;
  vmaInfo.device = m_logicDevice;
  vmaInfo.instance = m_instance;
  vmaInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  //vmaInfo.pVulkanFunctions = &vulkanFunctions;

  result = vmaCreateAllocator(&vmaInfo, &m_vma);
  DGASSERT(result == VK_SUCCESS);

  static const u32                  k_global_pool_size = 4096;
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, k_global_pool_size},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_global_pool_size},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, k_global_pool_size},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, k_global_pool_size},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, k_global_pool_size},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, k_global_pool_size}};

  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = k_global_pool_size * poolSizes.size();
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  result = vkCreateDescriptorPool(m_logicDevice, &poolInfo, nullptr, &m_descriptorPool);
  DGASSERT(result == VK_SUCCESS)
  std::vector<VkDescriptorPoolSize> bindLessSizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_max_bindless_resource},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, k_max_bindless_resource}};
  poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
  poolInfo.maxSets = k_global_pool_size * bindLessSizes.size();
  poolInfo.poolSizeCount = bindLessSizes.size();
  poolInfo.pPoolSizes = bindLessSizes.data();
  result = vkCreateDescriptorPool(m_logicDevice, &poolInfo, nullptr, &m_bindlessDescriptorPool);
  DGASSERT(result == VK_SUCCESS)
  DG_INFO("Descriptor Pool created successfully");

  m_buffers.init(409600);
  m_textures.init(4096);
  m_pipelines.init(4096);
  m_samplers.init(4096);
  m_descriptorSetLayouts.init(4096);
  m_descriptorSets.init(4096);
  m_renderPasses.init(128);
  m_shaderStates.init(128);
  m_frameBuffers.init(128);
  DG_INFO("Resource Pools created successfully");

  DescriptorSetLayoutCreateInfo bindLessLayout{};
  bindLessLayout.reset()
      .setName("bindlessLayout")
      .addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_bindless_sampled_texture_bind_index,
                   k_max_bindless_resource, "bindLessSampledTexture"})
      .addBinding({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, k_bindless_sampled_texture_bind_index + 1,
                   k_max_bindless_resource, "bindLessStorageTexture"});
  bindLessLayout.bindless = true;
  m_bindlessDescriptorSetLayout = createDescriptorSetLayout(bindLessLayout);
  DescriptorSetCreateInfo bindLessSetCI{};
  bindLessSetCI.reset().setName("bindlessSet").setLayout(m_bindlessDescriptorSetLayout);
  m_bindlessDescriptorSet = createDescriptorSet(bindLessSetCI);
  DescriptorSet* bindLessSet = accessDescriptorSet(m_bindlessDescriptorSet);
  if (bindLessSet) { m_VulkanBindlessDescriptorSet = bindLessSet->m_vkdescriptorSet; }

  VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  for (int i = 0; i < k_max_swapchain_images; ++i) {
    vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_render_complete_semaphore[i]);
    std::string name = "m_render_complete_semaphore" + std::to_string(i);
    setResourceName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t) m_render_complete_semaphore[i],
                    name.c_str());
    vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_image_acquired_semaphore[i]);
    setResourceName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t) m_image_acquired_semaphore[i],
                    "m_image_acquired_semaphore");
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(m_logicDevice, &fenceInfo, nullptr, &m_render_queue_complete_fence[i]);
  }
  BufferCreateInfo viewProjectUniformInfo;
  viewProjectUniformInfo.reset()
      .setDeviceOnly(false)
      .setName("viewProjectUniformBuffer")
      .setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(UniformData));
  m_viewProjectUniformBuffer = createBuffer(viewProjectUniformInfo);
  commandBufferRing.init(this);
  DG_INFO("commandBufferManager inited successfully");
  m_currentSwapchainImageIndex = 0;
  m_currentFrame = 0;
  m_FrameCount = 0;

  //m_delectionQueue.resize(128);
  m_descriptorSetUpdateQueue.resize(32);

  SamplerCreateInfo sc{};

  sc.set_address_uvw(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
                     VK_SAMPLER_ADDRESS_MODE_REPEAT)
      .set_name("Default sampler");
  sc.set_min_mag_mip(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
  m_defaultSampler = createSampler(sc);

  BufferCreateInfo bc{0, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, nullptr, "FullScr_vb", true};
  m_FullScrVertexBuffer = createBuffer(bc);

  TextureCreateInfo tc{nullptr,
                       {m_swapChainWidth, m_swapChainHeight, 1},
                       1,
                       VK_FORMAT_D32_SFLOAT,
                       TextureType::Enum::Texture2D,
                       "Depth_Texture",
                       TextureFlags::Mask::Default_mask};
  m_depthTexture = createTexture(tc);

  TextureCreateInfo dt{nullptr,
                       {1, 1, 1},
                       1,
                       VK_FORMAT_R8G8B8A8_UNORM,
                       TextureType::Enum::Texture2D,
                       "default texture",
                       TextureFlags::Mask::Default_mask};
  m_DummyTexture = createTexture(dt);

  m_defaultSetLayoutCI.reset().addBinding(
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "globalUniformBuffer"});

  //RenderPassCreat
  RenderPassCreateInfo renderpassInfo{};
  renderpassInfo.setName("SwapChain pass")
      .setType(RenderPassType::Enum::SwapChain)
      .setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear,
                     RenderPassOperation::Clear);
  m_swapChainPass = createRenderPass(renderpassInfo);

  {
    //m_gameViewImageCount = m_swapchainImageCount;
    m_gameViewFrameTextures.resize(m_gameViewImageCount);
    m_gameViewFrameBuffers.resize(m_gameViewImageCount);
    m_gameViewTextureDescs.resize(m_gameViewImageCount);
    m_gameViewWidth = m_swapChainWidth / 2;
    m_gameViewHeight = m_swapChainHeight / 2;
    DescriptorSetLayoutCreateInfo gameViewLayoutInfo{};
    gameViewLayoutInfo.reset()
        .setName("GameViewDescLayout")
        .addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, "GameViewDescLayout",
                     VK_SHADER_STAGE_FRAGMENT_BIT});
    m_gameViewDescLayout = createDescriptorSetLayout(gameViewLayoutInfo);

    TextureCreateInfo gameViewFrameTextureInfo{};
    gameViewFrameTextureInfo.setName("gameViewFrameTextureInfo")
        .setExtent({m_gameViewWidth, m_gameViewHeight, 1})
        .setFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
        .setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask
                 | TextureFlags::Mask::Compute_mask)
        .setMipmapLevel(1)
        .setTextureType(TextureType::Texture2D);
    //create image\imageView\fbo
    for (int i = 0; i < m_gameViewImageCount; ++i) {
      m_gameViewFrameTextures[i] = createTexture(gameViewFrameTextureInfo);

      Texture*       tex = accessTexture(m_gameViewFrameTextures[i]);
      CommandBuffer* cmd = getInstantCommandBuffer();
      cmd->begin();
      addImageBarrier(cmd->m_commandBuffer, tex, RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_COMMON, 0,
                      1, false);
      cmd->flush(m_graphicsQueue);

      DescriptorSetCreateInfo descInfo{};
      descInfo.reset()
          .setName("gameViewFrameDesc")
          .setLayout(m_gameViewDescLayout)
          .texture(m_gameViewFrameTextures[i], 0);
      m_gameViewTextureDescs[i] = createDescriptorSet(descInfo);
    }
    TextureCreateInfo tc{nullptr,
                         {m_gameViewWidth, m_gameViewHeight, 1},
                         1,
                         VK_FORMAT_D32_SFLOAT,
                         TextureType::Enum::Texture2D,
                         "GameView_Depth_Texture",
                         TextureFlags::Mask::Default_mask};
    m_gameViewDepthTexture = createTexture(tc);

    RenderPassCreateInfo gameViewPassInfo{};
    gameViewPassInfo.setName("GameViewPass")
        .setType(RenderPassType::Enum::Geometry)
        .setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear,
                       RenderPassOperation::Enum::Clear)
        .setFinalLayout(VK_IMAGE_LAYOUT_GENERAL)
        .setDepthStencilTexture(m_gameViewDepthTexture)
        .addRenderTexture(m_gameViewFrameTextures[0]);
    m_gameViewPass = createRenderPass(gameViewPassInfo);
    RenderPass* gameviewPassPtr = accessRenderPass(m_gameViewPass);
    if (m_gameViewImageCount > 1) { gameviewPassPtr->m_type = RenderPassType::SwapChain; }
    for (int i = 0; i < m_gameViewImageCount; ++i) {
      FrameBufferCreateInfo gameViewFboInfo{};
      gameViewFboInfo.reset()
          .setName("gameViewFrameBuffer")
          .setExtent({m_gameViewWidth, m_gameViewHeight})
          .setDepthStencilTexture(m_gameViewDepthTexture)
          .addRenderTarget(m_gameViewFrameTextures[i])
          .setRenderPass(m_gameViewPass);
      m_gameViewFrameBuffers[i] = createFrameBuffer(gameViewFboInfo);
    }
  }
}

void DeviceContext::newFrame() {
  VkFence* renderCompleteFence = &m_render_queue_complete_fence[m_currentFrame];
  vkWaitForFences(m_logicDevice, 1, renderCompleteFence, VK_TRUE, UINT64_MAX);
  VkResult res = vkAcquireNextImageKHR(m_logicDevice, m_swapchain, UINT64_MAX,
                                       m_image_acquired_semaphore[m_currentFrame], VK_NULL_HANDLE,
                                       &m_currentSwapchainImageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR) { reCreateSwapChain(); }
  vkResetFences(m_logicDevice, 1, renderCompleteFence);
}

void DeviceContext::present() {
  VkFence*        renderCompleteFence = &m_render_queue_complete_fence[m_currentFrame];
  VkSemaphore*    renderCompleteSemaphore = &m_render_complete_semaphore[m_currentFrame];
  VkCommandBuffer enquedCommandBuffers[16];
  for (int i = 0; i < m_queuedCommandBuffer.size(); ++i) {
    CommandBuffer* cmd = m_queuedCommandBuffer[i];
    enquedCommandBuffers[i] = cmd->m_commandBuffer;
    if (cmd->m_isRecording && cmd->m_currRenderPass
        && (cmd->m_currRenderPass->m_type != RenderPassType::Enum::Compute)) {
      vkCmdEndRenderPass(cmd->m_commandBuffer);
    }
    vkEndCommandBuffer(cmd->m_commandBuffer);
    cmd->m_isRecording = false;
    cmd->m_isBegin = false;
    cmd->m_currRenderPass = nullptr;
    //尝试换成cmd->reset();试试行不行
  }

  if (!m_texture_to_update_bindless.empty()) {
    VkWriteDescriptorSet  bindlessWrite[k_max_bindless_resource];
    VkDescriptorImageInfo bindlessImageInfo[k_max_bindless_resource];
    int                   currWriteIdx = 0;
    for (int i = m_texture_to_update_bindless.size() - 1; i >= 0; --i) {
      ResourceUpdate& resUpdate = m_texture_to_update_bindless[i];
      {
        Texture* texture = accessTexture({resUpdate.handle});
        if (texture->m_imageInfo.imageView == VK_NULL_HANDLE) {
          m_texture_to_update_bindless.pop_back();
          continue;
        }
        VkWriteDescriptorSet& descWrite = bindlessWrite[currWriteIdx];
        descWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descWrite.descriptorCount = 1;
        descWrite.dstArrayElement = resUpdate.handle;
        descWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descWrite.dstSet = m_VulkanBindlessDescriptorSet;
        descWrite.dstBinding = k_bindless_sampled_texture_bind_index;
        DGASSERT(texture->m_handle.index == resUpdate.handle)
        Sampler*               sampler = accessSampler(m_defaultSampler);
        VkDescriptorImageInfo& imageInfo = bindlessImageInfo[currWriteIdx];
        if (!resUpdate.deleting) {
          imageInfo.imageView = texture->m_imageInfo.imageView;
          if (texture->m_imageInfo.sampler != VK_NULL_HANDLE) {
            imageInfo.sampler = texture->m_imageInfo.sampler;
          } else {
            imageInfo.sampler = sampler->m_sampler;
          }
        } else {
          Texture* dumyTex = accessTexture(m_DummyTexture);
          imageInfo.imageView = dumyTex->m_imageInfo.imageView;
          imageInfo.sampler = sampler->m_sampler;
        }
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descWrite.pImageInfo = &imageInfo;
        m_texture_to_update_bindless.pop_back();
        ++currWriteIdx;

        if (resUpdate.deleting) {
          m_delectionQueue.push_back(
              {ResourceUpdateType::Enum::Texture, texture->m_handle.index, m_currentFrame, true});
        }
      }
    }
    if (currWriteIdx) {
      vkUpdateDescriptorSets(m_logicDevice, currWriteIdx, bindlessWrite, 0, nullptr);
    }
  }
  VkSemaphore          waitSmps[] = {m_image_acquired_semaphore[m_currentFrame]};
  VkPipelineStageFlags wait_stages = {VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR};
  VkSubmitInfo         subInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  subInfo.waitSemaphoreCount = 1;
  subInfo.pWaitSemaphores = waitSmps;
  subInfo.signalSemaphoreCount = 1;
  subInfo.pSignalSemaphores = renderCompleteSemaphore;
  subInfo.commandBufferCount = m_queuedCommandBuffer.size();
  subInfo.pCommandBuffers = enquedCommandBuffers;
  subInfo.pWaitDstStageMask = &wait_stages;
  //subInfo.pWaitDstStageMask = nullptr;
  vkQueueSubmit(m_graphicsQueue, 1, &subInfo, VK_NULL_HANDLE);

  //draw UI
  wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  subInfo.pWaitSemaphores = renderCompleteSemaphore;
  subInfo.pSignalSemaphores = &GUI::getInstance().getCurrSemaphore();
  subInfo.commandBufferCount = 1;
  subInfo.pCommandBuffers = &GUI::getInstance().getCurrentFramCb()->m_commandBuffer;
  subInfo.pWaitDstStageMask = &wait_stages;
  vkQueueSubmit(m_graphicsQueue, 1, &subInfo, *renderCompleteFence);
  //present
  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.pImageIndices = &m_currentSwapchainImageIndex;
  presentInfo.pSwapchains = &m_swapchain;
  presentInfo.swapchainCount = 1;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &GUI::getInstance().getCurrSemaphore();
  presentInfo.pResults = nullptr;
  VkResult res = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
  m_queuedCommandBuffer.clear();

  // if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || m_resized) {
  //   reCreateSwapChain();
  //   m_resized = false;
  // }
  // if (gameViewResize) {
  //   resizeGameViewPass({m_gameViewWidth, m_gameViewHeight});
  //   gameViewResize = false;
  // }
  m_currentFrame = (m_currentFrame + 1) % m_swapchainImageCount;
  ++m_FrameCount;

  if (!m_delectionQueue.empty()) {
    for (int i = m_delectionQueue.size() - 1; i >= 0; --i) {
      ResourceUpdate& rd = m_delectionQueue[i];
      if (rd.currentFrame == m_currentFrame) {
        switch (rd.type) {
          case (ResourceUpdateType::Enum::Buffer): {
            destroyBufferInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::Texture): {
            destroyTextureInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::DescriptorSet): {
            destroyDescriptorInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::DescriptorSetLayout): {
            destroyDescriptorSetLayoutInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::RenderPass): {
            destroyRenderpassInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::FrameBuffer): {
            destroyFrameBufferInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::Pipeline): {
            destroyPipelineInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::Sampler): {
            destroySamplerInstant(rd.handle);
            break;
          }
          case (ResourceUpdateType::Enum::ShaderState): {
            destroyShaderStateInstant(rd.handle);
            break;
          }
          default: {
            DG_ERROR("You are trying to destroy an unsupported object");
          }
        }
        rd.currentFrame = UINT32_MAX;
        //TODO: this should not pop the last obj directly
        m_delectionQueue.pop_back();
      }
    }
  }
}// namespace dg

void DeviceContext::Destroy() {
  DG_INFO("Destroy vulkan context");
  commandBufferRing.destroy();

  for (int i = 0; i < m_swapchainImageCount; ++i) {
    vkDestroySemaphore(m_logicDevice, m_render_complete_semaphore[i], nullptr);
    vkDestroyFence(m_logicDevice, m_render_queue_complete_fence[i], nullptr);
    vkDestroySemaphore(m_logicDevice, m_image_acquired_semaphore[i], nullptr);
  }

  // destroy game view pass
  {
    DestroyDescriptorSetLayout(m_gameViewDescLayout);
    DestroyRenderPass(m_gameViewPass);
    DestroyTexture(m_gameViewDepthTexture);
    for (int i = 0; i < m_gameViewImageCount; ++i) {
      DestroyTexture(m_gameViewFrameTextures[i]);
      FrameBuffer* fbo = accessFrameBuffer(m_gameViewFrameBuffers[i]);
      vkDestroyFramebuffer(m_logicDevice, fbo->m_frameBuffer, nullptr);
    }
  }

  DestroyTexture(m_DummyTexture);
  DestroyDescriptorSetLayout(m_bindlessDescriptorSetLayout);
  DestroyTexture(m_depthTexture);
  DestroyBuffer(m_FullScrVertexBuffer);
  DestroyBuffer(m_viewProjectUniformBuffer);
  DestroyRenderPass(m_swapChainPass);
  DestroySampler(m_defaultSampler);
  DestroyDescriptorSet(m_bindlessDescriptorSet);
  // DestroyPipeline(m_nprPipeline);
  // DestroyPipeline(m_pbrPipeline);
  // DestroyPipeline(m_rayTracingPipeline);
  for (u32 i = 0; i < m_delectionQueue.size(); ++i) {
    ResourceUpdate& resDelete = m_delectionQueue[i];
    if (resDelete.currentFrame == -1) continue;
    switch (resDelete.type) {
      case ResourceUpdateType::Enum::Buffer: {
        destroyBufferInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::Texture: {
        destroyTextureInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::Pipeline: {
        destroyPipelineInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::RenderPass: {
        destroyRenderpassInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::DescriptorSet: {
        destroyDescriptorInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::DescriptorSetLayout: {
        destroyDescriptorSetLayoutInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::Sampler: {
        destroySamplerInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::FrameBuffer: {
        destroyFrameBufferInstant(resDelete.handle);
        break;
      }
      case ResourceUpdateType::Enum::ShaderState: {
        destroyShaderStateInstant(resDelete.handle);
        break;
      }
      default: DG_WARN("Try to destroy an invalid resource object");
    }
  }
  destroySwapChain();
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vmaDestroyAllocator(m_vma);
  vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);
  //vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);
  vkDestroyDescriptorPool(m_logicDevice, m_bindlessDescriptorPool, nullptr);
  vkDestroyDevice(m_logicDevice, nullptr);
  DebugMessanger::GetInstance()->Clear();
  vkDestroyInstance(m_instance, nullptr);
  DG_INFO("Context destroied successfully");
}

void DeviceContext::setResourceName(VkObjectType objectType, uint64_t handle, const char* name) {
  if (!m_debugUtilsExtentUsed) return;
  DebugMessanger::GetInstance()->setObjectName(handle, name, objectType);
}

void DeviceContext::pushMarker(VkCommandBuffer cmd, const char* name) {
  VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
  label.pLabelName = name;
  label.color[0] = 1.0f;
  label.color[1] = 1.0f;
  label.color[2] = 1.0f;
  label.color[3] = 1.0f;
  vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
}

void DeviceContext::popMarker(VkCommandBuffer cmd) { vkCmdEndDebugUtilsLabelEXT(cmd); }

std::vector<const char*>
DeviceContext::fillFilterdNameArray(const std::vector<VkExtensionProperties>& properties,
                                    const ContextCreateInfo::ExtArray&        requested,
                                    std::vector<void*>&                       featureStrucutres) {
  std::vector<const char*> used;
  for (const auto& itr : requested) {
    bool found = false;
    for (const auto& property : properties) {
      if (strcmp(itr.name.c_str(), property.extensionName) == 0
          && (itr.version == 0 || itr.version == property.specVersion)) {
        found = true;
        break;
      }
    }
    if (found) {
      used.push_back(itr.name.c_str());
      if (itr.pFeatureStruct) { featureStrucutres.push_back(itr.pFeatureStruct); }
    } else if (itr.optional == false) {
      DG_ERROR("A critical Extension with name: ", itr.name, " is not supported!");
      //exit(-1);
    }
  }
  return used;
}
std::vector<const char*>
DeviceContext::fillFilteredNameArray(const std::vector<VkLayerProperties>& properties,
                                     const ContextCreateInfo::ExtArray&    requested) {
  std::vector<const char*> used;
  for (auto& itr : requested) {
    bool found = false;
    for (const auto& property : properties) {
      if (strcmp(property.layerName, itr.name.c_str()) == 0) {
        found = true;
        break;
      }
    }
    if (found) {
      used.push_back(itr.name.c_str());
    } else if (itr.optional == false) {
      DG_ERROR("Critical instance layer with name: ", itr.name, " is nor supported!");
      // exit(-1);
    }
  }
  return used;
}
}// namespace dg
