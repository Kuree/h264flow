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

#ifndef H264FLOW_MP4_HH
#define H264FLOW_MP4_HH

/* part of the implementation is from my work with Francis on the
 * TV broadcast project.
 */

#include "io.hh"
#include "nal.hh"
#include <vector>
#include <set>
#include <memory>
#include <iostream>

static const std::set<std::string> mp4_container_boxes{
        "moov", "trak", "edts", "mdia", "minf", "stbl", "mvex", "moof", "traf",
        "mfra", "skip", "strk", "meta", "dinf", "ipro", "sinf", "fiin", "paen",
        "meco", "mere"};

class Box {
public:
    explicit Box(BinaryReader & br) : Box(br, true) {}
    Box(BinaryReader & br, bool read_data);
    explicit Box() : _data(), _size(), _type(), _children() {}
    Box(const Box & box);
    explicit Box(std::shared_ptr<Box> box);

    uint32_t size() const { return _size; }
    std::string type() const { return _type; }
    std::string data() { return _data; }
    uint64_t data_offset() { return _data_offset; }

    virtual void print(const uint32_t indent = 0);
    std::shared_ptr<Box> find_first(std::string type);
    std::set<std::shared_ptr<Box>> find_all(std::string type);

    void add_child(std::shared_ptr<Box> box);
    void add_child(BinaryReader & br) {
        add_child(std::make_shared<Box>(br));
    }
    const std::vector<std::shared_ptr<Box>> children() { return _children; }

    virtual ~Box() = default;

protected:
    std::string _data;
    uint32_t _size;
    std::string _type;
    std::vector<std::shared_ptr<Box>> _children;
    uint64_t _data_start = 0;

    BinaryReader get_br(std::istream & stream);

private:
    uint64_t _data_offset = 0;
};


class MdatBox : public Box {
public:
    MdatBox() = delete;
    explicit MdatBox(Box & box) : Box(box), _nal_units() {}
    explicit MdatBox(std::shared_ptr<Box> box): MdatBox(*box.get()) { }

    void parse(std::vector<uint64_t> offsets);

    std::vector<std::shared_ptr<NALUnit>> nal_units() { return _nal_units; }

private:
    std::vector<std::shared_ptr<NALUnit>> _nal_units;
};


class FullBox : public Box {
public:
    explicit FullBox(const Box & box);

    uint8_t version() { return _version; }
    uint32_t flags() { return _flags; }

protected:
    uint8_t _version;
    uint32_t _flags;

};

class TkhdBox : public FullBox
{
public:
    TkhdBox(Box & box);
    uint64_t creation_time() { return _creation_time; }
    uint64_t modification_time() { return _modification_time; }
    uint32_t track_id() { return _track_id; }
    uint64_t duration() { return _duration; }
    uint32_t width() { return _width; }
    uint32_t height() { return _height; }

private:
    uint64_t _creation_time = 0;
    uint64_t _modification_time = 0;
    uint32_t _track_id = 0;
    uint64_t _duration = 0;
    int16_t _volume = 0;
    uint32_t _width = 0;
    uint32_t _height = 0;
    int16_t _layer = 0;
    int16_t _alternate_group = 0;
    std::vector<int32_t> _matrix = {
            0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000
    };
};

class StsdBox : public FullBox {
public:
    explicit StsdBox(const Box & box);
    explicit StsdBox(std::shared_ptr<Box> box) : StsdBox(*box.get()) {}
};

class StcoBox : public FullBox
{
public:
    explicit StcoBox(const Box & box) : StcoBox(box, box.type() == "co64") {}
    explicit StcoBox(const Box & box, bool read_large);

    std::vector<uint64_t> chunk_offsets() { return _entries; }

private:
    std::vector<uint64_t> _entries;
};

class Co64Box : public StcoBox {
public:
    Co64Box(const Box &box) : StcoBox(box, true) {}
    Co64Box(const Box &, bool) = delete;
};

class SampleEntry : public Box {
public:
    SampleEntry(const Box & box);

    /* accessors */
    uint16_t data_reference_index() { return data_reference_index_; }

private:
    uint16_t data_reference_index_;
};

class VisualSampleEntry : public SampleEntry {
public:
    VisualSampleEntry(const Box & box);

    /* accessors */
    uint16_t width() { return _width; }
    uint16_t height() { return _height; }
    std::string compressorname() { return _compressorname; }
    uint32_t horizresolution() { return _horizresolution; }
    uint32_t vertresolution() { return _vertresolution; }
    uint16_t frame_count() { return _frame_count; }
    uint16_t depth() { return _depth; }

private:
    uint16_t _width;
    uint16_t _height;
    std::string _compressorname;
    uint32_t _horizresolution = 0x00480000; /* 72 dpi */
    uint32_t _vertresolution = 0x00480000; /* 72 dpi */
    uint16_t _frame_count = 1;
    uint16_t _depth = 0x0018;
};

class AvcC : public Box {
public:
    AvcC(Box & box);
    AvcC();

    uint8_t configuration_version() { return _configuration_version; }
    uint8_t avc_profile() { return _avc_profile; }
    uint8_t avc_profile_compatibility() { return _avc_profile_compatibility; }
    uint8_t avc_level() { return _avc_level; }
    uint8_t length_size_minus_one() { return _length_size_minus_one; }
    std::vector<std::shared_ptr<SPS_NALUnit>> sps_units() { return _sps_units; }
    std::vector<std::shared_ptr<PPS_NALUnit>> pps_units() { return _pps_units; }
private:

    uint8_t _configuration_version = 1;
    uint8_t _avc_profile;
    uint8_t _avc_profile_compatibility;
    uint8_t _avc_level;
    uint8_t _length_size_minus_one = 3;

    std::vector<std::shared_ptr<SPS_NALUnit>> _sps_units;
    std::vector<std::shared_ptr<PPS_NALUnit>> _pps_units;
};

class Avc1 : public VisualSampleEntry {
public:
    Avc1(const Box & box);

    uint32_t avcc_size() { return _avcc_size; }
    AvcC & avcC() { return _avcC; }
private:
    /* for avcC */
    uint32_t _avcc_size;
    AvcC _avcC;
};

class MP4File {
public:
    MP4File(std::string filename);
    ~MP4File();

    void print();
    std::shared_ptr<Box> find_first(const std::string & type);
    std::set<std::shared_ptr<Box>> find_all(const std::string & type);

    /* used internally */
    std::string extract_stream(uint64_t position, uint64_t size);

private:
    std::shared_ptr<Box> _root;
    std::ifstream _stream;
};
#endif //H264FLOW_MP4_HH
