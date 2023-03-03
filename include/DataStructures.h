#include"Common.h"

struct MainDevice{
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkDeviceSize MinUniforBufferOffset;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct VulkanRenderData{
  VkInstance instance;
  MainDevice main_device;

  uint32_t graphic_queue_index;
  VkQueue graphic_queue;

  uint32_t min_image_count;
  uint32_t image_count;

  VkDescriptorPool imgui_descriptor_pool;
  VkDescriptorPool texture_descriptor_pool;
  VkDescriptorSetLayout texture_descriptor_layout;

  VkCommandPool command_pool;

  std::vector<VkCommandBuffer> command_buffers;
  VkRenderPass render_pass;
};

struct SubmissionSyncObjects{
  VkSemaphore OffScreenAvaliable;
  VkSemaphore ImageAvailable;
  VkSemaphore RenderFinished;
  VkFence InFlight;
};