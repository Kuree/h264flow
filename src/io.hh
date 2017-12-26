//
// Created by keyi on 12/21/17.
//

#ifndef H264FLOW_IO_HH
#define H264FLOW_IO_HH

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <string.h>

class BinaryReader {
public:
    BinaryReader(std::istream & stream) : _stream(stream) {}

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

    std::string read_bytes(uint32_t num);

    /* for NAL */
    uint64_t read_ue();
    int64_t read_se();
    uint8_t read_bit();
    uint64_t read_bits(uint8_t bits);
    void reset_bit(bool skip = true) {
        _bit_pos = 0;
        seek(skip ? pos() + 1: pos());
    }
    bool read_bit_as_bool() { return static_cast<bool>(read_bit()); }
    uint8_t bit_pos() const { return _bit_pos; }

    /* io functions */
    uint32_t pos() { return _stream.tellg(); }
    void seek(uint32_t pos) { _stream.seekg(pos); }
    bool eof() { return _stream.eof(); }
    uint32_t size();
    void set_little_endian(bool endian) { _little_endian = endian; }
    bool little_endian() { return little_endian(); }

    ~BinaryReader() {}

private:
    std::istream & _stream;
    bool _little_endian = true;
    uint8_t _bit_pos = 0;

    template<typename T> T read_raw(bool switch_endian) {
        size_t size = sizeof(T);
        std::string data = read_bytes(size);
        const T * value = reinterpret_cast<const T *>(data.c_str());
        if (switch_endian) {
            T result;
            char * dst = reinterpret_cast<char *>(&result);
            memset(dst, 0, sizeof(T));
            const char * src = reinterpret_cast<const char *>(value);
            for (uint32_t i = 0; i < size; i++) {
                dst[size - i - 1] = src[i];
            }
            return result;
        } else {
            return *value;
        }

    }
};

#endif //H264FLOW_IO_HH
