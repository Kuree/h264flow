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
#include "../decoder/h264.hh"

namespace py = pybind11;


void init_h264(py::module &m) {
    py::class_<h264>(m, "h264").def(py::init<const std::string &>())
        .def("load_frame", &h264::load_frame)
        .def("index_size", &h264::index_size)
        .def("index_nal", &h264::index_nal);
}

void init_mv_frame(py::module &m) {
    py::class_<MvFrame>(m, "h264");
}
