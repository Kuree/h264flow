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
