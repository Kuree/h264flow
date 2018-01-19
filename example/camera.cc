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
#include "../src/decoder/h264.hh"

using namespace cv;
using namespace std;


void draw_text(Mat &mat, const string &type, bool value, int &y) {
    int x = mat.cols >= 120 ? mat.cols / 2 - 60 : 0;
    putText(mat, type + (value ? " True" :" False"), Point(x, y),
            FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 0), 4);
    y += 30;
}

void draw_mv(MvFrame &mvs, Mat &mat) {
    /* draw the motion region */
    std::vector<std::set<MotionVector>> regions = mv_partition(mvs, 4);

    /* compute the camera movement and print it out to the frame */
    auto cam_result = CategorizeCameraMotion(mvs, regions, 0.4);
    int y = mat.rows / 2 - 75;
    draw_text(mat, "No motion", cam_result[MotionType::NoMotion], y);
    draw_text(mat, "Move right", cam_result[MotionType::TranslationRight], y);
    draw_text(mat, "Move left", cam_result[MotionType::TranslationLeft], y);
    draw_text(mat, "Move up", cam_result[MotionType::TranslationUp], y);
    draw_text(mat, "Move down", cam_result[MotionType::TranslationDown], y);
    draw_text(mat, "Zoom", cam_result[MotionType::Zoom], y);
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

    namedWindow("video", WINDOW_AUTOSIZE);
    uint32_t frame_counter = 0;
    for (; frame_counter < decoder->index_size(); frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;
        MvFrame mvs = decoder->load_frame(frame_counter);
        /* median filter */
        //mvs = median_filter(mvs, 3);
        draw_mv(mvs, frame);
        imshow("video", frame);
        waitKey(50);
    }
    return EXIT_SUCCESS;
}