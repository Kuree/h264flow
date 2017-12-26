cmake_minimum_required(VERSION 3.9)
project(h264flow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wextra -Weffc++ -Werror")

add_library(stdc++fs UNKNOWN IMPORTED)
set_property(TARGET stdc++fs PROPERTY IMPORTED_LOCATION "/usr/lib/gcc/x86_64-linux-gnu/7/libstdc++fs.a")

add_executable(h264flow src/io.hh src/io.cc src/nal.cc src/nal.hh src/mp4.cc src/mp4.hh src/main.cc src/util.hh src/h264.cc src/h264.hh)
target_link_libraries(h264flow stdc++fs)
