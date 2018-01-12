add_executable(read_mv read_mv.cc)
target_link_libraries(read_mv h264)

add_executable(motion_region motion_region.cc)
target_link_libraries(motion_region h264)

find_package(OpenCV)
if (OPENCV_FOUND)
    add_executable(visualize visualize.cc)
    target_link_libraries(visualize h264)
    target_link_libraries(visualize ${OpenCV_LIBS})
else()
    MESSAGE(WARNING
            "OpenCV not found. executable <visualize> will not be built")
endif()