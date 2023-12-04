FILE(GLOB_RECURSE LIB_SRCS *pp *.h ../include/*pp)
#aux_source_directory(. LIB_SRCS)
message("--------------file lists-------------")
message(${LIB_SRCS})
message("-------------------------------------")
message("${CMAKE_SOURCE_DIR}")
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
add_library(dogEngine STATIC ${LIB_SRCS})
target_include_directories(dogEngine PRIVATE 
                        "${VULKAN_SDK_PATH}/Include"
                        "${CMAKE_SOURCE_DIR}/external/glfw/include"
                        "${CMAKE_SOURCE_DIR}/external/VMA/include"
                        "${CMAKE_SOURCE_DIR}/external/glm/include"
                        "${CMAKE_SOURCE_DIR}/external/spdlog/include"
                        "${CMAKE_SOURCE_DIR}/external/tinyobjloader"
                        "${CMAKE_SOURCE_DIR}/external/tinygltf"
                        "${CMAKE_SOURCE_DIR}/external/stb"
                        "${CMAKE_CURRENT_SOURCE_DIR}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/GUI"

                        )
