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
    /* TODO: use read byte to avoid repeated disk read */
    uint32_t pos = this->pos();
    uint8_t tmp = read_uint8();
    tmp = static_cast<uint8_t>((tmp >> (7 - _bit_pos)) & 1);
    if (_bit_pos % 8 == 7) {
        _bit_pos = 0;
    } else {
        _bit_pos++;
        seek(pos);
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