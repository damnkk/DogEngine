#pragma once
#include "Common.h"

struct Texture{
    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation allocation;
    VkDescriptorSet textureDescriptor;
    VkSampler sampler = VK_NULL_HANDLE;
    VkFormat Format		= VK_FORMAT_R8G8B8A8_SRGB;
    uint32_t miplevels = 1;
    void destroy(VkDevice& device,VmaAllocator &allocator);
};

struct Buffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};
