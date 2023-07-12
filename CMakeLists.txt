cmake_minimum_required(VERSION 3.25.0)
project(Vulkan_Hpp_learning)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)
FILE(GLOB_RECURSE sourceFiles  "./src/*.cpp")
message("-----------------------")
message(${sourceFiles})
message("-----------------------")
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/tinygltf)
add_subdirectory(external/TinyObjLoader)
add_subdirectory(external/VMA)
   
add_executable(VulkanHppLearning Vulkan.cpp ${sourceFiles})
add_definitions(-DDEBUG)
target_include_directories(VulkanHppLearning PUBLIC
    "external/glfw/include"
    "include"
    "external/glm"
    "external/stb"
    "external/tinygltf"
    "external/VMA/include"
    "external/vulkan-header"
    ${Vulkan_INCLUDE_DIRS})

target_link_libraries(VulkanHppLearning PUBLIC ${Vulkan_LIBRARIES} glfw )