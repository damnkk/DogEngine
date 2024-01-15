cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#set(CMAKE_CXX_COMPILER "E:/software/MinGW/bin/g++.exe")
project(vulkan_learning)
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})  

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glslang)
find_package(Vulkan REQUIRED)
add_subdirectory(dogEngine)
execute_process(
        COMMAND python ./update_glslang_sources.py
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/external/glslang"
        RESULT_VARIABLE SPIRV_TOOLS_CLONE
)
if(NOT SPIRV_TOOLS_CLONE EQUAL "0")
    message(FATAL_ERROR "Can not clone spirv-tools, glslang will not be compile successfully")
endif()
add_executable(vulkan_learning Vulkan.cpp)
target_include_directories(vulkan_learning PUBLIC
                            #${VULKAN_SDK_PATH}/Include
                            ${CMAKE_SOURCE_DIR}/dogEngine 
                            ${CMAKE_SOURCE_DIR}/external/glslang/glslang/include
                            #${CMAKE_SOURCE_DIR}/external/glm/glm
                            # ${CMAKE_SOURCE_DIR}/external/glfw/include
                            # ${CMAKE_SOURCE_DIR}/external/VMA/include
                            # ${CMAKE_SOURCE_DIR}/external/stb
                            # ${CMAKE_SOURCE_DIR}/external/spdlog/include
                            )
target_link_libraries(vulkan_learning
                      dogEngine)

add_executable(vktu vktu.cpp)
target_include_directories(vktu  PUBLIC ${CMAKE_SOURCE_DIR}/external/stb
                                        ${CMAKE_SOURCE_DIR}/external/glfw/include 
                                        ${CMAKE_SOURCE_DIR}/external/glm/glm
                                        ${VULKAN_SDK_PATH}/Include
                                        "${VULKAN_SDK_PATH}/Include"
                                        "${CMAKE_SOURCE_DIR}/external/spdlog/include"
                                        "${CMAKE_SOURCE_DIR}/external/tinyobjloader"
                                        "${CMAKE_SOURCE_DIR}/external/tinygltf"
                                        "${CMAKE_SOURCE_DIR}/external/stb"
                                        "${CMAKE_CURRENT_SOURCE_DIR}"
                                        "${CMAKE_CURRENT_SOURCE_DIR}/GUI")
target_link_libraries(vktu  Vulkan::Vulkan glfw spdlog)
