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
#include "../ffmpeg/libavflow.hh"
#include "../src/util/argparser.hh"
#include "../src/model/model-io.hh"
#include "../src/util/filesystem.hh"

using namespace std;
using namespace cv;


template<typename T>
Mat draw_mv(const vector<vector<pair<T, T>>> &result) {
    auto frame_height = (int) result.size();
    auto frame_width = (int) result[0].size();
    /* compute max and min */
    double max_rad = -1;
    for (int i = 0; i < frame_height; i++) {
        for (int j = 0; j < frame_width; j++) {
            double fx = result[i][j].first;
            double fy = result[i][j].second;
            double rad = sqrt(fx * fx + fy * fy);
            max_rad = max<double>(rad, max_rad);
        }
    }
    if (max_rad == 0) max_rad = 1;

    Mat mat(frame_height, frame_width, CV_8UC3);
    for (int i = 0; i < frame_height; i++) {
        for (int j = 0; j < frame_width; j++) {
            auto color = compute_color(result[i][j].first / max_rad,
                                       result[i][j].second / max_rad);
            mat.at<Vec3b>(i, j) = {get<2>(color), get<1>(color),
                                   get<0>(color)};
        }
    }
    return mat;
}

int main(int argc, char * argv[]) {
    ArgParser parser("Read motion vectors from a media file and visualize");
    parser.add_arg("-i", "input", "media file input", false);
    parser.add_arg("-f", "flo", "flow file dir. if present, it will override "
            "all the other inputs, that is, it will only visualize the flow "
            "files", false);
    parser.add_arg("-o", "output", "output folder for raw data", false);
    parser.add_arg("-v", "visualize", "output visualized video file. "
            "if present, it will disable raw data dump", false);
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    string output_dir;
    if (arg_values.find("output") != arg_values.end()) {
        output_dir = arg_values["output"];
        if (!dir_exists(output_dir))
            throw std::runtime_error(output_dir + " does not exist");
    }
    string output_file;
    if (arg_values.find("visualize") != arg_values.end())
        output_file = arg_values["visualize"];
    bool write_video = false;
    bool dump_mv = true;
    if (!output_file.empty()) {
        dump_mv = false;
        write_video = true;
    }

    if (!write_video && !dump_mv) {
        parser.print_help(argv[0]);
        return EXIT_FAILURE;
    }

    string flow_dir;
    bool read_flo_file = false;
    if (arg_values.find("flo") != arg_values.end()) {
        flow_dir = arg_values["flo"];
        dump_mv = false;
        write_video = false;
        read_flo_file = true;
    }


    Mat frame;
    if (!read_flo_file) {
        VideoWriter writer;
        LibAvFlow flow(filename);

        if (write_video)
            writer = VideoWriter(output_file, 0x21, 25, Size(flow.width(),
                                                             flow.height()));

        while (flow.current_frame_num() < flow.total_frames()) {
            /* get raw frame*/
            if (!flow.decode_frame())
                break;
            auto mvs = flow.get_mv();
            if (mvs.empty())
                continue;

            if (write_video) {
                auto mat = draw_mv<int>(mvs);
                writer.write(mat);
                waitKey(10);
            }
            if (dump_mv) {
                auto luma = flow.get_luma();
                if (flow.current_frame_num() > 10000)
                    break;
                char buf[120];
                std::snprintf(buf, 120, "%s/%04d.frame", output_dir.c_str(),
                              flow.current_frame_num());
                dump_av(mvs, luma, buf);
            }
        }
        if (write_video)
            writer.release();
    } else {
        /* only visualize the flo file */
        if (!dir_exists(flow_dir))
            throw std::runtime_error(flow_dir + " does not exist");
        auto flo_files = get_files(flow_dir, ".flo");
        /* frame num comparator */
        auto cmp = [] (const std::string &entry1, const std::string &entry2) {
            /* made an assumption about how it's named */
            std::string name1 = entry1.substr(6, 4);
            std::string name2 = entry2.substr(6, 4);
            int num1 = stoi(name1);
            int num2 = stoi(name2);
            return num1 < num2;
        };
        std::priority_queue<std::string, std::vector<std::string>,
                decltype(cmp)> queue(cmp);
        for (auto const & name : flo_files) {
            queue.push(name);
        }
        namedWindow("video");
        while (!queue.empty()) {
            std::string name = queue.top();
            queue.pop();
            auto mvs = load_sintel_flo(flow_dir + "/" +  name);
            auto mat = draw_mv<float>(mvs);
            imshow("video", mat);
            waitKey(10);
        }

    }
    return EXIT_SUCCESS;
}