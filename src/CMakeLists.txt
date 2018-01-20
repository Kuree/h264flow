file(GLOB SRC_FILES decoder/*.hh decoder/*.cc query/*.hh query/*.cc
        model/model-io.cc model/model-io.hh)
add_library(h264 ${SRC_FILES})

add_subdirectory(model)