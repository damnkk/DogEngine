#ifndef ALLOCATEOBJECT_H
#define ALLOCATEOBJECT_H
#include "Common.h"

struct Texture{
    vk::Image TextureImage;
    vk::DescriptorImageInfo m_TextureInfo;
    vk::DeviceMemory TextureMemory;
    vk::Format TextureFormat;
    void destroy();
};

struct Buffer{
    vk::Buffer bufferObject;
    vk::DeviceMemory bufferMemory;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};

#endif