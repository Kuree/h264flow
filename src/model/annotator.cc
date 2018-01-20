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
#include "../model/model-io.hh"
#include <functional>
#include "../decoder/h264.hh"

using namespace cv;
using namespace std;

using Action = std::function<void(int)>;

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
    int pre_frame_num = frame_num;
    auto total_frame_num = (int)decoder->index_size();

    namedWindow("main", CV_WINDOW_NORMAL);
    createTrackbar("track", "main", &frame_num, total_frame_num);


    /* control values */
    bool pause = false;
    bool camera_left = false;
    bool camera_right = false;
    bool camera_up = false;
    bool camera_down = false;
    bool camera_zoom_in = false;
    bool camera_zoom_out = false;
    MvFrame * mv_frame = nullptr;

    Action pause_action = [&] (int) { pause = !pause; };
    Action left_action = [&] (bool state) { camera_left = state; };
    Action right_action = [&] (bool state) { camera_right = state; };
    Action up_action = [&] (bool state) { camera_up = state; };
    Action down_action = [&] (bool state) { camera_down = state; };
    Action zoom_in_action = [&] (bool state) { camera_zoom_in = state; };
    Action zoom_out_action = [&] (bool state) { camera_zoom_out = state; };
    Action save_action = [&] (int) {
        if (mv_frame) {
            /* dump the frame_num */
            /* TODO: fix this */
            string output_filename = output_dir + "/" + prefix + "_" +
                    to_string(frame_num) + ".mv";
            uint32_t label = create_label(camera_left, camera_right, camera_up,
                                          camera_down, camera_zoom_in,
                                          camera_zoom_out);
            dump_mv(*mv_frame, label, output_filename);
        }
    };

    cv::ButtonCallback btn_callback = [] (int state, void* user_data) {
        (*(Action*)user_data)(state);
    };

    createButton("pause", btn_callback, &pause_action,
                 QT_PUSH_BUTTON, 0);
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

        if (pre_frame_num != frame_num -1
            && !capture.set(CV_CAP_PROP_POS_MSEC, frame_num))
            cerr << "failed to set frame number " << frame_num << endl;
        capture >> frame;
        if (frame.empty())
            break;
        MvFrame mvs = decoder->load_frame((uint32_t)frame_num);
        mv_frame = &mvs;
        imshow("main", frame);

        pre_frame_num = frame_num;
        frame_num++;
        setTrackbarPos("track", "main", frame_num);
        waitKey(10);
    }

    destroyAllWindows();
    capture.release();
    return EXIT_SUCCESS;
}