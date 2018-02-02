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

#include "../ffmpeg/libavflow.hh"
#include "../src/util/argparser.hh"
#include <opencv2/opencv.hpp>

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



int main(int argc, char * argv[]) {
    ArgParser parser("Read motion vectors from a media file and visualize");
    parser.add_arg("-i", "input", "media file input");
    parser.add_arg("-o", "output", "output mv video file");
    if (!parser.parse(argc, argv))
        return EXIT_FAILURE;
    auto arg_values = parser.get_args();

    string filename = arg_values["input"];
    string output_file = arg_values["output"];

    Mat frame;
    bool has_initialized = false;
    VideoWriter writer;

    LibAvFlow flow(filename);
    while (true) {
        auto result = flow.get_mv();
        if (result.empty())
            break;
        auto frame_height = (int)result.size();
        auto frame_width = (int)result[0].size();
        if (!has_initialized) {
            writer = VideoWriter(output_file, 0x21, 24, Size(frame_width, frame_height));
            has_initialized = true;
        }
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
                mat.at<Vec3b>(i, j) = {get<2>(color), get<1>(color), get<0>(color)};
            }
        }
        writer.write(mat);
    }
    writer.release();

    return EXIT_SUCCESS;
}