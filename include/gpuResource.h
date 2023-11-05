#pragma once
#include "platform.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

namespace dg{

using Resourcehandle = u32;

struct BufferHandle{
   Resourcehandle              index;
};

struct Texturehandle{
    Resourcehandle             index;
};

struct ShaderStateHandle{
    Resourcehandle             index;
};

struct SamplerHandle{
    Resourcehandle             index;
};

struct DescrpitorSetLayoutHandle{
    Resourcehandle              index;
};

struct DescriptorSetHandle{
    Resourcehandle              index;
};

struct PipelineHandle{
    Resourcehandle              index;
};

struct RenderPassHandle{
    Resourcehandle              index;
};

struct Texture{
    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation allocation;
    VkDescriptorSet textureDescriptor;
    VkSampler sampler = VK_NULL_HANDLE;
    VkFormat Format		= {};
    uint32_t miplevels = 1;
    void destroy(VkDevice& device,VmaAllocator &allocator);
};

struct Buffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};
} //namespace dg