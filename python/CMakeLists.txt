set(PYBIND11_CPP_STANDARD -std=c++14)
pybind11_add_module(h264flow h264flow.cc opencvw.hh)
target_link_libraries(h264flow PRIVATE pybind11::module)
target_link_libraries(h264flow PRIVATE h264)
set_target_properties(h264flow PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}")

find_package(OpenCV)
if (OPENCV_FOUND)
    target_link_libraries(h264flow PRIVATE ${OpenCV_LIBS})
endif ()

target_link_libraries(h264flow PRIVATE ffmpeg)

# disable warnings for pybind11, which is a header only library
set_source_files_properties(h264flow.cc PROPERTIES COMPILE_FLAGS
        "-Wno-effc++ -Wno-deprecated-declarations")
