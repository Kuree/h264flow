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

using std::string;

/* this holds for most cases. however, ideally it should be provided by
 * mp4 avc box.
 */
static const uint32_t LENGTH_SIZE_MINUS_ONE = 3;


Box::Box(BinaryReader &br) : _data(), _size(), _type(), _children() {
    _size = br.read_uint32(true);
    _type = br.read_bytes(4);

    if (mp4_container_boxes.find(_type) != mp4_container_boxes.end()) {
        uint32_t end_pos = br.pos() + _size - 8;
        while (br.pos() < end_pos) {
            add_child(br);
        }
    } else if (_size > 8) {
        _data = br.read_bytes(_size - 8);
    }
}

Box::Box(const Box & box) : _data(box._data), _size(box._size),
                            _type(box._type), _children() {}

Box::Box(std::shared_ptr<Box> box) : Box(*box.get()) {}

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
    if (_children.size())
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
        if (box->type() == type) {
            result.insert(box);
            auto child_result = box->find_all(type);
            result.insert(child_result.begin(), child_result.end());
        }
    }
    return result;
}

MP4File::MP4File(std::string filename): _root(std::make_shared<Box>()), _stream() {
    _stream.open(filename);
    BinaryReader br(_stream);
    uint32_t end_pos = br.size() - 7;
    while (br.pos() <= end_pos){
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

std::shared_ptr<Box> MP4File::find_first(const std::string & type) {
    return _root->find_first(type);
}

std::set<std::shared_ptr<Box>> MP4File::find_all(const std::string & type) {
    return _root->find_all(type);
}

MdatBox::MdatBox(Box & box) : Box(box), _nal_units() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    while (!br.eof()) {
        if (LENGTH_SIZE_MINUS_ONE == 3) {
            uint32_t size = br.read_uint32(true);
            if (!size) break;
            auto nal = std::make_shared<NALUnit>(br, size);
            _nal_units.emplace_back(nal);
        } else {
            throw std::runtime_error("Not supported");
        }

    }
}

MdatBox::MdatBox(std::shared_ptr<Box> box) : MdatBox(*box.get()) {
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

StcoBox::StcoBox(const Box &box, bool read_large) : FullBox(box), _entries() {
    std::istringstream stream(_data);
    BinaryReader br = get_br(stream);
    uint32_t entry_count = br.read_uint32();
    _entries = std::vector<uint64_t>(entry_count);
    for (uint32_t i = 0 ; i < entry_count; i++) {
        if (read_large)
            _entries.emplace_back(br.read_uint64());
        else
            _entries.emplace_back(br.read_uint32());
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
    uint32_t pos = br.pos();
    _avcc_size = br.read_uint32();
    string type = br.read_bytes(4);

    if (type != "avcC") {
        throw std::runtime_error("AVCC does not follow AVC1 immediately");
    }

    br.seek(pos);
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
    uint8_t num_sps = tmp & 0x1F;
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