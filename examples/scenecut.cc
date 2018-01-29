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

void output_frame(Mat &frame, VideoWriter &writer, bool write_to_file) {
    if (write_to_file) {
        writer.write(frame);
    }
    else {
        imshow("video", frame);
        waitKey(10);
    }
}

int main(int argc, char *argv[]) {
    ArgParser parser("Scene cut detection");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-t", "threshold", "intra macroblock threshold in P-slice "
                   "(0, 1). default 0.8", false);
    parser.add_arg("-o", "output", "output file", false);
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    std::string output_file;
    float threshold = 0.8;

    VideoCapture capture(filename);
    Mat frame;
    if (!capture.isOpened())
        throw runtime_error("error in opening " + filename);

    /* setup writer */
    VideoWriter writer;
    if (arg_values.find("output") != arg_values.end()) {
        output_file = arg_values["output"];
        auto frame_width = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_WIDTH));
        auto frame_height = static_cast<int>(capture.get(CV_CAP_PROP_FRAME_HEIGHT));
        double fps = capture.get(CV_CAP_PROP_FPS);
        writer = VideoWriter(output_file, 0x21, fps,
                             Size(frame_width, frame_height));
    } else {
        namedWindow("video", WINDOW_AUTOSIZE);
    }

    if(arg_values.find("threshold") != arg_values.end())
        threshold = static_cast<float>(atof(arg_values["threshold"].c_str()));

    /* open decoder */
    h264 decoder(filename);

    vector<MotionRegion> current_mr;

    auto scene_cuts = index_scene_cut(decoder, threshold);

    for (uint32_t frame_counter = 0; frame_counter < scene_cuts.size();
         frame_counter++) {
        capture >> frame;
        if (frame.empty())
            break;

        if (scene_cuts[frame_counter]) {
            Mat mat(frame.rows, frame.cols, frame.type(),
                    Scalar(0, 0, 255));
            float alpha = 0.3;
            addWeighted(mat, alpha, frame, 1 - alpha, 0.0, frame);

            for (int i = 0; i < 20; i++) {
                output_frame(frame, writer, !output_file.empty());
            }
        }
        output_frame(frame, writer, !output_file.empty());
    }

    destroyAllWindows();
    capture.release();
    if (!output_file.empty())
        writer.release();

    return EXIT_SUCCESS;
}
