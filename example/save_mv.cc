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

#include <algorithm>
#include <opencv2/opencv.hpp>
#include "../src/query/operator.hh"
#include "../src/util/argparser.hh"

using namespace cv;
using namespace std;

void draw_mv(MvFrame &mvs, Mat &mat) {
    for (uint32_t y = 0; y < mvs.mb_height(); y++) {
        for (uint32_t x = 0; x < mvs.mb_width(); x++) {
            auto mv = mvs.get_mv(x, y);
            /* convert from block unit to pixel unit */
            auto mv_x = (int)mv.mvL0[0];
            auto mv_y = (int)mv.mvL0[1];
            uint32_t start_x = x * 16 + 8;
            uint32_t start_y = y * 16 + 8;
            double norm = sqrt(mv_x * mv_x + mv_y + mv_y);
            if (norm > 0) {
                auto red = (uint8_t) ((norm / 25.0 * 255) < 255 ?
                                      (norm / 25.0 * 255) : 255);
                /* draw the vector */
                arrowedLine(mat, Point(start_x, start_y),
                            Point(start_x + mv_x, start_y + mv_y),
                            Scalar(255 - red, 0, red), 2);
            } else {
                /* draw a dot instead */
                circle(mat, Point(start_x, start_y), 1, Scalar(128, 128, 128),
                       CV_FILLED);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    ArgParser parser("Read motion vectors from a media file "
                             "and visualize them");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-o", "output", "output file");
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    string output_file = arg_values["output"];

    VideoCapture capture(filename);
    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    auto frame_width = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_WIDTH));
    auto frame_height = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_HEIGHT));
    double fps = capture.get(CV_CAP_PROP_FPS);
    VideoWriter writer(output_file, 0x21, fps, Size(frame_width, frame_height));
    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);


    for (uint32_t frame_counter = 0; frame_counter < decoder->index_size();
         frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;
        MvFrame mvs;
        tie(mvs, ignore) = decoder->load_frame(frame_counter);

        draw_mv(mvs, frame);
        writer.write(frame);
    }

    writer.release();
    capture.release();

    return EXIT_SUCCESS;
}
