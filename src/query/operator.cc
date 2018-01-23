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
#include <cmath>
#include <map>
#include <numeric>
#include <iterator>
#include <set>
#include "operator.hh"

void ThresholdOperator::execute() {
    BooleanOperator::execute();
    for (uint32_t y = 0; y < _frame.mb_height(); y++) {
        for (uint32_t x = 0; x < _frame.mb_width(); x++) {
            auto mv = _frame.get_mv(x, y);
            if (mv.energy > _threshold) {
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

MvFrame median_filter(const MvFrame &frame, uint32_t size) {
    MvFrame result = MvFrame(frame.width(), frame.height(), frame.mb_width(),
                             frame.mb_height());
    uint32_t median_element = size / 2;
    for (uint32_t i = median_element;
         i < frame.mb_height() - median_element; i++) {
        std::vector<MotionVector> mv_row = frame.get_row(i);
        for (uint32_t j = median_element;
             j < frame.mb_width() - median_element; j++) {
            /* TODO: optimize this */
            std::vector<float> mv0 = std::vector<float>(size * size - 1);
            std::vector<float> mv1 = std::vector<float>(size * size - 1);
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
            float x = mv.mvL0[1];
            float y = mv.mvL0[0];
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


bool operator<(const MotionVector &p1, const MotionVector &p2) {
    return p1.x < p2.x or p1.y < p2.y;
}

void add_points(std::vector<bool> & visited, std::set<MotionVector> & result,
                const MvFrame &frame, uint32_t index, uint32_t width,
                uint32_t height) {
    if (visited[index]) return;
    result.insert(frame.get_mv(index));
    visited[index] = true;

    /* search for its neighbors */
    if (index > width) {
        /* up */
        add_points(visited, result, frame, index - width, width, height);
    }
    if (index % width) {
        /* left */
        add_points(visited, result, frame, index - 1, width, height);
    }
    if ((index + 1) % width != 0) {
        /* right */
        add_points(visited, result, frame, index + 1, width, height);
    }
    if ((index / width < height - 1)) {
        /* down */
        add_points(visited, result, frame, index + width, width, height);
    }
}

std::vector<MotionRegion> mv_partition(const MvFrame &frame,
                                                 double threshold) {
    std::vector<bool> visited = std::vector<bool>(frame.mb_height() *
                                                          frame.mb_width(),
                                              true);
    std::vector<MotionRegion> result;
    for (uint32_t i = 0; i < visited.size(); i++) {
        MotionVector mv = frame.get_mv(i);
        if (mv.energy > threshold)
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
        std::set<MotionVector> s;
        add_points(visited, s, frame, index, frame.mb_width(),
                   frame.mb_height());
        if (s.size() > 1) {
            MotionRegion mr(s);
            result.emplace_back(mr);
        }
        index++;
    }
    return result;
}

std::map<MotionType, bool> CategorizeCameraMotion(MvFrame &frame, double threshold,
                                  double fraction) {
    return CategorizeCameraMotion(frame, mv_partition(frame, threshold), fraction);
}

std::map<MotionType, bool> CategorizeCameraMotion(
        MvFrame &frame, std::vector<MotionRegion> motion_regions,
        double fraction) {
    /*
     * Naive approach:
     * Assumptions:
     * 1. If a camera is moving, the majority (defined by fraction) of the
     * motion vectors should be connected to each other (defined by threshold).
     * 2. If it is a camera move, motion vectors will have lots of similar
     * values, whereas if the camera zooms, the std is very high
     */
    uint64_t count = 0;
    auto minimum = (uint32_t) (frame.mb_height() * frame.mb_width() *
                               fraction);
    MotionRegion mv_set;
    for (const auto &region : motion_regions) {
        if (region.mvs.size() > count && region.mvs.size() > minimum) {
            mv_set = region;
            count = region.mvs.size(); /* find the largest region */
        }
    }

    /* compute the mean and std */
    std::vector<float> x_list(mv_set.mvs.size());
    std::vector<float> y_list(mv_set.mvs.size());
    int i = 0;
    for (const auto &mv : mv_set.mvs) {
        x_list[i] = mv.mvL0[0];
        y_list[i] = mv.mvL0[1];
        i++;
    }

    double sum_x = std::accumulate(x_list.begin(), x_list.end(), 0.0);
    double mean_x = sum_x / x_list.size();
    double sq_sum_x = std::inner_product(x_list.begin(), x_list.end(),
                                         x_list.begin(), 0.0);
    double stdev_x = std::sqrt(sq_sum_x / x_list.size() - mean_x * mean_x);

    double sum_y = std::accumulate(x_list.begin(), x_list.end(), 0.0);
    double mean_y = sum_y / x_list.size();
    double sq_sum_y = std::inner_product(x_list.begin(), x_list.end(),
                                         x_list.begin(), 0.0);
    double stdev_y = std::sqrt(sq_sum_y / x_list.size() - mean_y * mean_y);


    bool zoom = std::abs(stdev_x) + std::abs(stdev_y) >
                (std::abs(mean_x) + std::abs(mean_y));
    bool move_right = mean_x < -1; /* reversed motion */
    bool move_left = mean_x > 1;
    bool move_up = mean_y < -1;
    bool move_down = mean_y > 1;
    bool no_motion = !(zoom || move_down || move_left || move_right || move_up);

    std::map<MotionType, bool> result;

    result[MotionType::NoMotion] = no_motion;
    result[MotionType::TranslationRight] = move_right;
    result[MotionType::TranslationUp] = move_up;
    result[MotionType::TranslationLeft] = move_left;
    result[MotionType::TranslationDown] = move_down;
    result[MotionType::Zoom] = zoom;

    return result;
}

std::set<MotionVector> background_filter(const MvFrame & frame) {
    /* http://ieeexplore.ieee.org/document/1334181
     * http://ieeexplore.ieee.org/document/6872825
     * http://ieeexplore.ieee.org/document/871569
    */

    /* my own algorithm based on these three papers */

    /* 1: median filter to remove noise */
    auto smooth_frame = median_filter(frame, 3);

    /* 2: do an xor to obtain background motion */
    /* detect actual object motions with high threshold */
    auto obj_motions = mv_partition(frame, 10);
    /* background motion with low threshold */
    auto bg_motions = mv_partition(frame, 0.5);

    /* flatten the motions */
    std::set<MotionVector> obj_flatten;
    std::set<MotionVector> bg_flatten;
    for (const auto &set : obj_motions) {
        obj_flatten.insert(set.mvs.begin(), set.mvs.end());
    }
    for (const auto &set : bg_motions) {
        bg_flatten.insert(set.mvs.begin(), set.mvs.end());
    }
    /* xor */
    std::set<MotionVector> background;
    std::set_difference(bg_flatten.begin(), bg_flatten.end(),
                        obj_flatten.begin(), obj_flatten.end(),
                        std::inserter(background, background.begin()));
    return background;
}


inline bool operator==(const MotionRegion &lhs, const MotionRegion &rhs)
{ return lhs.id == rhs.id; }

inline bool operator<(const MotionRegion &lhs, const MotionRegion &rhs)
{ return lhs.id < rhs.id; }

/* initialize static members */
std::mt19937_64 MotionRegion::gen_ = std::mt19937_64(std::random_device()());
std::uniform_int_distribution<unsigned long long> MotionRegion::dis_ =
        std::uniform_int_distribution<unsigned long long>();


std::pair<float, float> compute_centroid(const std::set<MotionVector> &set) {
    float sum_x = 0, sum_y = 0;
    for (const auto &mv : set) {
        sum_x += mv.x;
        sum_y += mv.y;
    }
    uint64_t size = set.size();
    return std::make_pair(sum_x / size, sum_y / size);
}

MotionRegion::MotionRegion(std::set<MotionVector> mvs)
        : mvs(std::move(mvs)), id(0) {
    while (!id) {
        id = MotionRegion::dis_(MotionRegion::gen_);
    }
    /* compute centroid */
    std::tie(x, y) = compute_centroid(this->mvs);
}

std::map<uint64_t, uint64_t> match_motion_region(
        std::vector<MotionRegion> regions1, std::vector<MotionRegion> regions2,
        float threshold) {
    /* TODO: optimize this */
    std::map<uint64_t, uint64_t> result;
    std::set<MotionRegion> mr2 = std::set<MotionRegion>(regions2.begin(),
                                                        regions2.end());
    for (const auto &r1 : regions1) {
        for (const auto &r2 : mr2) {
            if (r1 == r2)
                break;
            float distance = std::sqrt((r1.x - r2.x) * (r1.x - r2.x) +
                                               (r1.y - r2.y) * (r1.y - r2.y));
            if (distance < threshold) {
                result.insert({r1.id, r2.id});
                mr2.erase(r2);
                break;
            }
        }
    }

    return result;
}