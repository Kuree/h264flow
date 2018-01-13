/*  This file is part of h264flow.

    h264flow is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    h264flow is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with h264flow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "../src/decoder/h264.hh"

namespace py = pybind11;


void init_h264(py::module &m) {
    py::class_<h264>(m, "h264").def(py::init<const std::string &>())
        .def("load_frame", &h264::load_frame)
        .def("index_size", &h264::index_size)
        .def("index_nal", &h264::index_nal);
}

void init_mv_frame(py::module &m) {
    py::class_<MvFrame>(m, "MvFrame").def(py::init<uint32_t, uint32_t, uint32_t, uint32_t>())
            .def("get_mv", (MotionVector (MvFrame::*)(uint32_t, uint32_t))&MvFrame::get_mv)
            .def("width", &MvFrame::width)
            .def("height", &MvFrame::height);
}

void init_mv(py::module &m) {
    py::class_<MotionVector>(m, "MotionVector")
            .def("motion_distance_L0", &MotionVector::motion_distance_L0)
            .def_property_readonly("mvL0", [](MotionVector &mv) { return py::array(2, mv.mvL0);})
            .def_property_readonly("mvL1", [](MotionVector &mv) { return py::array(2, mv.mvL1);})
            //.def_property_readonly("mvL1", [](const MotionVector& mv) { return &mv.mvL1; })
            .def_readwrite("x", &MotionVector::x)
            .def_readwrite("y", &MotionVector::y);
}

PYBIND11_PLUGIN(h264flow) {
    py::module m("h264flow", "h264flow python binding");
    init_h264(m);
    init_mv_frame(m);
    init_mv(m);
    return m.ptr();
}
