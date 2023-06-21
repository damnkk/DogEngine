aux_source_directory(. LIB_SRCS)
message("----------------------")
message(${LIB_SRCS})
message("----------------------")
add_library(SRC_LIB STATIC ${LIB_SRCS})
target_include_directories(SRC_LIB PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    "../include"
    "../external/glm"
    "../external/glfw/include"
    "../external/VMA/include"
    "../external/vulkan-header/include"
    "../external/TinyObjLoader"
    "../external/stb"
)