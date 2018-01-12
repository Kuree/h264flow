set(PYBIND11_CPP_STANDARD -std=c++14)
find_package(pybind11 REQUIRED)

pybind11_add_module(h264flow h264flow.cc)