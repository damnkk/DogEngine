#pragma once
#include"Common.h"

struct UniformData{
  glm::mat4 vpMatrix;
};
struct MainDevice{
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkDeviceSize MinUniforBufferOffset;
};
