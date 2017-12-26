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
    Box(BinaryReader & br);
    Box() : _data(), _size(), _type(), _children() {}
    Box(const Box & box);
    Box(std::shared_ptr<Box> box);

    uint32_t size() { return _size; }
    std::string type() { return _type; }
    std::string data() { return _data; }

    virtual void print(const uint32_t indent = 0);
    std::shared_ptr<Box> find_first(std::string type);
    std::set<std::shared_ptr<Box>> find_all(std::string type);

    void add_child(std::shared_ptr<Box> box);
    void add_child(BinaryReader & br) {
        add_child(std::make_shared<Box>(br));
    }
    const std::vector<std::shared_ptr<Box>> children() { return _children; }

    virtual ~Box() {}

protected:
    std::string _data;
    uint32_t _size;
    std::string _type;
    std::vector<std::shared_ptr<Box>> _children;
    uint32_t _data_start = 0;

    BinaryReader get_br(std::istream & stream);
};


class MdatBox : public Box {
public:
    MdatBox() = delete;
    MdatBox(Box & box);
    MdatBox(std::shared_ptr<Box> box);

    std::vector<std::shared_ptr<NALUnit>> nal_units() { return _nal_units; }

private:
    std::vector<std::shared_ptr<NALUnit>> _nal_units;
};


class FullBox : public Box {
public:
    FullBox(const Box & box);

    uint8_t version() { return _version; }
    uint32_t flags() { return _flags; }

protected:
    uint8_t _version;
    uint32_t _flags;

};

class StsdBox : public FullBox
{
public:
    StsdBox(const Box & box);
};

class SampleEntry : public Box
{
public:
    SampleEntry(const Box & box);

    /* accessors */
    uint16_t data_reference_index() { return data_reference_index_; }

private:
    uint16_t data_reference_index_;
};

class VisualSampleEntry : public SampleEntry
{
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

class Avc1 : public VisualSampleEntry
{
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

private:
    std::shared_ptr<Box> _root;
    std::ifstream _stream;
};
#endif //H264FLOW_MP4_HH
