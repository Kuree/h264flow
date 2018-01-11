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

#include "mp4.hh"
#include <sstream>
#include <map>
#include <experimental/filesystem>

using std::string;
namespace fs = std::experimental::filesystem;

/* this holds for most cases. however, ideally it should be provided by
 * mp4 avc box.
 */
static const uint32_t LENGTH_SIZE_MINUS_ONE = 3;


Box::Box(BinaryReader &br, bool read_data) : _data(), _size(), _type(),
                                             _children() {
    _size = br.read_uint32(true);
    _type = br.read_bytes(4);

    parse_box(br, read_data);
}

Box::Box(uint32_t size, std::string type, BinaryReader &br, bool read_data)
        : _data(), _size(size), _type(type), _children() {
    parse_box(br, read_data);
}

Box::Box(const Box & box) : _data(box._data), _size(box._size),
                            _type(box._type), _children() {}

Box::Box(std::shared_ptr<Box> box) : Box(*box.get()) {}

void Box::parse_box(BinaryReader &br, bool read_data) {
    _data_offset = br.pos(); /* used to get data from mp4 stream */

    if (mp4_container_boxes.find(_type) != mp4_container_boxes.end()) {
        uint64_t end_pos = br.pos() + _size - 8;
        while (br.pos() < end_pos) {
            add_child(br);
        }
    } else if (_size > 8 && read_data && _type != "mdat") {
        _data = br.read_bytes(_size - 8);
    } else {
        /* skip data */
        uint64_t dst_pos = br.pos() + _size - 8;
        br.seek(dst_pos);
    }
}

void Box::add_child(std::shared_ptr<Box> box) {
    if (box->type() == "stsd") {
        _children.emplace_back(std::make_shared<StsdBox>(box));
    } else {
        _children.emplace_back(box);
    }
}

void Box::print(const uint32_t indent) {
    std::cout << std::string(indent, ' ') << "- " << _type << " "
              << _size << std::endl;
    if (!_children.empty())
        for (const auto & box : _children)
            box->print(indent + 2);
}

BinaryReader Box::get_br(std::istream & stream) {
    BinaryReader br(stream);
    br.seek(_data_start);
    return br;
}

std::shared_ptr<Box> Box::find_first(std::string type) {
    for (const auto & box : _children) {
        if (box->type() == type)
            return box;
        auto result = box->find_first(type);
        if (result) return result;
    }
    return nullptr;
}

std::set<std::shared_ptr<Box>> Box::find_all(std::string type) {
    std::set<std::shared_ptr<Box>> result;
    for (const auto & box : _children) {
        if (box->_type == type) {
            result.insert(box);
        }
        auto child_result = box->find_all(type);
        result.insert(child_result.begin(), child_result.end());
    }
    return result;
}

MP4File::MP4File(std::string filename): _root(std::make_shared<Box>()),
                                        _stream() {
    fs::path p(filename.c_str());
    if (!fs::exists(p))
        throw std::runtime_error(filename + " not found");
    _stream.open(filename, std::ifstream::binary);
    BinaryReader br(_stream);
    uint64_t end = br.size() - 1;
    while (br.pos() < end){
        auto box = std::make_shared<Box>(br);
        _root->add_child(box);
    }


}

MP4File::~MP4File() {
    _stream.close();
}

void MP4File::print() {
    for (auto const & box : _root->children()) {
        box->print();
    }
}

std::string MP4File::extract_stream(uint64_t position, uint64_t size) {
    BinaryReader br(_stream);
    br.seek(position);
    return br.read_bytes(size);
}

std::shared_ptr<Box> MP4File::find_first(const std::string & type) {
    return _root->find_first(type);
}

std::set<std::shared_ptr<Box>> MP4File::find_all(const std::string & type) {
    return _root->find_all(type);
}

void MdatBox::parse(std::vector<uint64_t> offsets) {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    for (uint64_t offset : offsets) {
        br.seek(offset);
        if (LENGTH_SIZE_MINUS_ONE == 3) {
            uint32_t size = br.read_uint32(true);
            auto nal = std::make_shared<NALUnit>(br, size);
            _nal_units.emplace_back(nal);
        } else {
            throw std::runtime_error("Not supported");
        }

    }
}

FullBox::FullBox(const Box & box) : Box(box), _version(), _flags() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);

    uint32_t tmp = br.read_uint32();
    _version = static_cast<uint8_t >((tmp >> 24) & 0xFF);
    _flags = tmp & 0x00FFFFFF;

    _data_start = br.pos();
}

StsdBox::StsdBox(const Box & box) : FullBox(box) {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);

    uint32_t num_sample_entries = br.read_uint32();

    for (uint32_t i = 0; i < num_sample_entries; ++i) {
        std::shared_ptr<Box> b = std::make_shared<Box>(br);
        add_child(b);
    }
}


TkhdBox::TkhdBox(Box &box) : FullBox(box) {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);

    if (version() == 1) {
        _creation_time = br.read_uint64();
        _modification_time = br.read_uint64();
        _track_id = br.read_uint32();
        br.read_bytes(4); /* reserved */
        _duration = br.read_uint64();
    } else {
        _creation_time = br.read_uint32();
        _modification_time = br.read_uint32();
        _track_id = br.read_uint32();
        br.read_bytes(4); /* reserved */
        _duration = br.read_uint32();
    }

    br.read_bytes(8); /* reserved */
    _layer = br.read_int16();
    _alternate_group = br.read_int16();
    _volume = br.read_int16();
    br.read_bytes(2); /* reserved */

    std::vector<int32_t> matrix;
    for (int i = 0; i < 9; ++i) {
        matrix.emplace_back(br.read_int32());
    }
    _matrix = move(matrix);

    /* width and height are 16.16 fixed-point numbers */
    _width = br.read_uint32() / 65536;
    _height = br.read_uint32() / 65536;
}

StcoBox::StcoBox(const Box &box, bool read_large) : FullBox(box), _entries() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    uint32_t entry_count = br.read_uint32();
    _entries = std::vector<uint64_t>(entry_count);
    for (uint32_t i = 0 ; i < entry_count; i++) {
        if (read_large)
            _entries[i] = br.read_uint64();
        else
            _entries[i] = br.read_uint32();
    }
}

StscBox::StscBox(std::shared_ptr<Box> box) : FullBox(*box.get()), _entries() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    uint32_t entry_count = br.read_uint32();
    _entries = std::vector<StscBox::SampleToChunk>(entry_count);
    for (uint32_t i = 0; i < entry_count; i++) {
        uint32_t first_chunk = br.read_uint32();
        uint32_t samples_per_chunk = br.read_uint32();
        uint32_t sample_desc_index = br.read_uint32();
        _entries[i] = SampleToChunk {
                first_chunk,
                samples_per_chunk,
                sample_desc_index
        };
    }
}

StszBox::StszBox(std::shared_ptr<Box> box) : FullBox(*box.get()), _entries() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    uint32_t sample_size = br.read_uint32();
    uint32_t sample_count = br.read_uint32();
    if (sample_size == 0) {
        _entries = std::vector<uint32_t>(sample_count);
        for (uint32_t i = 0; i < sample_count; i++) {
            _entries[i] = br.read_uint32();
        }
    }
}

SampleEntry::SampleEntry(const Box & box) : Box(box), data_reference_index_() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);

    br.read_bytes(6); /* reserved */
    data_reference_index_ = br.read_uint16();

    _data_start = br.pos();
}

VisualSampleEntry::VisualSampleEntry(const Box & box) : SampleEntry(box),
                                                        _width(), _height(),
                                                        _compressorname() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    br.read_bytes(2); /* pre-defined */
    br.read_bytes(2); /* reserved */
    br.read_bytes(12); /* pre-defined */

    _width = br.read_uint16();
    _height = br.read_uint16();
    _horizresolution = br.read_uint32();
    _vertresolution = br.read_uint32();

    br.read_bytes(4); /* reserved */

    _frame_count = br.read_uint16();

    /* read compressorname and ignore padding */
    uint8_t displayed_bytes = br.read_uint8();
    if (displayed_bytes > 0) { /* be sure to avoid reading 0! */
        _compressorname = br.read_bytes(displayed_bytes);
    } else if (displayed_bytes > 31)
        throw std::runtime_error("displayed_bytes is larger than 31");
    br.read_bytes(static_cast<uint8_t>(31 - displayed_bytes));

    _depth = br.read_uint16();

    br.read_bytes(2); /* pre-defined */
    _data_start = br.pos();
}

Avc1::Avc1(const Box & box) : VisualSampleEntry(box), _avcc_size(), _avcC() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    std::string box_type = "";
    for (int i = 0; i < 2; i++) {
        /* two optional boxes */
        _avcc_size = br.read_uint32();
        box_type = br.read_bytes(4);
        if (box_type == "avcC") {
            br.seek(br.pos() - 8);
            break;
        }
        Box box(_avcc_size, box_type, br, false);
    }
    if (box_type != "avcC")
        throw std::runtime_error("avcC not found");

    auto b = std::make_shared<Box>(br);
    _avcC = AvcC(*b.get());
}

AvcC::AvcC(Box &box) : Box(box), _avc_profile(), _avc_profile_compatibility(),
                       _avc_level(), _sps_units(), _pps_units() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    _configuration_version = br.read_uint8();
    _avc_profile = br.read_uint8();
    _avc_profile_compatibility = br.read_uint8();
    _avc_level = br.read_uint8();

    uint8_t tmp = br.read_uint8();
    if ((tmp >> 2) != 0x3F)
        throw std::runtime_error("reserved not equal to 111111'b");
    _length_size_minus_one = (uint8_t)(tmp & 0x03);
    tmp = br.read_uint8();
    uint8_t num_sps = (uint8_t)(tmp & 0x1F);
    for (uint8_t i = 0 ; i < num_sps; i++) {
        uint16_t length = br.read_uint16();
        auto unit = std::make_shared<SPS_NALUnit>(br.read_bytes(length));
        _sps_units.emplace_back(unit);
    }
    uint8_t num_pps = br.read_uint8();
    for (uint8_t i = 0; i < num_pps; i++) {
        uint16_t  length = br.read_uint16();
        auto unit = std::make_shared<PPS_NALUnit>(br.read_bytes(length));
        _pps_units.emplace_back(unit);
    }
}

AvcC::AvcC() : Box(), _avc_profile(), _avc_profile_compatibility(),
               _avc_level(), _sps_units(), _pps_units()  {}