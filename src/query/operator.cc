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
#include <math.h>
#include "operator.hh"

void ThresholdOperator::execute() {
    BooleanOperator::execute();
    for (uint32_t y = 0; y < _frame.mb_height(); y++) {
        for (uint32_t x = 0; x < _frame.mb_width(); x++) {
            auto mv = _frame.get_mv(x, y);
            if (mv.motion_distance_L0() > _threshold) {
                _result = true;
                return;
            }
        }
    }
}

void CropOperator::reduce() {
    MvFrame frame(_width, _height, 16, 16);
    for (uint32_t y = 0; y < _height; y++) {
        for (uint32_t x = 0; x < _width; x++) {
            frame.set_mv(x, y, _frame.get_mv(x + _x, y + _y));
        }
    }
    _frame = frame;
}

MvFrame median_filter(MvFrame &frame, uint32_t size) {
    MvFrame result = MvFrame(frame.width(), frame.height(), frame.mb_width(),
                             frame.mb_height());
    uint32_t median_element = size / 2;
    for (uint32_t i = median_element;
         i < frame.mb_height() - median_element; i++) {
        std::vector<MotionVector> mv_row = frame.get_row(i);
        for (uint32_t j = median_element;
             j < frame.mb_width() - median_element; j++) {
            /* TODO: optimize this */
            std::vector<int> mv0 = std::vector<int>(size * size - 1);
            std::vector<int> mv1 = std::vector<int>(size * size - 1);
            uint32_t counter = 0;
            for (uint32_t k = 0; k < size * size; k++) {
                int row = k / size - median_element;
                int col = k % size - median_element;
                if (row == 0 && col == 0)
                    continue;
                mv0[counter] = frame.get_mv(j + col, i + row).mvL0[0];
                mv1[counter] = frame.get_mv(j + col, i + row).mvL0[1];
                ++counter;
            }
            std::nth_element(mv0.begin(), mv0.begin() + median_element,
                             mv0.end());
            std::nth_element(mv1.begin(), mv1.begin() + median_element,
                             mv1.end());
            MotionVector mv;
            mv.mvL0[0] = mv0[median_element];
            mv.mvL0[1] = mv1[median_element];
            result.set_mv(j, i, mv);
        }
    }
    return result;
}

MvFrame horizontal_filter(MvFrame &frame) {
    MvFrame result = MvFrame(frame.width(), frame.height(), frame.mb_width(),
                             frame.mb_height());
    for (uint32_t i = 0; i < frame.mb_height(); i++) {
        std::vector<MotionVector> mv_row = frame.get_row(i);
        for (uint32_t j = 0; j < frame.mb_width(); j++) {
            MotionVector mv;
            mv.mvL0[0] = mv.mvL0[0];
            result.set_mv(j, i, mv);
        }
    }
    return result;
}

MvFrame vertical_filter(MvFrame &frame) {
    MvFrame result = MvFrame(frame.width(), frame.height(), frame.mb_width(),
                             frame.mb_height());
    for (uint32_t i = 0; i < frame.mb_height(); i++) {
        std::vector<MotionVector> mv_row = frame.get_row(i);
        for (uint32_t j = 0; j < frame.mb_width(); j++) {
            MotionVector mv;
            mv.mvL0[1] = mv.mvL0[1];
            result.set_mv(j, i, mv);
        }
    }
    return result;
}

std::vector<uint32_t> angle_histogram(MvFrame &frame, uint32_t row_start,
                                      uint32_t col_start, uint32_t width,
                                      uint32_t height, uint32_t bins) {
    std::vector<uint32_t> result(bins, 0);
    double unit = M_PI * 2 / bins;
    for (uint32_t i = row_start; i < row_start + height; i++) {
        for (uint32_t j = col_start; j < col_start + width; j++) {
            MotionVector mv = frame.get_mv(j, i);
            int x = mv.mvL0[1];
            int y = mv.mvL0[0];
            if (x == 0 && y > 0) {
                result[bins / 4] += 1;
            } else if (x == 0 && y < 0) {
                result[bins * 3 / 4] += 1;
            } else if (x != 0) {
                /* TODO: optimize this*/
                double angle = atan2(abs(y), abs(x));
                double addition = 0;
                if (x < 0 && y > 0)
                    addition = M_PI_2;
                else if(x < 0 && y < 0)
                    addition = M_PI;
                else if (x > 0 && y < 0)
                    addition = M_PI_2 + M_PI;
                angle += addition;
                auto bin = (uint32_t)ceil(angle / unit);
                result[bin] += 1;
            }
        }
    }
    return result;
}


bool operator<(const MvPoint &p1, const MvPoint &p2) {
    return p1.x < p2.x or p1.y < p2.y;
}

void add_points(std::vector<bool> & visited, std::set<MvPoint> & result,
                uint32_t index, uint32_t width, uint32_t height) {
    if (visited[index]) return;
    uint32_t x = index % width;
    uint32_t y = index / width;
    MvPoint p {
            x, y
    };
    result.insert(p);
    visited[index] = true;

    /* search for its neighbors */
    if (index > width) {
        /* up */
        add_points(visited, result, index - width, width, height);
    }
    if (index % width) {
        /* left */
        add_points(visited, result, index - 1, width, height);
    }
    if ((index + 1) % width != 0) {
        /* right */
        add_points(visited, result, index + 1, width, height);
    }
    if ((index / width < height - 1)) {
        /* down */
        add_points(visited, result, index + width, width, height);
    }
}

std::vector<std::set<MvPoint>> mv_partition(MvFrame &frame, double threshold) {
    std::vector<bool> visited = std::vector<bool>(frame.mb_height() *
                                                          frame.mb_width(),
                                              true);
    std::vector<std::set<MvPoint>> result;
    for (uint32_t i = 0; i < visited.size(); i++) {
        MotionVector mv = frame.get_mv(i);
        int x = mv.mvL0[0];
        int y = mv.mvL0[1];
        if (x * x + y * y > threshold)
            visited[i] = false;
    }
    uint32_t index = 0;
    while (index < visited.size()) {
        bool found = false;
        uint32_t i = 0;
        for (i = index; i < visited.size(); i++) {
            if (!visited[i]) {
                found = true;
                break;
            }
        }

        if (!found)
            break; /* terminate search */
        std::set<MvPoint> s;
        add_points(visited, s, index, frame.mb_width(), frame.mb_height());
        if (s.size() > 1)
            result.emplace_back(s);
        index++;
    }
    return result;
}