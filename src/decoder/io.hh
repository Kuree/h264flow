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

#ifndef H264FLOW_IO_HH
#define H264FLOW_IO_HH

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>

class BinaryReader {
public:
    explicit BinaryReader(std::istream & stream);

    /* read values */
    uint8_t read_uint8() { return read_raw<uint8_t>(false); }
    uint16_t read_uint16(bool switch_endian) { return read_raw<uint16_t>(switch_endian); }
    uint32_t read_uint32(bool switch_endian) { return read_raw<uint32_t>(switch_endian); }
    uint64_t read_uint64(bool switch_endian) { return read_raw<uint64_t>(switch_endian); }

    int8_t read_unt8() { return read_raw<int8_t>(false); }
    int16_t read_int16(bool switch_endian) { return read_raw<int16_t>(switch_endian); }
    int32_t read_int32(bool switch_endian) { return read_raw<int32_t>(switch_endian); }
    int64_t read_int64(bool switch_endian) { return read_raw<int64_t>(switch_endian); }

    uint16_t read_uint16() { return read_raw<uint16_t>(_little_endian); }
    uint32_t read_uint32() { return read_raw<uint32_t>(_little_endian); }
    uint64_t read_uint64() { return read_raw<uint64_t>(_little_endian); }

    int16_t read_int16() { return read_raw<int16_t>(_little_endian); }
    int32_t read_int32() { return read_raw<int32_t>(_little_endian); }
    int64_t read_int64() { return read_raw<int64_t>(_little_endian); }

    std::string read_bytes(uint64_t num);

    /* for NAL */
    uint64_t read_ue();
    int64_t read_se();
    uint64_t read_te(uint64_t range);
    uint8_t read_bit();
    uint64_t read_bits(uint64_t bits);
    uint64_t next_bits(uint64_t bits);
    void reset_bit(bool skip = true) {
        _bit_pos = 0;
        seek(skip ? pos() + 1: pos());
    }
    bool read_bit_as_bool() { return static_cast<bool>(read_bit()); }
    uint8_t bit_pos() const { return _bit_pos; }
    void set_bit_pos(uint8_t bit_pos);
    int64_t bits_left();

    /* io functions */
    uint64_t pos() { return (uint64_t)_stream.tellg(); }
    void seek(uint64_t pos) { _stream.seekg(pos); _bit_pos = 0; }
    bool eof() { return _stream.eof() || (pos() == size() && !_bit_pos); }
    uint64_t size();
    void set_little_endian(bool endian) { _little_endian = endian; }
    bool little_endian() { return _little_endian; }
    void switch_stream(std::istream &stream)
    { _stream.rdbuf(stream.rdbuf()); seek(0); }

    std::string print_bit_pos(uint64_t offset = 0);
    ~BinaryReader() = default;

private:
    std::istream & _stream;
    bool _little_endian = true;
    uint8_t _bit_pos = 0;
    uint8_t _last_byte = 0;

    uint64_t _size = 0; // avoid seeking on disk

    template<typename T> T read_raw(bool switch_endian) {
        size_t size = sizeof(T);
        std::string data = read_bytes(size);
        auto * value = reinterpret_cast<const T *>(data.c_str());
        if (switch_endian) {
            T result;
            auto * dst = reinterpret_cast<char *>(&result);
            memset(dst, 0, sizeof(T));
            auto * src = reinterpret_cast<const char *>(value);
            for (uint32_t i = 0; i < size; i++) {
                dst[size - i - 1] = src[i];
            }
            return result;
        } else {
            return *value;
        }

    }
};

class BitReader {
public:
    explicit BitReader(std::string data) : data_(std::move(data)) {}
    inline uint64_t pos() { return pos_; }
    inline uint8_t bit_pos() { return bit_pos_; }
    inline uint64_t size() { return data_.size(); }
    inline void seek(uint64_t pos) { pos_ = pos; bit_pos_ = 0; }
    inline void set_bit_pos(uint8_t bit_pos) { bit_pos_ = bit_pos; }
    bool read_bit_as_bool() { return static_cast<bool>(read_bit()); }
    inline uint8_t read_uint8() { return (uint8_t)data_[pos_++]; }
    /* TODO: escape RBSP */
    inline uint8_t read_bit()
    {
        auto tmp = (uint8_t)data_[pos_];
        tmp = (uint8_t)((tmp >> (7 - bit_pos_)) & 1);
        ++bit_pos_;
        if (bit_pos_ == 8) { bit_pos_ = 0; ++pos_; }
        return tmp;
    }

    inline uint64_t read_ue() {
        uint8_t num_zero = 0;
        uint64_t result;
        while(!read_bit())
            num_zero++;
        result = 1 << num_zero | read_bits(num_zero);
        return result - 1;
    }

    inline uint64_t read_bits(uint64_t bits) {
        uint64_t result = 0;
        for (int i = bits - 1; i >= 0; i--) {
            uint8_t  bit = read_bit();
            result |= bit << i;
        }
        return result;
    }

    inline uint64_t next_bits(uint64_t bits) {
        uint64_t curr_pos = pos_;
        uint8_t bit_pos = bit_pos_;
        uint64_t result = read_bits(bits);
        pos_ = curr_pos;
        bit_pos_ = bit_pos;
        return result;
    }

    inline uint64_t read_te(uint64_t range) {
        if (range > 1) {
            return read_ue();
        } else {
            return static_cast<uint64_t>(!read_bit_as_bool());
        }
    }

    int64_t read_se();

    inline int64_t bits_left() { return (data_.size() - pos_) * 8 - bit_pos_; }

private:
    std::string data_;
    uint64_t pos_ = 0;
    uint8_t bit_pos_ = 0;
};

class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream & stream) : stream_(stream) {}
    void write_uint8(uint8_t value);
    void write_uint32(uint32_t value);
private:
    std::ostream & stream_;
};

void unescape_rbsp(BinaryReader &br, BinaryWriter &bw, uint64_t size = 0);
bool search_nal(BinaryReader &br, bool skip_tag, uint32_t &tag_size);

#endif //H264FLOW_IO_HH
