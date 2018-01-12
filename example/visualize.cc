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

#include "../src/decoder/h264.hh"
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

void draw_mv(shared_ptr<MvFrame> mvs, Mat & mat) {
    if (!mvs) return;
    for (uint32_t y = 0; y < mvs->mb_height(); y++) {
        for (uint32_t x = 0; x < mvs->mb_width(); x++) {
            auto mv = mvs->get_mv(x, y);
            Rect rect(x * 16, y * 16, 16, 16);
            uint8_t color = uint8_t((abs(mv.mvL0[0]) + abs(mv.mvL0[1])) * 10);
            if (color > 0)
                rectangle(mat, rect, Scalar(color, color, color), CV_FILLED);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return EXIT_FAILURE;
    }
    string filename = argv[1];
    VideoCapture capture(filename);
    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    namedWindow("video", 1);
    namedWindow("mv", 2);
    uint32_t frame_counter = 0;
    for (; ; frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;
        imshow("video", frame);
        shared_ptr<MvFrame> mvs;
        try {
            mvs = decoder->load_frame(frame_counter);
        } catch (NotImplemented & ex) {
            mvs = make_shared<MvFrame>(frame.cols, frame.rows, frame.cols / 16, frame.rows / 16);
        }
        Mat mv_frame = Mat::zeros(frame.rows, frame.cols, CV_8UC3);
        draw_mv(mvs, mv_frame);
        imshow("mv", mv_frame);
        waitKey(20);
    }
    return EXIT_SUCCESS;
}