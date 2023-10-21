aux_source_directory(. LIB_SRCS)
message("--------------file lists-------------")
message(${LIB_SRCS})
message("-------------------------------------")
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
add_library(SRC_LIB STATIC ${LIB_SRCS})
target_include_directories(SRC_LIB PUBLIC "${VULKAN_SDK_PATH}/Include" "F:/training/GLlib/include" "D:/vulkan/Include" "../include" 
                        ${CMAKE_CURRENT_SOURCE_DIR}
                        "../external/glfw/include"
                        "../external/VMA/include"
                        )
