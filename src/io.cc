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

#include "io.hh"
#include <cmath>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using std::runtime_error;
using std::string;

string BinaryReader::read_bytes(uint64_t num) {
    if (pos() + num > size())
        throw std::runtime_error("stream eof");
    std::vector<char> result(num);  // Because vector is guaranteed to be contiguous in C++03
    _stream.read(&result[0], num);

    return std::string(&result[0], &result[num]);
}

uint64_t BinaryReader::size() {
    if (_size) {
        return _size;
    }
    uint32_t pos = this->pos();
    _stream.seekg(0, std::iostream::end);
    auto size = static_cast<uint32_t>(_stream.tellg());
    seek(pos);
    _size = size;
    return size;
}

/* https://goo.gl/hjaQRK */
uint64_t BinaryReader::read_ue() {
    uint8_t num_zero = 0;
    uint64_t result;

    while(!read_bit())
        num_zero++;

    result = 1 << num_zero | read_bits(num_zero);
    return result - 1;
}

uint64_t BinaryReader::read_te() {
    uint64_t x = read_ue();
    if (x > 1) {
        return x;
    } else {
        return static_cast<uint64_t>(!read_bit_as_bool());
    }
}

int64_t BinaryReader::read_se() {
    uint64_t value = read_ue();
    auto result = static_cast<int64_t>(std::ceil(value / 2.0));
    return value % 2 ? result : -result;
}

uint8_t BinaryReader::read_bit() {
     if (_stream.eof())
        throw std::runtime_error("stream eof");
    uint8_t tmp;
    if (_bit_pos && _bit_pos != 8) {
        tmp = _last_byte;
    } else {
        tmp = read_uint8();
        _last_byte = tmp;
        _bit_pos = 0;
    }
    tmp = static_cast<uint8_t>((tmp >> (7 - _bit_pos)) & 1);
    _bit_pos++;
    return tmp;

    /*uint64_t _pos = pos();
    uint8_t tmp = read_uint8();
    tmp = static_cast<uint8_t>((tmp >> (7 - _bit_pos)) & 1);
    if (_bit_pos % 8 == 7) {
        _bit_pos = 0;
    } else {
        _bit_pos++;
        _stream.seekg(_pos);
    }
    return tmp;
    */
}

uint64_t BinaryReader::read_bits(uint64_t bits) {
    if (bits > 64)
        throw std::runtime_error("Cannot read more than 64 bits");
    uint64_t result = 0;
    for (int i = bits - 1; i >= 0; i--) {
        uint8_t  bit = read_bit();
        result |= bit << i;
    }
    return result;
}

std::string BinaryReader::print_bit_pos(uint64_t offset) {
    std::ostringstream stream;
    stream << "bit pos: " << offset + pos() * 8 + _bit_pos;
    return stream.str();
}

uint64_t BinaryReader::next_bits(uint64_t bits) {
    uint64_t _pos = pos();
    uint8_t bit_pos = _bit_pos;
    uint8_t last_byte = _last_byte;
    uint64_t result = read_bits(bits);
    _stream.seekg(_pos); /* this one will reset _bit_pos to 0 */
    _bit_pos = bit_pos;
    _last_byte = last_byte;
    return result;
}

void BinaryWriter::write_uint8(uint8_t value) {
    auto buf = reinterpret_cast<char *>(&value);
    _stream.write(buf, sizeof(value));
}

void unescape_rbsp(BinaryReader &br, BinaryWriter &bw, uint64_t size) {
    uint8_t zero_count = 0;
    uint64_t stop_pos = size ? br.pos() + size : br.size();
    while (br.pos() < stop_pos) {
        uint8_t tmp = br.read_uint8();
        if (tmp) {
            bw.write_uint8(tmp);
            zero_count = 0;
        } else {
            if (zero_count < 1) {
                bw.write_uint8(tmp);
                zero_count++;
            } else if (zero_count == 1) {
                /* write the 0 first */
                bw.write_uint8(tmp);
                /* escape the 0x03 */
                tmp = br.read_uint8();
                zero_count = 0;
                if (tmp != 0x03)
                    bw.write_uint8(tmp);
                else if (!tmp)
                    throw std::runtime_error("no emulation prevention found");
            } else {
                throw std::runtime_error("incorrect state in unescape");
            }
        }
    }
}

bool search_nal(BinaryReader &br, bool skip_tag, uint32_t & tag_size) {
    if (br.pos() + 4 > br.size())
        return false;
    /* assume at the very biginning of the file it's 0x000001 */
    uint8_t tmp1 = br.read_uint8();
    uint8_t tmp2 = br.read_uint8();
    uint8_t tmp3 = br.read_uint8();
    uint8_t tmp4 = br.read_uint8();
    while (br.size() != br.pos()) {
        if (tmp2 == 0 && tmp3 == 0 && tmp4 == 1) {
            /* we found one */
            tag_size = tmp1 ? 3 : 4;
            if (!skip_tag) {
                br.seek(br.pos() - tag_size);
            }
            return true;
        } else {
            tmp2 = tmp3;
            tmp3 = tmp4;
            tmp4 = br.read_uint8();
        }
    }
    return false;
}