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
#include <algorithm>
#include <vector>
#include "io.hh"

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
uint64_t intlog2(uint64_t x);

uint8_t read_coeff_token(int nC, BitReader & br);

uint8_t read_ce_levelprefix(BitReader &br);

int code_from_bitstream_2d(BitReader &br,
                                  const uint8_t *lentab, const uint8_t *codtab,
                                  const int tabwidth, const int tabheight);

int read_ce_totalzeros(BitReader &br, const int vlcnum,
                              const int chromadc);

int read_ce_runbefore(BitReader &br, const int vlcnum);

void read_rbsp_trailing_bits(BitReader &br);

inline int InverseRasterScan_x(const int a, const int b, const int,
                               const int d) {
    return (a % (d / b)) * b;
}

inline int InverseRasterScan_y(const int a, const int b, const int c,
                               const int d) {
    return (a / (d / b)) * c;
}

bool is_mb_intra(uint64_t mb_type, uint64_t slice_type);

template<typename T> T median(T * array, uint32_t num_elem) {
    std::vector<T> v(array, array + num_elem);
    uint32_t n = num_elem / 2;
    std::nth_element(v.begin(), v.begin() + n, v.end());
    return v[n];
}

uint64_t NumSubMbPart(uint64_t sub_mb_type, uint64_t slice_type);

uint64_t SubMbPredMode(uint64_t sub_mb_type, uint64_t slice_type);

int MbPartPredMode(uint64_t mb_type, uint64_t x, uint64_t slice_type);
uint64_t NumMbPart(uint64_t mb_type);

uint64_t MbPartWidth(uint64_t mb_type);

uint64_t MbPartHeight(uint64_t mb_type);

int MbPredLuma(uint64_t mb_type);

int MbPredChroma(uint64_t mb_type);

bool is_p_slice(uint8_t first_byte);

bool file_exists(const std::string &filename);

std::string file_extension(const std::string &filename);

#endif //H264FLOW_UTIL_HH
