find_package(OpenCV)
if (OPENCV_FOUND)
    add_executable(annotator annotator.cc)
    target_link_libraries(annotator h264)
    target_link_libraries(annotator ${OpenCV_LIBS})
else()
    MESSAGE(WARNING
            "OpenCV not found. executables will not be built")
endif()