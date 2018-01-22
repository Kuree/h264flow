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


#include <vector>
#include <set>
#include <memory>
#include <iostream>
#include "io.hh"
#include "nal.hh"

static const std::set<std::string> mp4_container_boxes{
        "moov", "trak", "edts", "mdia", "minf", "stbl", "mvex", "moof", "traf",
        "mfra", "skip", "strk", "meta", "dinf", "ipro", "sinf", "fiin", "paen",
        "meco", "mere"};

class Box {
public:
    explicit Box(BinaryReader & br) : Box(br, true) {}
    Box(uint32_t size, std::string type, BinaryReader &br, bool read_data);
    Box(BinaryReader & br, bool read_data);
    explicit Box() : data_(), size_(), type_(), children_() {}
    Box(const Box & box);
    explicit Box(std::shared_ptr<Box> box);

    uint32_t size() const { return size_; }
    std::string type() const { return type_; }
    std::string data() { return data_; }
    uint64_t data_offset() { return data_offset_; }

    virtual void print(const uint32_t indent = 0);
    std::shared_ptr<Box> find_first(std::string type);
    std::set<std::shared_ptr<Box>> find_all(std::string type);

    void add_child(std::shared_ptr<Box> box);
    void add_child(BinaryReader & br) {
        add_child(std::make_shared<Box>(br));
    }
    const std::vector<std::shared_ptr<Box>> children() { return children_; }

    virtual ~Box() = default;

protected:
    std::string data_;
    uint32_t size_;
    std::string type_;
    std::vector<std::shared_ptr<Box>> children_;
    uint64_t data_start_ = 0;

    BinaryReader get_br(std::istream & stream);

private:
    uint64_t data_offset_ = 0;
    void parse_box(BinaryReader &br, bool read_data);
};


class MdatBox : public Box {
public:
    MdatBox() = delete;
    explicit MdatBox(Box & box) : Box(box), nal_units_() {}
    explicit MdatBox(std::shared_ptr<Box> box): MdatBox(*box.get()) { }

    void parse(std::vector<uint64_t> offsets);

    std::vector<std::shared_ptr<NALUnit>> nal_units() { return nal_units_; }

private:
    std::vector<std::shared_ptr<NALUnit>> nal_units_;
};


class FullBox : public Box {
public:
    explicit FullBox(const Box & box);

    uint8_t version() { return version_; }
    uint32_t flags() { return flags_; }

protected:
    uint8_t version_;
    uint32_t flags_;

};

class TkhdBox : public FullBox
{
public:
    TkhdBox(Box & box);
    uint64_t creation_time() { return creation_time_; }
    uint64_t modification_time() { return modification_time_; }
    uint32_t track_id() { return track_id_; }
    uint64_t duration() { return duration_; }
    uint32_t width() { return width_; }
    uint32_t height() { return height_; }

private:
    uint64_t creation_time_ = 0;
    uint64_t modification_time_ = 0;
    uint32_t track_id_ = 0;
    uint64_t duration_ = 0;
    int16_t volume_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    int16_t layer_ = 0;
    int16_t alternate_group_ = 0;
    std::vector<int32_t> matrix_ = {
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

    std::vector<uint64_t> chunk_offsets() { return entries_; }

private:
    std::vector<uint64_t> entries_;
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
    uint16_t width() { return width_; }
    uint16_t height() { return height_; }
    std::string compressorname() { return compressorname_; }
    uint32_t horizresolution() { return horizresolution_; }
    uint32_t vertresolution() { return vertresolution_; }
    uint16_t frame_count() { return frame_count_; }
    uint16_t depth() { return depth_; }

private:
    uint16_t width_;
    uint16_t height_;
    std::string compressorname_;
    uint32_t horizresolution_ = 0x00480000; /* 72 dpi */
    uint32_t vertresolution_ = 0x00480000; /* 72 dpi */
    uint16_t frame_count_ = 1;
    uint16_t depth_ = 0x0018;
};

class AvcC : public Box {
public:
    AvcC(Box & box);
    AvcC();

    uint8_t configuration_version() { return configuration_version_; }
    uint8_t avc_profile() { return avc_profile_; }
    uint8_t avc_profile_compatibility() { return avc_profile_compatibility_; }
    uint8_t avc_level() { return avc_level_; }
    uint8_t length_size_minus_one() { return length_size_minus_one_; }
    std::vector<std::shared_ptr<SPS_NALUnit>> sps_units() { return sps_units_; }
    std::vector<std::shared_ptr<PPS_NALUnit>> pps_units() { return pps_units_; }
private:

    uint8_t configuration_version_ = 1;
    uint8_t avc_profile_;
    uint8_t avc_profile_compatibility_;
    uint8_t avc_level_;
    uint8_t length_size_minus_one_ = 3;

    std::vector<std::shared_ptr<SPS_NALUnit>> sps_units_;
    std::vector<std::shared_ptr<PPS_NALUnit>> pps_units_;
};

class Avc1 : public VisualSampleEntry {
public:
    explicit Avc1(const Box & box);

    uint32_t avcc_size() { return avcc_size_; }
    AvcC & avcC() { return avcC_; }
private:
    /* for avcC */
    uint32_t avcc_size_;
    AvcC avcC_;
};

class StscBox : public FullBox {
public:
    explicit StscBox(std::shared_ptr<Box> box);

    struct SampleToChunk {
        uint32_t first_chunk;
        uint32_t samples_per_chunk;
        uint32_t sample_description_index;
    };

    std::vector<StscBox::SampleToChunk> entries() { return entries_; }
private:
    std::vector<StscBox::SampleToChunk> entries_;
};

class StszBox : public FullBox {
public:
    explicit StszBox(std::shared_ptr<Box> box);
    std::vector<uint32_t> entries() { return entries_; }
private:
    std::vector<uint32_t> entries_;
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
    std::shared_ptr<Box> root_;
    std::ifstream stream_;
};
#endif //H264FLOW_MP4_HH
