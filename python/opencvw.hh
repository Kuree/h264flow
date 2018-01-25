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

#ifndef H264FLOW_OPENCVW_HH
#define H264FLOW_OPENCVW_HH
/* an header only wrapper for python to get frames without installing opencv */
#ifdef OPENCV_ENABLED

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class OpenCV {
public:
    explicit OpenCV(const std::string &filename);
    cv::Mat get_frame(int frame_num);
    cv::Mat get_frame() { return get_frame(current_frame_ + 1); }
    ~OpenCV();

private:
    cv::VideoCapture capture_;
    int current_frame_ = -1;
};

OpenCV::OpenCV(const std::string &filename) : capture_(filename) {
    if (!capture_.isOpened())
        throw std::runtime_error("error in opening " + filename);
}

OpenCV::~OpenCV() {
    capture_.release();
}

cv::Mat OpenCV::get_frame(int frame_num) {
    cv::Mat frame;
    if (frame_num != current_frame_ + 1) {
        if (!capture_.set(CV_CAP_PROP_POS_FRAMES, frame_num))
           throw std::runtime_error("failed to set frame number "  +
                                            std::to_string(frame_num));
    }
    current_frame_ = frame_num;
    capture_ >> frame;
    return frame;
}
#endif

#endif //H264FLOW_OPENCVW_HH
