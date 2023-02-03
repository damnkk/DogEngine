#pragma once
#include "Common.h"

//read shader file
//choose properly Format
//get queue family indices
//get possible device extension support

//create image
//createimge view
//create sampler
//create multiple buffer image(depth position normal color)

//memory type index

//create buffer
//copy buffer cmd
//copy image buffer 

//command buffer begin recording and end&submit

//create shader module

class Utility{
public:
    //这些device都得改

    // You have to use the Setup to init Utility, before any initializing process in the vulkan render.
    static void Setup(MainDevice* main_device, VkSurfaceKHR* surface, VkCommandPool* command_pool, VkQueue* graphics_queue, VmaAllocator* allocator);

    // GENERIC
    static std::vector<char> ReadFile(const std::string& filename);
    static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidate, VkImageTiling tiling, VkFormatFeatureFlags features);
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    // IMAGE&TEXTURE
    static void createImage(uint32_t width, uint32_t height, uint32_t miplevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Texture& image);
    static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t miplevels);
    static VkSampler CreateSampler(const VkSamplerCreateInfo& sampler_create_info);
    static void CreateTextureSampler(Texture& image);
    static void CreateDepthBufferImage(Texture& image, const VkExtent2D &img_extent);
	static void CreatePositionBufferImage(Texture& image, const VkExtent2D& image_extent);
	static void CreateColorBufferImage(Texture& image, const VkExtent2D& img_extent);

    // BUFFERS
    static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer& buffer);
    static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static void CopyBufferToImage(const VkBuffer& src, const VkImage& image, const uint32_t width, const uint32_t height);

    // COMMAND BUFFER
    static VkCommandBuffer beginSingleTimeCommands();
    static void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // SHADERS
    static VkShaderModule createShaderModule(const std::vector<char>& code);
private:
    Utility() = default;
    static MainDevice* m_MainDevice;
    static VkSurfaceKHR* m_Surface;
    static VkCommandPool* m_CommandPool;
    static VkQueue* m_GraphicsQueue;
    static VmaAllocator* m_allocator;
};