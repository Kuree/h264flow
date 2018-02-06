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

#include <fstream>
#include "model-io.hh"
#include "../decoder/util.hh"
#include "../query/operator.hh"
#include "../util/filesystem.hh"

#ifdef OPENCV_ENABLED
#include <opencv2/opencv.hpp>
#endif

void dump_mv(const MvFrame &frame, uint32_t label, std::string filename) {
    if (!frame.p_frame()) return;
    std::ofstream stream;
    stream.open(filename.c_str(), std::ios::trunc | std::ios::binary);
    uint32_t size = frame.mb_width() * frame.mb_height();
    uint32_t width = frame.mb_width();
    uint32_t height = frame.mb_height();
    /* write width and height */
    stream.write((char*)&width, sizeof(width));
    stream.write((char*)&height, sizeof(height));
    stream.write((char*)&label, sizeof(label));

    /* dump each struct to the stream */
    for (uint32_t i = 0; i < size; i++) {
        auto mv = frame.get_mv(i);
        stream.write((char*)&mv, sizeof(mv));
    }
    stream.close();
}

std::tuple<MvFrame, uint32_t> load_mv(std::string filename) {
    if (!file_exists(filename))
        throw std::runtime_error(filename + " does not exist");
    std::ifstream stream;
    stream.open(filename, std::ios::binary);

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t label = 0;
    stream.read((char*)&width, sizeof(width));
    stream.read((char*)&height, sizeof(height));
    stream.read((char*)&label, sizeof(label));

    MvFrame frame(width * 16, height * 16, width, height);
    uint32_t size = width * height;
    for (uint32_t i = 0; i < size; i++) {
        MotionVector mv;
        stream.read((char*)&mv, sizeof(mv));
        frame.set_mv(i, mv);
    }
    stream.close();
    return std::make_tuple(frame, label);
}

void dump_processed_mv(const MvFrame &frame, uint32_t label,
                       std::string filename) {
    auto mvs = background_filter(frame);
    std::ofstream stream;
    stream.open(filename, std::ios::binary);
    uint64_t size = mvs.size();
    stream.write((char*)&size, sizeof(size));
    stream.write((char*)&label, sizeof(label));
    for (const auto & mv : mvs)
        stream.write((char*)&mv, sizeof(mv));
    stream.close();
}

double get_angle(const MotionVector & mv) {
    float x = mv.mvL0[0];
    float y = mv.mvL0[1];
    double angle = atan2(y, x);
    if (angle < 0)
        return angle + 2 * M_PI;
    else
        return angle;
}

std::vector<double> load_processed_mv(const std::string & filename,
                                      uint32_t &label) {
    if (!file_exists(filename))
        throw std::runtime_error(filename + " does not exist");
    std::ifstream stream;
    stream.open(filename, std::ios::binary);
    uint64_t size;
    stream.read((char*)&size, sizeof(size));

    stream.read((char*)&label, sizeof(label));

    std::vector<double> angles(size);
    for (uint32_t i = 0; i < size; i++) {
        MotionVector mv;
        stream.read((char*)&mv, sizeof(mv));
        double angle = get_angle(mv);
        angles[i] = angle;
    }

    return angles;
}

uint32_t create_label(bool left, bool right, bool up, bool down,
                      bool zoom_in, bool zoom_out) {
    uint32_t result = 0;
    if (left) result |= 1 << 0;
    if (right) result |= 1 << 1;
    if (up) result |= 1 << 2;
    if (down) result |= 1 << 3;
    if (zoom_in) result |= 1 << 4;
    if (zoom_out)  result |= 1 << 5;
    return result;
}

void load_label(uint32_t label, bool &left, bool &right, bool &up, bool &down,
                bool &zoom_in, bool &zoom_out) {
    left = (label & 1 << 0) != 0;
    right = (label & 1 << 1) != 0;
    up = (label & 1 << 2) != 0;
    down = (label & 1 << 3) != 0;
    zoom_in = (label & 1 << 4) != 0;
    zoom_out = (label & 1 << 5) != 0;
}

void dump_av(std::vector<std::vector<std::pair<int, int>>> & mvs,
             std::vector<uint8_t> &luma, std::string filename) {
    std::ofstream stream;
    stream.open(filename.c_str(), std::ios::trunc | std::ios::binary);

    /* version */
    int const version = 1;
    auto const height = (uint32_t)mvs.size();
    auto const width = (uint32_t)mvs[0].size();

    stream.write((char*)(&version), sizeof(version));
    stream.write((char*)&width, sizeof(width));
    stream.write((char*)&height, sizeof(height));

    /* mvs first */
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            stream.write((char*)&mvs[i][j].first, sizeof(int));
            stream.write((char*)&mvs[i][j].second, sizeof(int));
        }
    }

    /* raw luma */
    const uint8_t * array = &luma[0];
    stream.write((char *)array, luma.size());
}


std::pair<std::vector<std::vector<std::pair<int, int>>>, std::vector<uint8_t>>
load_av(const std::string &filename) {
    if (!file_exists(filename))
        throw std::runtime_error(filename + " does not exist");
    std::ifstream stream;
    stream.open(filename, std::ios::binary);

    uint32_t version = 0;
    uint32_t width = 0;
    uint32_t height = 0;

    stream.read((char*)&version, sizeof(version));
    stream.read((char*)&width, sizeof(width));
    stream.read((char*)&height, sizeof(height));

    if (version != 1)
        throw std::runtime_error("version should be 1");

    /* mvs first */
    std::vector<std::vector<std::pair<int, int>>> mvs =
            std::vector<std::vector<std::pair<int, int>>>(
                    height, std::vector<std::pair<int, int>>(width));
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            int x;
            int y;
            stream.read((char*)&x, sizeof(int));
            stream.read((char*)&y, sizeof(int));
            mvs[i][j].first = x;
            mvs[i][j].second = y;
        }
    }

    std::vector<uint8_t> luma = std::vector<uint8_t>(width * height);
    stream.read((char*)&luma[0], width * height);
    return std::pair(mvs, luma);
}


std::vector<std::vector<std::pair<float, float>>>
load_sintel_flo(const std::string &filename) {
    if (!file_exists(filename))
        throw std::runtime_error(filename + " does not exist");
    std::ifstream stream;
    stream.open(filename, std::ios::binary);

    float tag;
    stream.read((char*)&tag, sizeof(tag));
    if (tag != 202021.25)
        throw std::runtime_error("the machine is not little endian");

    int width, height;
    stream.read((char*)&width, sizeof(width));
    stream.read((char*)&height, sizeof(height));

    if (width < 1 || width > 99999)
        throw std::runtime_error("illegal width");
    if (height < 1 || height > 99999)
        throw std::runtime_error("illegal height");

    std::vector<std::vector<std::pair<float, float>>> result =
            std::vector<std::vector<std::pair<float, float>>>
                    ((uint32_t)height,
                     std::vector<std::pair<float, float>>((uint32_t)width));

    /* raster scan */
    for (int i = 0; i < height; i++) {
        auto row = result[i].data();
        stream.read((char*)row, width * 2 * sizeof(float));
    }
    return result;
}


#ifdef OPENCV_ENABLED
/* visualize the motion vectors */
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
    return std::make_tuple(colors[0], colors[1], colors[2]);
}

std::pair<std::vector<std::vector<int>>, std::set<int>>
load_davis_annotation(const std::string &annotation_file) {
    if (!file_exists(annotation_file))
        throw std::runtime_error(annotation_file + " does not exist");
    cv::Mat image = cv::imread(annotation_file);
    std::vector<std::vector<int>> result =
            std::vector<std::vector<int>>(static_cast<uint32_t>(image.rows),
                                          std::vector<int>(static_cast<uint32_t>
                                                           (image.cols)));
    std::set<int> labels;
    /* access every pixels */
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            cv::Vec3b c = image.at<cv::Vec3b>(i, j);
            /* doesn't matter which order or which type*/
            int color_index = c[0] << 24 | c[1] << 8 | c[2];
            result[i][j] = color_index;
            labels.insert(color_index);
        }
    }

    return std::make_pair(result, labels);
}
#endif