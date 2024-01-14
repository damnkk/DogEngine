FILE(GLOB_RECURSE LIB_SRCS *pp *.h  ../include/*pp)
#aux_source_directory(. LIB_SRCS)
message("--------------file lists-------------")
message(${LIB_SRCS})
message("-------------------------------------")
message("${CMAKE_SOURCE_DIR}")
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})

set(KTX_DIR ./ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c)

add_library(dogEngine STATIC ${LIB_SRCS} ${KTX_SOURCES})
target_include_directories(dogEngine PUBLIC 
                        "${VULKAN_SDK_PATH}/Include"
                        "${CMAKE_SOURCE_DIR}/external/glfw/include"
                        "${CMAKE_SOURCE_DIR}/external/VMA/include"
                        "${CMAKE_SOURCE_DIR}/external/glm/include"
                        "${CMAKE_SOURCE_DIR}/external/spdlog/include"
                        "${CMAKE_SOURCE_DIR}/external/tinyobjloader"
                        "${CMAKE_SOURCE_DIR}/external/tinygltf"
                        "${CMAKE_SOURCE_DIR}/external/glslang/glslang/include"
                        "${CMAKE_SOURCE_DIR}/external/glslang/glslang"
                        "./ktx/include"
                        "${CMAKE_SOURCE_DIR}/external/stb"
                        "${CMAKE_CURRENT_SOURCE_DIR}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/GUI"
                        )
target_link_libraries(dogEngine PUBLIC glslang  glslang-default-resource-limits Vulkan::Vulkan glfw SPIRV)
