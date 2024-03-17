#ifndef DGPCH_H
#define DGPCH_H
// stb lib
#include "numeric"
#include "string"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory.h>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "dgplatform.h"

void load_VK_EXTENSIONS(VkInstance instance, PFN_vkGetInstanceProcAddr getInstanceProcAddr,
                        VkDevice device, PFN_vkGetDeviceProcAddr getDeviceProcAddr);

#endif//DGPCH_H