#pragma once

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#undef  GLM_FORCE_RADIANS
#undef  GLM_FORCE_DEPTH_ZERO_TO_ONE

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

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
#include <algorithm>
#include <iomanip>
#include "DataStructures.h"
#include "allocateObject.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

static uint32_t miplevels;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

struct constentData{
    glm::mat4 modelMatrix;
};

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const std::vector<const char*> InstanceLayers = {
    "VK_LAYER_KHRONOS_validation"
};

