#pragma once
#include "Common.h"

struct Texture{
    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation allocation;
    VkDescriptorSet textureDescriptor;
    VkSampler sampler = VK_NULL_HANDLE;
    VkFormat Format		= {};
    uint32_t miplevels = 0;
    void destroy(VkDevice& device,VmaAllocator &allocator);
};

struct Buffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};
