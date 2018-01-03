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
    explicit BinaryReader(std::istream & stream) : _stream(stream) {}

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
    uint64_t read_te();
    uint8_t read_bit();
    uint64_t read_bits(uint64_t bits);
    uint64_t next_bits(uint64_t bits);
    void reset_bit(bool skip = true) {
        _bit_pos = 0;
        seek(skip ? pos() + 1: pos());
    }
    bool read_bit_as_bool() { return static_cast<bool>(read_bit()); }
    uint8_t bit_pos() const { return _bit_pos; }
    void set_bit_pos(uint8_t bit_pos) { _bit_pos = bit_pos; }

    /* io functions */
    uint64_t pos() { return (uint64_t)_stream.tellg(); }
    void seek(uint64_t pos) { _stream.seekg(pos); _bit_pos = 0; }
    bool eof() { return _stream.eof() || (pos() == size() && !_bit_pos); }
    uint32_t size();
    void set_little_endian(bool endian) { _little_endian = endian; }
    bool little_endian() { return _little_endian; }
    void switch_stream(std::istream &stream)
    { _stream.rdbuf(stream.rdbuf()); seek(0); }

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

class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream & stream) : _stream(stream) {}
    void write_uint8(uint8_t value);
private:
    std::ostream & _stream;
};

void unescape_rbsp(BinaryReader &br, BinaryWriter &bw, uint64_t size = 0);
bool search_nal(BinaryReader &br, bool skip_tag, uint32_t &tag_size);

#endif //H264FLOW_IO_HH
