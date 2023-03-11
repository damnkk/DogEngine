#include "allocateObject.h"

void Texture::destroy(VkDevice& device, VmaAllocator& allocator){
    vkDestroyImageView(device, textureImageView, nullptr);
    vmaDestroyImage(allocator, textureImage, allocation);
    vkDestroySampler(device, sampler, nullptr);
}

void Buffer::destroy(VkDevice& device, VmaAllocator& allocator){
    vmaDestroyBuffer(allocator, buffer, allocation);
}

