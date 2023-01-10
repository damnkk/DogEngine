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

#include "vk_mem_alloc.h"

