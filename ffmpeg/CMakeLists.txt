add_library(ffmpeg libavflow.cc libavflow.hh)
target_link_libraries(ffmpeg ${FFMPEG_LIBRARIES})

set_source_files_properties(libavflow.cc PROPERTIES COMPILE_FLAGS
        "-Wno-effc++ -Wno-missing-field-initializers")
