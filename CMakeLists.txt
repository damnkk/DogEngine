cmake_minimum_required(VERSION 3.10)


project(vulkan_learning)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(src)
add_subdirectory(external/glfw)
add_subdirectory(external/tinygltf)
find_package(Vulkan REQUIRED)
aux_source_directory(. DIR_SRC)
add_executable(vulkan_learning ${DIR_SRC})
target_link_libraries(vulkan_learning PUBLIC SRC_LIB)

target_include_directories(vulkan_learning PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    "./include"
    "./external/glm"
    "./external/glfw/include"
    "./external/VMA/include"
    "./external/TinyObjLoader"
    "./external/tinygltf"
)
target_link_libraries(vulkan_learning PUBLIC glfw Vulkan::Vulkan)
