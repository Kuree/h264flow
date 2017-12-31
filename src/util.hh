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

#ifndef H264FLOW_UTIL_HH
#define H264FLOW_UTIL_HH

#include <stdexcept>
#include "io.hh"
#include "consts.hh"

class NotImplemented : public std::logic_error
{
public:
    NotImplemented(std::string func_name) :
            std::logic_error(func_name + " not yet implemented") { };
};

/**
 Calculate the log base 2 of the argument, rounded up.
 Zero or negative arguments return zero
 Idea from http://www.southwindsgames.com/blog/2009/01/19/fast-integer-log2-function-in-cc/
 */
static uint64_t intlog2(uint64_t x)
{
    uint64_t log = 0;
    while ((x >> log) > 0)
    {
        log++;
    }
    if (x == 1u<<(log-1)) { log--; }
    return log;
}

static uint8_t read_coeff_token(int nC, BinaryReader & br) {
    /* adapted from https://goo.gl/pWSEqT */
    if (nC >= 8) {
        auto code = static_cast<uint32_t>(br.read_bits(6));
        int TotalCoeff   = (code >> 2);
        int TrailingOnes = (code & 3);
        if (TotalCoeff == 0 && TrailingOnes == 3)
            TrailingOnes = 0;
        else
            TotalCoeff++;
        return (uint8_t)((TotalCoeff << 2) | (TrailingOnes));
    }

    int tab = (nC == -2) ? 4 : (nC == -1) ? 3 : (nC < 2) ? 0 : (nC < 4) ? 1 : (nC < 8) ? 2 : 5;

    for (int TrailingOnes = 0; TrailingOnes < 4; TrailingOnes++) {
        for (int TotalCoeff = 0; TotalCoeff < 17; TotalCoeff++) {
            uint32_t length = coeff_token_length[tab][TrailingOnes][TotalCoeff];
            int code   = coeff_token_code  [tab][TrailingOnes][TotalCoeff];
            if (length > 0 && br.next_bits(length) == code) {
                br.read_bits(length);
                return (uint8_t)((TotalCoeff << 2) | (TrailingOnes));
            }
        }
    }
    throw std::runtime_error("coeff_token not found");
}

#endif //H264FLOW_UTIL_HH
