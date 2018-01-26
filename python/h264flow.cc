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
#include <pybind11/stl.h>
#include "../src/decoder/h264.hh"
#include "../src/query/operator.hh"
#include "../src/model/model-io.hh"

#ifdef OPENCV_ENABLED
#include "opencvw.hh"
#endif

namespace py = pybind11;


void init_h264(py::module &m) {
    py::class_<h264>(m, "h264").def(py::init<const std::string &>())
        .def("load_frame", &h264::load_frame)
        .def("index_size", &h264::index_size)
        .def("index_nal", &h264::index_nal);
}

void init_mv_frame(py::module &m) {
    py::class_<MvFrame>(m, "MvFrame").def(py::init<uint32_t, uint32_t, uint32_t, uint32_t>())
            .def("get_mv", (MotionVector (MvFrame::*)(uint32_t, uint32_t) const)&MvFrame::get_mv)
            .def("width", &MvFrame::width)
            .def("height", &MvFrame::height)
            .def_property_readonly("mvL0", [](MvFrame &mv) {
                auto lst = py::list();
                for (uint32_t i = 0; i < mv.height() / 16; i++) {
                    auto row = mv[i];
                    auto row_list = py::list();
                    for (uint32_t j = 0; j < row.size(); j++)
                        row_list.append(py::array(2, row[j].mvL0));
                    lst.append(row_list);
                }
                return py::array(lst);
            })
            .def_property_readonly("p_frame", [](MvFrame &mv)
            { return mv.p_frame(); });
}

void init_mv(py::module &m) {
    py::class_<MotionVector>(m, "MotionVector")
            .def_property_readonly("mvL0", [](MotionVector &mv)
            { return py::array(2, mv.mvL0);})
            .def_readwrite("x", &MotionVector::x)
            .def_readwrite("y", &MotionVector::y);
}

void init_op(py::module &m) {
    py::class_<Operator>(m, "Operator").def(py::init<Operator&>())
            .def(py::init<MvFrame>())
            .def("execute", &Operator::execute)
            .def("get_frame", &Operator::get_frame);
    py::class_<ThresholdOperator>(m, "ThresholdOperator")
            .def(py::init<uint32_t, Operator&>())
            .def(py::init<uint32_t, CropOperator&>())
            .def(py::init<uint32_t, MvFrame>())
            .def("execute", &ThresholdOperator::execute)
            .def("result", &ThresholdOperator::result);
    py::class_<CropOperator>(m, "CropOperator")
            .def(py::init<uint32_t, uint32_t, uint32_t, uint32_t, Operator&>())
            .def(py::init<uint32_t, uint32_t, uint32_t, uint32_t, MvFrame>())
            .def("execute", &ReduceOperator::execute);

    py::class_<MotionRegion>(m, "MotionRegion")
            .def_readwrite("x", &MotionRegion::x)
            .def_readwrite("y", &MotionRegion::y);

    m.def("motion_in_frame", &motion_in_frame);
    m.def("crop_frame", &crop_frame);
    m.def("frames_without_motion", &frames_without_motion);
    m.def("mv_partition", &mv_partition);
    m.def("get_bbox", &get_bbox);
}

void init_model(py::module &m) {
    m.def("load_mv", &load_mv);
    m.def("dump_mv", &dump_mv);
    m.def("create_mv", [](py::array_t<float> array) {
        auto shape = array.shape();
        if (shape[2] != 2)
            throw std::runtime_error("array has to be a vector field");
        auto matrix = array.mutable_unchecked<3>();
        auto height = (uint32_t)shape[0];
        auto width = (uint32_t)shape[1];
        MvFrame frame(width * 16, height * 16, width, height, false);
        for (uint32_t i = 0; i < height; i++) {
            for (uint32_t j = 0; j < width; j++) {
                auto x = matrix(i, j, 0);
                auto y = matrix(i, j, 1);
                MotionVector mv {
                        x,
                        y,
                        j * 16,
                        i * 16,
                        (uint32_t)(x * x + y * y)
                };
                frame.set_mv(j, i, mv);
            }
        }
        return frame;
    });
}

#ifdef OPENCV_ENABLED
using ShapeContainer = py::detail::any_container<ssize_t>;
void init_opencv(py::module &m) {
    py::class_<OpenCV>(m, "OpenCV").def(py::init<const std::string &>())
            .def("get_next_frame", [](OpenCV& opencv, int frame_num){
                cv::Mat mat = opencv.get_frame(frame_num);
                ShapeContainer shape({mat.rows, mat.cols, mat.channels()});
                return py::array(shape, mat.data);
            });
}

#endif

PYBIND11_PLUGIN(h264flow) {
    py::module m("h264flow", "h264flow python binding");
    init_h264(m);
    init_mv_frame(m);
    init_mv(m);
    init_op(m);
    init_model(m);
#ifdef OPENCV_ENABLED
    init_model(m);
#endif
    return m.ptr();
}
