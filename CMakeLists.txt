cmake_minimum_required(VERSION 3.9)
project(h264flow)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Weffc++ -Werror -fPIC -pthread -g")

# default release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

find_package(OpenCV)
if (OPENCV_FOUND)
    add_definitions(-DOPENCV_ENABLED)
endif()

find_package(FFmpeg)


add_subdirectory(python/pybind11)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(python)
add_subdirectory(ffmpeg)
