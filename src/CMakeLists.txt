aux_source_directory(. LIB_SRCS)
message(${LIB_SRCS})
add_library(SRC_LIB STATIC ${LIB_SRCS})