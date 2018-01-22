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

#include <opencv2/opencv.hpp>
#include <mutex>
#include "../model/model-io.hh"

using namespace cv;
using namespace std;

using Action = std::function<void(int)>;

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
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << "<media_file> <output_dir> <prefix>"
             << endl;
        return EXIT_FAILURE;
    }

    string filename = argv[1];
    string output_dir = argv[2];
    string prefix = argv[3];
    VideoCapture capture(filename);
    Mat frame;

    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* open decoder */
    unique_ptr<h264> decoder = make_unique<h264>(filename);

    int frame_num = 0;
    int pts = 0;
    bool has_seek = false;
    auto total_frame_num = (int)decoder->index_size();

    namedWindow("main", CV_WINDOW_AUTOSIZE);
    cv::TrackbarCallback track_callback = [] (int state, void* user_data) {
        (*(Action*)user_data)(state);
    };
    Action seek = [&] (int) {
        if (pts < frame_num - 5 || pts > frame_num + 5) {
            has_seek = true;
            pts = frame_num;
        }
    };
    createTrackbar("track", "main", &frame_num, total_frame_num, track_callback,
                   &seek);

    /* control values */
    bool pause = false;
    bool camera_left = false;
    bool camera_right = false;
    bool camera_up = false;
    bool camera_down = false;
    bool camera_zoom_in = false;
    bool camera_zoom_out = false;
    MvFrame mv_frame;

    Action pause_action = [&] (int) { pause = !pause; };
    Action left_action = [&] (bool state) { camera_left = state; };
    Action right_action = [&] (bool state) { camera_right = state; };
    Action up_action = [&] (bool state) { camera_up = state; };
    Action down_action = [&] (bool state) { camera_down = state; };
    Action zoom_in_action = [&] (bool state) { camera_zoom_in = state; };
    Action zoom_out_action = [&] (bool state) { camera_zoom_out = state; };
    Action save_action = [&] (int) {
        if (mv_frame.mb_height() > 0) {
            /* dump the frame_num */
            /* TODO: fix this */
            string output_filename = output_dir + "/" + prefix + "_" +
                    to_string(frame_num) + ".mv";
            uint32_t label = create_label(camera_left, camera_right, camera_up,
                                          camera_down, camera_zoom_in,
                                          camera_zoom_out);
            dump_mv(mv_frame, label, output_filename);
        }
    };

    cv::ButtonCallback btn_callback = [] (int state, void* user_data) {
        (*(Action*)user_data)(state);
    };

    createButton("pause", btn_callback, &pause_action,
                 QT_PUSH_BUTTON, false);
    createButton("camera up", btn_callback, &up_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("camera down", btn_callback, &down_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("camera left", btn_callback, &left_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("camera right", btn_callback, &right_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("camera zoom in", btn_callback, &zoom_in_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("camera zoom out", btn_callback, &zoom_out_action, QT_CHECKBOX | QT_NEW_BUTTONBAR);
    createButton("save", btn_callback, &save_action, QT_PUSH_BUTTON | QT_NEW_BUTTONBAR);

    while (frame_num < total_frame_num) {
        if (pause) {
            waitKey(50);
            continue;
        }

        /* new frame, reset everything */
        camera_down = false;
        camera_left = false;
        camera_right = false;
        camera_up = false;
        camera_zoom_in = false;
        camera_zoom_out = false;

        if (has_seek) {
            if (!capture.set(CV_CAP_PROP_POS_FRAMES, frame_num))
                cerr << "failed to set frame number " << frame_num << endl;
            has_seek = false;
        }
        capture >> frame;
        if (frame.empty())
            break;
        MvFrame mvs = decoder->load_frame((uint32_t)frame_num);
        mv_frame = MvFrame(mvs); /* copy constructor */
        draw_mv(mvs, frame);
        imshow("main", frame);

        frame_num++;
        pts++;
        setTrackbarPos("track", "main", frame_num);
        waitKey(10);
    }

    destroyAllWindows();
    capture.release();
    return EXIT_SUCCESS;
}