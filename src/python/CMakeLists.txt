set(PYBIND11_CPP_STANDARD -std=c++14)
find_package(pybind11)
if (PYBIND11_FIND)
    pybind11_add_module(h264flow h264flow.cc)

    target_link_libraries(h264flow PRIVATE pybind11::module)
    set_target_properties(h264flow PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
        SUFFIX "${PYTHON_MODULE_EXTENSION}")
endif()