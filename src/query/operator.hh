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
            : ReduceOperator(frame), _x(x), _y(y), _width(width),
              _height(height) {}

protected:
    void reduce() override;

private:
    uint32_t _x, _y, _width, _height;
};


MvFrame median_filter(MvFrame &frame, uint32_t size);
MvFrame horizontal_filter(MvFrame &frame);
MvFrame vertical_filter(MvFrame &frame);

bool operator<(const MotionVector &p1, const MotionVector &p2);

std::vector<uint32_t> angle_histogram(MvFrame &frame, uint32_t row_start,
                                      uint32_t col_start, uint32_t width,
                                      uint32_t height, uint32_t bins);

std::vector<std::set<MotionVector>> mv_partition(MvFrame &frame,
                                                 double threshold);


enum MotionType {
    NoMotion = 0,
    Translation = 1,
    Rotation = 1 << 1,
    ZoomIn = 1 << 2,
    ZoomOut = 1 << 3
};

MotionType CategorizeMotion();
#endif //H264FLOW_OPERATOR_HH
