add_executable(read_mv read_mv.cc)
target_link_libraries(read_mv h264)

add_executable(motion_region motion_region.cc)
target_link_libraries(motion_region h264)

add_executable(benchmark benchmark.cc)
target_link_libraries(benchmark h264)

find_package(OpenCV)
if (OPENCV_FOUND)
    add_executable(visualize visualize.cc)
    target_link_libraries(visualize h264)
    target_link_libraries(visualize ${OpenCV_LIBS})

    add_executable(camera camera.cc)
    target_link_libraries(camera h264)
    target_link_libraries(camera ${OpenCV_LIBS})

    add_executable(boundary boundary.cc)
    target_link_libraries(boundary h264)
    target_link_libraries(boundary ${OpenCV_LIBS})

    add_executable(save_mv save_mv.cc)
    target_link_libraries(save_mv h264)
    target_link_libraries(save_mv ${OpenCV_LIBS})

    add_executable(scenecut scenecut.cc)
    target_link_libraries(scenecut h264)
    target_link_libraries(scenecut ${OpenCV_LIBS})

    add_executable(ffmpeg_mv ffmpeg_mv.cc)
    target_link_libraries(ffmpeg_mv ffmpeg)
    target_link_libraries(ffmpeg_mv ${OpenCV_LIBS})
    target_link_libraries(ffmpeg_mv h264)

    set_source_files_properties(ffmpeg_mv.cc PROPERTIES COMPILE_FLAGS
            "-Wno-effc++ -Wno-missing-field-initializers")

    add_executable(davis davis.cc)
    target_link_libraries(davis h264)
    target_link_libraries(davis ${OpenCV_LIBS})
else()
    MESSAGE(WARNING
            "OpenCV not found. executables will not be built")
endif()