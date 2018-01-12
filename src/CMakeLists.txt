file(GLOB SRC_FILES decoder/*.hh decoder/*.cc query/*.hh query/*.cc)
add_library(h264 ${SRC_FILES})

add_subdirectory(python)