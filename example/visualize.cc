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

void draw_mv(MvFrame &mvs, Mat &mat, vector<MotionRegion> &pre_mr,
             vector<MotionRegion> &current_mr, map<uint64_t, uint64_t> &mr_id) {
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

    /* draw the motion region */
    auto region_matches = match_motion_region(current_mr, pre_mr, 50);
    for (const auto &s : current_mr) {
        for (const auto &p : s.mvs) {
            if (p.x + 16 > (uint32_t)mat.cols || p.y + 16 > (uint32_t)mat.rows)
                continue;
            Mat roi = mat(Rect(p.x, p.y, 16, 16));
            cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(0, 255, 0));
            float alpha = 0.3;
            addWeighted(color, alpha, roi, 1 - alpha, 0.0, roi);
            /* notice that in the region matches, it's arg1 -> arg2,
             * so we are good */
            if (region_matches.find(s.id) != region_matches.end()) {
                uint64_t old_id = region_matches[s.id];
                uint64_t id = 0;
                if (mr_id.find(old_id) != mr_id.end()) {
                    id = mr_id[old_id];
                    //mr_id.erase(old_id);
                    mr_id.insert({s.id, id});
                } else {
                    id = mr_id.size();
                    mr_id.insert({s.id, id});
                }
                /* put id label on centroid */
                Point pt(static_cast<int>(s.x), static_cast<int>(s.y));
                putText(mat, "ID " + to_string(id), pt,
                        FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0), 4);
            }
        }
        /* draw the bbox */
        uint32_t x1, y1, x2, y2;
        tie(x1, y1, x2, y2) = get_bbox(s);
        rectangle(mat, Point(x1, y1), Point(x2, y2), Scalar(0, 255, 0), 2);
    }
}

int main(int argc, char *argv[]) {
    ArgParser parser("Read motion vectors from a media file "
                             "and print them out");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-m", "median", "median filter size. set to 0 to disable. "
            "default is 0.", false);
    parser.add_arg("-t", "threshold", "threshold to perform motion region partition. "
            "default is 4.", false);
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    uint32_t median = 0;
    if (arg_values.find("median") != arg_values.end())
        median = (uint32_t)stoi(arg_values["median"]);
    uint32_t motion_threshold = 4;
    if (arg_values.find("threshold") != arg_values.end())
        motion_threshold = (uint32_t)stoi(arg_values["threshold"]);

    VideoCapture capture(filename);
    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    /* motion region information*/
    vector<MotionRegion> pre_mr;
    vector<MotionRegion> current_mr;
    map<uint64_t, uint64_t> mr_id;

    namedWindow("video", WINDOW_AUTOSIZE);
    uint32_t frame_counter = 0;
    for (; frame_counter < decoder->index_size(); frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;
        MvFrame mvs = decoder->load_frame(frame_counter);
        /* median filter */
        if (median)
            mvs = median_filter(mvs, median);

        current_mr = mv_partition(mvs, motion_threshold);
        draw_mv(mvs, frame, pre_mr, current_mr, mr_id);
        pre_mr = current_mr;

        imshow("video", frame);
        waitKey(10);
    }

    destroyAllWindows();
    capture.release();

    return EXIT_SUCCESS;
}
