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
#include <deque>
#include <functional>
#include "operator.hh"
#include "../decoder/util.hh"
#include "../decoder/consts.hh"

typedef std::chrono::high_resolution_clock Clock;


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
    MvFrame frame(_width,  _height, _width / 16, _height / 16);
    for (uint32_t y = 0; y < _height / 16; y++) {
        for (uint32_t x = 0; x < _width / 16; x++) {
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
        std::vector<MotionVector> mv_row = frame[i];
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

MvFrame median_filter(std::vector<MvFrame> mv_frames) {
    uint32_t mb_height = mv_frames[0].mb_height();
    uint32_t mb_width = mv_frames[0].mb_width();
    /* check if the dimensions are matched */
    for (const auto & frame : mv_frames) {
        if (mb_height != frame.mb_height() || mb_width != frame.mb_width())
            throw std::runtime_error("dimension does not match");
    }

    /* just do a copy. we're going to override it anyways */
    MvFrame result = MvFrame(mv_frames[0]);
    uint64_t median_element = mv_frames.size() / 2;
    for (uint32_t i = 0; i < mb_height; i++) {
        for (uint32_t j = 0; j < mb_width; j++) {
            std::vector<float> values0(mv_frames.size());
            std::vector<float> values1(mv_frames.size());
            /* TODO: optimize this. this is not efficient for the
             * cache */
            for (uint32_t k = 0; k < values0.size(); k++) {
                values0[k] = mv_frames[k].get_mv(j, i).mvL0[0];
                values1[k] = mv_frames[k].get_mv(j, i).mvL0[1];
            }
            auto mv = result.get_mv(j, i);
            std::nth_element(values0.begin(), values0.begin() +
                    median_element, values0.end());
            std::nth_element(values1.begin(), values1.begin() +
                    median_element, values1.end());
            mv.mvL0[0] = values0[median_element];
            mv.mvL0[1] = values1[median_element];
            result.set_mv(j, i, mv);
        }
    }
    return result;
}

MvFrame horizontal_filter(MvFrame &frame) {
    MvFrame result = MvFrame(frame.width(), frame.height(), frame.mb_width(),
                             frame.mb_height());
    for (uint32_t i = 0; i < frame.mb_height(); i++) {
        std::vector<MotionVector> mv_row = frame[i];
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
        std::vector<MotionVector> mv_row = frame[i];
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
                                       double threshold,
                                       uint32_t size_threshold) {
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
        if (s.size() > size_threshold) {
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

MotionRegion::MotionRegion(const MotionRegion &r) : MotionRegion(r.mvs) {
    id = r.id;
}

std::pair<float, float> compute_centroid(const std::set<MotionVector> &set) {
    //if (set.empty()) return;
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
        const std::vector<MotionRegion> &regions1,
        const std::vector<MotionRegion> &regions2,
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

std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> get_bbox(
        const MotionRegion & region) {
    uint32_t x_min = 0xFFFFFFFF, y_min = 0xFFFFFFFF;
    uint32_t x_max = 0, y_max = 0;
    for (auto const & mv : region.mvs) {
        if (mv.x < x_min)
            x_min = mv.x;
        if (mv.y < y_min)
            y_min = mv.y;
        if (mv.x > x_max)
            x_max = mv.x;
        if (mv.y > y_max)
            y_max = mv.y;
    }
    return std::make_tuple(x_min, y_min, x_max + MACROBLOCK_SIZE,
                           y_max + MACROBLOCK_SIZE);
}

std::vector<uint64_t> frames_without_motion(h264 &decoder,
                                            double threshold,
                                            uint32_t size_threshold) {
    uint64_t num_of_frames = decoder.index_size();
    std::vector<uint64_t> result;
    for (uint64_t i = 0; i < num_of_frames; i++) {
        bool p_slice;
        MvFrame frame;
        std::tie(frame, p_slice) = decoder.load_frame(i);
        if (p_slice && !motion_in_frame(frame, threshold, size_threshold))
            result.emplace_back(i);
    }
    return result;
}

bool motion_in_frame(const MvFrame &frame, double threshold,
                     uint32_t size_threshold) {
    auto regions = mv_partition(frame, threshold, size_threshold);
    return !regions.empty();
}

MvFrame crop_frame(const MvFrame& frame, uint32_t x, uint32_t y, uint32_t width,
                   uint32_t height) {
    CropOperator crop(x, y, width, height, frame);
    crop.execute();
    return crop.get_frame();
}

inline std::pair<double, double> mean_velocity(const MotionRegion &r) {
    double x = 0;
    double y = 0;
    for (auto const & mv : r.mvs) {
        x += mv.mvL0[0];
        y += mv.mvL0[1];
    }
    return std::make_pair(x / r.mvs.size(), y / r.mvs.size());
}

template<typename T>
inline std::set<T> set_diff(const std::set<T> &set1,
                                 const std::set<T> &set2) {
    std::set<T> result;
    for (const auto & e : set1)
        if (set2.find(e) != set2.end())
            result.insert(e);
    return result;
}

/* r2 will move based on its mean velocity */
std::pair<MotionRegion, bool> merge_detection(const MotionRegion &r1,
                                                    const MotionRegion &r2,
                                                    double fraction = 0.9) {
    /* compute velocity */
    double x, y;
    std::tie(x, y) = mean_velocity(r2);
    auto dx = static_cast<int>(x / MACROBLOCK_SIZE);
    auto dy = static_cast<int>(y / MACROBLOCK_SIZE);
    MotionRegion r;
    if (dx || dy) {
        /* move r2 with the velocity */
        r = MotionRegion(r2);
        for (auto &mv : r.mvs) {
            mv.mvL0[0] += dx;
            mv.mvL0[1] += dy;
        }
    } else {
        r = r2;
    }
    auto common = set_diff<MotionVector>(r1.mvs, r2.mvs);
    double size = common.size();
    return std::make_pair(MotionRegion(common),
                          size >= r1.mvs.size() * fraction
                          || size >= r2.mvs.size() * fraction);

}

/* TODO: fix this. currently not working */
void temporal_mv_partition(std::vector<MotionRegion> &current_frame,
                           const std::vector<MotionRegion> &previous_frame) {
    /* main assumptions:
     *      1. an object won't disappear in the middle of the movement
     *      2. sequences are temporally invariant, that is, it doesn't matter
     *         if it's played forward or backward.
     * limitations:
     *      1. if two objects are close to each other within all the sequences,
     *         then this method will not work since in motion vector's domain,
     *         there is no difference between these two objects
     * */

    /* only implement forward partition */
    std::set<MotionRegion> region_to_delete;
    std::set<MotionRegion> region_to_add;
    for (const auto &r1 : current_frame) {
        for (const auto &r2 : previous_frame) {
            auto result = merge_detection(r1, r2);
            if (result.second) {
                region_to_add.insert(result.first);
                region_to_delete.insert(r1);
            }
        }
    }
    for (const auto &r : region_to_add)
        current_frame.emplace_back(r);
    for (const auto &r : region_to_delete) {
        current_frame.erase(std::remove(current_frame.begin(),
                                        current_frame.end(), r),
                            current_frame.end());
    }
}

bool is_scene_cut(std::vector<std::shared_ptr<MacroBlock>> mvs,
                  float threshold) {
    /* assumptions made here:
     * 1. because we use --ref 0 in x264, all P-slices only have one reference
     * frame, that is, the previous on. Therefore, a I-slice is a very good
     * indication of scene.
     * 2. in addition to I-slices, if the majority of the macroblocks are intra,
     * it is also a good indication of scene cut.
     * */
    uint32_t count = 0;
    for (const auto &mb : mvs) {
        if (is_mb_intra(mb->mb_type, 0))
            ++count;
    }
    return count >= (mvs.size() * threshold);
}

bool is_scene_cut(h264 &decoder, uint32_t frame_num, float threshold) {
    auto mvs = decoder.get_raw_mb(frame_num);
    /* we trust x264 is going to make a good guess for scene cut detection */
    if (!mvs.empty())
        return is_scene_cut(mvs, threshold);
    else
        return true;
}

std::vector<bool> index_scene_cut(h264 &decoder, float threshold) {
    std::vector<bool> first_pass_result(decoder.index_size());
    auto start = Clock::now();
    const uint64_t total_frames = first_pass_result.size();
    /* first pass */
    for (uint32_t frame_num = 0; frame_num < total_frames; frame_num++) {
        try {
            first_pass_result[frame_num] =
                    is_scene_cut(decoder, frame_num, threshold);
        } catch (std::runtime_error &ex) {
            std::cerr << "Unable to decode frame " << frame_num << std::endl
                      << "Error: " << ex.what() << std::endl;
        }
        if (frame_num % 100 == 99) {
            auto end = Clock::now();
            auto time_used = std::chrono::duration_cast<
                    std::chrono::milliseconds>(end - start).count();
            std::cerr << '\r' << frame_num << "/" <<total_frames
                      << " speed: " << (1e5f / time_used);
            start = end;
        }
    }
    std::cerr << '\r';
    /* second pass */
    const uint32_t window_size = 20;
    std::deque<int> frame_queue;
    std::vector<bool> second_pass_result(first_pass_result.size());
    uint64_t counter = 0;
    int sum = 0;
    for (uint32_t frame_num = 0; frame_num < total_frames; frame_num++) {
        if (frame_queue.size() < window_size) {
            int value = first_pass_result[frame_num];
            frame_queue.push_back(value);
            sum += value;
        } else {
            int new_value = first_pass_result[frame_num];
            if (!sum) { /* window clean */
                second_pass_result[counter++] = false;
                /* the value is 0 */
                frame_queue.pop_front();
                frame_queue.push_back(new_value);
                sum += new_value;
            } else {
                if (new_value) {
                    /* new values pop in the frame, therefore *zero* out
                     * all the values in the queue */
                    second_pass_result[counter++] = false;
                    int old_value = frame_queue.front();
                    frame_queue.pop_front();
                    frame_queue.push_back(new_value);
                    sum += new_value - old_value;
                } else {
                    int old_value = frame_queue.front();
                    /* be careful since we want the last 1 to be stored */
                    if (old_value == sum) {
                        second_pass_result[counter++] = true;
                        frame_queue.pop_front();
                        sum = 0;
                    } else {
                        second_pass_result[counter++] = false;
                        frame_queue.pop_front();
                        sum -= old_value;
                    }
                    frame_queue.push_back(new_value);
                }
            }
        }
    }

    while(!frame_queue.empty()) {
        second_pass_result[counter++] = frame_queue.front();
        frame_queue.pop_front();
    }
    if (counter != total_frames)
        throw std::runtime_error("incorrect state in scene cut calculation");

    /* first frame is not a cut */
    second_pass_result[0] = false;
    return second_pass_result;
}
