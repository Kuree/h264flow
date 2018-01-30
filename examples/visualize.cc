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
             vector<MotionRegion> &current_mr, map<uint64_t, uint64_t> &mr_id,
             bool tracking) {
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
            if (tracking) {
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
        }
        /* draw the bbox */
        uint32_t x1, y1, x2, y2;
        tie(x1, y1, x2, y2) = get_bbox(s);
        rectangle(mat, Point(x1, y1), Point(x2, y2), Scalar(0, 255, 0), 2);
    }
}

int main(int argc, char *argv[]) {
    ArgParser parser("Read motion vectors from a media file "
                             "and visualize them");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-m", "median", "median filter size. set to 0 to disable. "
            "default is 0.", false);
    parser.add_arg("-t", "threshold", "threshold to perform motion region partition. "
            "default is 4.", false);
    parser.add_arg("-M", "median_t", "temporal median filter size. set to 0 to disable."
            " default is 0", false);
    parser.add_arg("-T", "tracking", "enable object tracking. set to 0 to disable."
            "default is 1", false);
    parser.add_arg("-o", "output", "output file", false);
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
    uint32_t median_t = 0;
    if (arg_values.find("median_t") != arg_values.end())
        median_t = static_cast<uint32_t>(stoi(arg_values["median_t"]));
    bool obj_tracking = true;
    if (arg_values.find("tracking") != arg_values.end())
        obj_tracking = static_cast<bool>(stoi(arg_values["tracking"]));


    VideoCapture capture(filename);

    string output_filename;
    if (arg_values.find("output") != arg_values.end())
        output_filename = arg_values["output"];
    VideoWriter writer;
    if (!filename.empty()) {
        auto frame_width = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_WIDTH));
        auto frame_height = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_HEIGHT));
        double fps = capture.get(CV_CAP_PROP_FPS);
        writer = VideoWriter(output_filename, 0x21, fps, Size(frame_width, frame_height));
    }

    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    /* motion region information*/
    vector<MotionRegion> pre_mr;
    vector<MotionRegion> current_mr;
    map<uint64_t, uint64_t> mr_id;

    /* by default implement temporal median filter with 3 frames */
    vector<MvFrame> temp_frames;
    uint32_t temp_count = 0;
    bool show_video = output_filename.empty();
    if (show_video)
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
            /* median filter */
            if (median)
                mvs = median_filter(mvs, median);

            /* temporal median filters */
            if (median_t > 0 && temp_frames.size() < median_t) {
                temp_frames.emplace_back(mvs);
            } else if (median_t > 0) {
                temp_frames[temp_count] = mvs;
                temp_count = (temp_count + 1) % 3;
                mvs = median_filter(temp_frames);
            }

            current_mr = mv_partition(mvs, motion_threshold);
            draw_mv(mvs, frame, pre_mr, current_mr, mr_id, obj_tracking);
            pre_mr = current_mr;
        }
        if (show_video) {
            waitKey(10);
            imshow("video", frame);
        } else {
            writer.write(frame);
        }
    }

    if (show_video)
        destroyAllWindows();
    else
        writer.release();

    capture.release();


    return EXIT_SUCCESS;
}
