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

#ifndef H264FLOW_OPERATOR_HH
#define H264FLOW_OPERATOR_HH

#include <set>
#include <map>
#include <random>
#include "../decoder/h264.hh"


/* TODO:
 * We need to redesign how Operators works
 */

class Operator {
public:
    explicit Operator(Operator & op) : _frame(op._frame) {}
    explicit Operator(MvFrame frame) : _frame(std::move(frame)) {}
    virtual void execute() { _has_executed = true; };
    virtual ~Operator() = default;
    bool has_executed() const { return _has_executed; }
    MvFrame get_frame() const { return _frame; }

protected:
    MvFrame _frame;
    bool _has_executed = false;
};

class BooleanOperator : public Operator {
public:
    explicit BooleanOperator(Operator & op) : Operator(op)
    { if (!op.has_executed()) { op.execute(); } }
    explicit BooleanOperator(MvFrame frame) : Operator(std::move(frame)) {}

    virtual bool result () { return false; }
};

class ReduceOperator : public Operator {
public:
    explicit ReduceOperator(Operator & op) : Operator(op)
    { if (!op.has_executed()) { op.execute(); } }
    explicit ReduceOperator(MvFrame frame) : Operator(std::move(frame)) {}
    void execute() final
    { if (!_has_executed) { reduce(); _has_executed = true; } }
protected:
    virtual void reduce() {}
};


class ThresholdOperator : public BooleanOperator {
public:
    ThresholdOperator(uint32_t threshold, Operator &op)
            : BooleanOperator(op), _threshold(threshold) {}
    ThresholdOperator(uint32_t threshold, MvFrame frame)
            : BooleanOperator(frame), _threshold(threshold) {}
    bool result() override { return _result; }
    void execute() override;
private:
    uint32_t _threshold;
    bool _result = false;
};

class CropOperator : public ReduceOperator {
public:
    CropOperator(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
                 Operator &op) : ReduceOperator(op), _x(x), _y(y),
                                 _width(width), _height(height) {}
    CropOperator(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height,
                 MvFrame frame)
            : ReduceOperator(frame), _x(x / 16), _y(y / 16), _width(width),
              _height(height) {}

protected:
    void reduce() override;

private:
    uint32_t _x, _y, _width, _height;
};


MvFrame median_filter(const MvFrame &frame, uint32_t size);
MvFrame horizontal_filter(MvFrame &frame);
MvFrame vertical_filter(MvFrame &frame);
/* perform median filter in temporal domain */
MvFrame median_filter(std::vector<MvFrame> frames);

bool operator<(const MotionVector &p1, const MotionVector &p2);

std::vector<uint32_t> angle_histogram(MvFrame &frame, uint32_t row_start,
                                      uint32_t col_start, uint32_t width,
                                      uint32_t height, uint32_t bins);

class MotionRegion {
public:
    std::set<MotionVector> mvs;
    /* those are centroid */
    float x = 0;
    float y = 0;
    uint64_t id = 0;
    explicit MotionRegion(std::set<MotionVector> mvs);
    explicit MotionRegion() : MotionRegion(std::set<MotionVector>()) {}
    explicit MotionRegion(const MotionRegion &r);

private:
    static std::mt19937_64 gen_;
    static std::uniform_int_distribution<unsigned long long> dis_;
};

inline bool operator==(const MotionRegion &lhs, const MotionRegion &rhs);
inline bool operator<(const MotionRegion &lhs, const MotionRegion &rhs);

/// Partition the motion vector with given threshold and size requirement
/// \param frame Motion vector frame.
/// \param threshold threshold: magnitude^2 of the motion vector.
/// \param size_threshold minimum macroblocks in a region
/// \return MotionRegion
std::vector<MotionRegion> mv_partition(const MvFrame &frame,
                                       double threshold,
                                       uint32_t size_threshold = 4);

enum MotionType {
    NoMotion = 0,
    TranslationRight = 1,
    TranslationUp = 1 << 1,
    TranslationLeft = 1 << 2,
    TranslationDown = 1 << 3,
    Rotation = 1 << 4,
    Zoom = 1 << 5
};

std::map<MotionType, bool> CategorizeCameraMotion(MvFrame &frame,
                                                    double threshold = 1,
                                                    double fraction = 0.6);

std::map<MotionType, bool> CategorizeCameraMotion(
        MvFrame &frame, std::vector<MotionRegion> motion_regions,
        double fraction = 0.6);

std::set<MotionVector> background_filter(const MvFrame & frame);

std::map<uint64_t, uint64_t> match_motion_region(
        const std::vector<MotionRegion> &regions1,
        const std::vector<MotionRegion> &regions2,
        float threshold);

std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> get_bbox(
        const MotionRegion & region);

std::vector<uint64_t> frames_without_motion(h264 &decoder,
                                            double threshold,
                                            uint32_t size_threshold);

bool motion_in_frame(const MvFrame &frame, double threshold,
                     uint32_t size_threshold);

MvFrame crop_frame(const MvFrame& frame, uint32_t x, uint32_t y, uint32_t width,
                   uint32_t height);


void temporal_mv_partition(std::vector<MotionRegion> &current_frame,
                           const std::vector<MotionRegion> &previous_frame);

#endif //H264FLOW_OPERATOR_HH
