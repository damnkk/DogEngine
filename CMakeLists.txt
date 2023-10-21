cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#set(CMAKE_CXX_COMPILER "E:/software/MinGW/bin/g++.exe")
project(vulkan_learning)
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})  
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
add_subdirectory(src)
add_subdirectory(external/glfw)
find_package(Vulkan REQUIRED)
find_library(assimp libassimpd.dll "E:/Lib/lib")
aux_source_directory(. DIR_SRC)
add_executable(vulkan_learning ${DIR_SRC})

target_include_directories(vulkan_learning PUBLIC "E:/repository/OpenSceneGraph/include/" 
                            "${VULKAN_SDK_PATH}/Include" 
                            "E:/training/GLlib/include" 
                            "D:/vulkan/Include" 
                            "./src" 
                            "./include"
                            "external/glfw/include"
                            "external/VMA/include"
                            "external/glm/glm"
                            "external/stb")
target_link_libraries(vulkan_learning SRC_LIB)
target_link_libraries(vulkan_learning imm32)

target_link_libraries(vulkan_learning ${assimp})

target_include_directories(vulkan_learning PUBLIC "${PROJECT_BINARY_DIR}"
"./src")
target_link_libraries(vulkan_learning Vulkan::Vulkan glfw)