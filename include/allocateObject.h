#ifndef ALLOCATEOBJECT_H
#define ALLOCATEOBJECT_H
#include "Common.h"

struct Texture{
    vk::Image TextureImage;
    vk::ImageView TextureImageView;
    vk::DeviceMemory TextureMemory;
    void destroy();
};

struct Buffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};

#endif