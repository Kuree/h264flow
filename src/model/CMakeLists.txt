find_package(OpenCV)
if (OPENCV_FOUND)
    add_executable(annotator annotator.cc)
    target_link_libraries(annotator h264)
    target_link_libraries(annotator ${OpenCV_LIBS})

else()
    MESSAGE(WARNING
            "OpenCV not found. executables will not be built")
endif()


#add_executable(train_model train_model.cc)

#target_link_libraries(train_model h264)
#add_subdirectory(tiny-dnn)
#include_directories(tiny-dnn)


# disable warnings for tiny-dnn
#set_source_files_properties(train_model.cc PROPERTIES COMPILE_FLAGS
#        "-Wno-effc++ -Wno-ignored-qualifiers -Wno-unused-parameter \
#        -Wno-implicit-fallthrough")