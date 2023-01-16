#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef  GLFW_INCLUDE_VULKAN

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#undef  GLM_FORCE_RADIANS
#undef  GLM_FORCE_DEPTH_ZERO_TO_ONE


#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <ctime>
#include <unordered_map>

#include "vk_mem_alloc.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

struct constentData{
      uint32_t textureIndex;
      uint32_t textureNum;
};


const int MAX_FRAMES_IN_FLIGHT = 2;


const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

