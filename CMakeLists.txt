cmake_minimum_required(VERSION 3.10)

project(vulkan_learning)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include_directories("C:/VulkanSDK/1.3.231.1/Include" "E:/training/GLlib/include" "D:/vulkan/Include" "./src")
add_subdirectory(src)
find_library(vulkan_lib vulkan-1 "C:/VulkanSDK/1.3.231.1/Lib" "D:/vulkan/Lib")
find_library(glfw_lib glfw3 "C:/VulkanSDK/1.3.231.1/Lib" "E:/training/GLlib/libs")
aux_source_directory(. DIR_SRC)
add_executable(vulkan_learning ${DIR_SRC})
target_link_libraries(vulkan_learning SRC_LIB)

target_include_directories(vulkan_learning PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(vulkan_learning PUBLIC "C:/VulkanSDK/1.3.231.1/Include" "E:/training/GLlib/include" "D:/vulkan/Include" )
target_link_libraries(vulkan_learning ${vulkan_lib})
target_link_libraries(vulkan_learning ${glfw_lib})