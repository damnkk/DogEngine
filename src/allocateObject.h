#include "Common.h"

struct Texture{
    VkImage textureImage;
    VkImageView textureImageView;
    VmaAllocation allocation;
    VkDescriptorSet textureDescriptor;
    void destroy(VkDevice& device,VmaAllocator &allocator);
};

struct Buffer{
    VkBuffer buffer;
    VmaAllocation allocation;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};
