cmake_minimum_required(VERSION 3.9)
project(h264flow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Weffc++ -Werror -fPIC -pthread -g")

# default release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

find_package(OpenCV)
if (OPENCV_FOUND)
    add_definitions(-DOPENCV_ENABLED)
endif()

add_subdirectory(python/pybind11)

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(python)
