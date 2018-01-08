add_executable(h264flow read_mv.cc)
target_link_libraries(h264flow h264lib)
target_link_libraries(h264flow stdc++fs)