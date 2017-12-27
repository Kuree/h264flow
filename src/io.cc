//
// Created by keyi on 12/21/17.
//

#include "io.hh"
#include <cmath>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using std::runtime_error;
using std::string;

string BinaryReader::read_bytes(uint64_t num) {
    std::vector<char> result(num);  // Because vector is guranteed to be contiguous in C++03
    _stream.read(&result[0], num);

    return std::string(&result[0], &result[num]);
}

uint32_t BinaryReader::size() {
    uint32_t pos = this->pos();
    _stream.seekg(0, std::iostream::end);
    auto size = static_cast<uint32_t>(_stream.tellg());
    seek(pos);
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

int64_t BinaryReader::read_se() {
    uint64_t value = read_ue();
    auto result = static_cast<int64_t>(std::ceil(value / 2.0));
    return value % 2 ? result : -result;
}

uint8_t BinaryReader::read_bit() {
    uint8_t tmp;
    if (bit_pos()) {
        tmp = _last_byte;
    } else {
        tmp = read_uint8();
        _last_byte = tmp;
    }
    tmp = static_cast<uint8_t>((tmp >> (7 - _bit_pos)) & 1);
    if (_bit_pos % 8 == 7) {
        _bit_pos = 0;
    } else {
        _bit_pos++;
    }
    return tmp;
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

void BinaryWriter::write_uint8(uint8_t value) {
    auto buf = reinterpret_cast<char *>(&value);
    _stream.write(buf, sizeof(value));
}

void unescape_rbsp(BinaryReader &br, BinaryWriter &bw, uint64_t size) {
    uint8_t zero_count = 0;
    uint64_t stop_pos = size ? br.pos() + size : br.size();
    while (br.pos() != stop_pos) {
        uint8_t tmp = br.read_uint8();
        if (tmp) {
            bw.write_uint8(tmp);
            zero_count = 0;
        } else {
            if (zero_count < 2) {
                bw.write_uint8(tmp);
                zero_count++;
            } else if (zero_count == 2) {
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