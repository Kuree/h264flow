cmake_minimum_required(VERSION 3.9)
project(h264flow)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Weffc++ -Werror -fPIC")

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(python)