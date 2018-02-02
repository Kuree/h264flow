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

void dump_av(const std::vector<std::vector<std::pair<int, int>>> & mvs,
             const std::vector<uint8_t> &luma, std::string filename) {
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