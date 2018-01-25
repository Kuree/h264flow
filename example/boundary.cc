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

#define BAND 100

void draw_mv(MvFrame &mvs, Mat &mat, double threshold, uint32_t s_threshold) {
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

    auto top_region = crop_frame(mvs, 0, 0, mvs.width(), BAND);
    auto left_region = crop_frame(mvs, 0, 0, BAND, mvs.height());
    auto bottom_region = crop_frame(mvs, 0, mvs.height() - BAND,
                                    mvs.width(), BAND);
    auto right_region = crop_frame(mvs, mvs.width() - BAND, 0, BAND,
                                   mvs.height());

    vector<MotionRegion> regions;
    if (motion_in_frame(top_region, threshold, s_threshold)) {
        auto r = mv_partition(top_region, threshold, s_threshold);
        regions.insert(regions.end(), r.begin(), r.end());
    }
    if (motion_in_frame(left_region, threshold, s_threshold)) {
        auto r = mv_partition(left_region, threshold, s_threshold);
        regions.insert(regions.end(), r.begin(), r.end());
    }
    if (motion_in_frame(bottom_region, threshold, s_threshold)) {
        auto r = mv_partition(bottom_region, threshold, s_threshold);
        regions.insert(regions.end(), r.begin(), r.end());
    }
    if (motion_in_frame(right_region, threshold, s_threshold)) {
        auto r = mv_partition(right_region, threshold, s_threshold);
        regions.insert(regions.end(), r.begin(), r.end());
    }

    /* draw the motion region */
    for (const auto &s : regions) {
        for (const auto &p : s.mvs) {
            if (p.x + 16 > (uint32_t)mat.cols || p.y + 16 > (uint32_t)mat.rows)
                continue;
            Mat roi = mat(Rect(p.x, p.y, 16, 16));
            cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(0, 255, 0));
            float alpha = 0.3;
            addWeighted(color, alpha, roi, 1 - alpha, 0.0, roi);
        }
    }
}

int main(int argc, char *argv[]) {
    ArgParser parser("Check movement in the boundrary");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-t", "threshold", "energy threshold (x^2 + y^2)."
            "default is 4.", false);
    parser.add_arg("-st", "size_threshold", "size threshold in motion region"
            "partition. default is 1.", false);
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    uint32_t motion_threshold = 4;
    if (arg_values.find("threshold") != arg_values.end())
        motion_threshold = (uint32_t)stoi(arg_values["threshold"]);
    uint32_t size_threshold = 0;
    if (arg_values.find("size_threshold") != arg_values.end())
        size_threshold = static_cast<uint32_t>(
                stoi(arg_values["size_threshold"]));

    VideoCapture capture(filename);
    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    vector<MotionRegion> current_mr;

    namedWindow("video", WINDOW_AUTOSIZE);
    uint32_t frame_counter = 0;
    for (; frame_counter < decoder->index_size(); frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;
        bool p_slice;
        MvFrame mvs;
        tie(mvs, p_slice) = decoder->load_frame(frame_counter);
        if (p_slice) {
            current_mr = mv_partition(mvs, motion_threshold);
            draw_mv(mvs, frame, motion_threshold, size_threshold);
        }
        imshow("video", frame);
        waitKey(10);
    }

    destroyAllWindows();
    capture.release();

    return EXIT_SUCCESS;
}
