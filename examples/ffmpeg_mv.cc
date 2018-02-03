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

/* taken from MPI Sintel */

int ncols = 0;
#define MAXCOLS 60
int colorwheel[MAXCOLS][3];

void set_cols(int r, int g, int b, int k)
{
    colorwheel[k][0] = r;
    colorwheel[k][1] = g;
    colorwheel[k][2] = b;
}

void make_color_wheel()
{
    // relative lengths of color transitions:
    // these are chosen based on perceptual similarity
    // (e.g. one can distinguish more shades between red and yellow
    //  than between yellow and green)
    int RY = 15;
    int YG = 6;
    int GC = 4;
    int CB = 11;
    int BM = 13;
    int MR = 6;
    ncols = RY + YG + GC + CB + BM + MR;
    //printf("ncols = %d\n", ncols);
    if (ncols > MAXCOLS)
        exit(1);
    int i;
    int k = 0;
    for (i = 0; i < RY; i++) set_cols(255, 255 * i / RY, 0, k++);
    for (i = 0; i < YG; i++) set_cols(255 - 255 * i / YG, 255, 0, k++);
    for (i = 0; i < GC; i++) set_cols(0, 255, 255 * i / GC, k++);
    for (i = 0; i < CB; i++) set_cols(0, 255 - 255 * i / CB, 255, k++);
    for (i = 0; i < BM; i++) set_cols(255 * i / BM, 0, 255, k++);
    for (i = 0; i < MR; i++) set_cols(255, 0, 255 - 255 * i / MR, k++);
}

std::tuple<uint8_t, uint8_t, uint8_t> compute_color(double fx, double fy)
{
    if (ncols == 0)
        make_color_wheel();

    double rad = sqrt(fx * fx + fy * fy);
    double a = atan2(-fy, -fx) / M_PI;
    double fk = (a + 1.0) / 2.0 * (ncols-1);
    auto k0 = static_cast<int>(fk);
    int k1 = (k0 + 1) % ncols;
    double f = fk - k0;
    uint8_t colors[3];
    for (int b = 0; b < 3; b++) {
        double col0 = colorwheel[k0][b] / 255.0;
        double col1 = colorwheel[k1][b] / 255.0;
        double col = (1 - f) * col0 + f * col1;
        if (rad <= 1)
            col = 1 - rad * (1 - col); // increase saturation with radius
        else
            col *= .75; // out of range
        colors[2 - b] = static_cast<uint8_t>(255.0 * col);
    }
    return make_tuple(colors[0], colors[1], colors[2]);
}

void draw_mv(VideoWriter &writer,
             const vector<vector<pair<int, int>>> &result) {
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
    writer.write(mat);
    waitKey(10);
}

int main(int argc, char * argv[]) {
    ArgParser parser("Read motion vectors from a media file and visualize");
    parser.add_arg("-i", "input", "media file input");
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


    Mat frame;
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
            draw_mv(writer, mvs);
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

    return EXIT_SUCCESS;
}