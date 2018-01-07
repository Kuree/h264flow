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

#include "h264.hh"
#include "consts.hh"
#include "util.hh"
#include <sstream>
#include <experimental/filesystem>

using std::shared_ptr;
using std::runtime_error;
namespace fs = std::experimental::filesystem;

BitStream::BitStream(std::string filename) : _stream(), _chunk_offsets() {
    fs::path p(filename.c_str());
    if (!fs::exists(p))
        throw std::runtime_error(filename + " not found");
    _stream.open(filename);

    /* index chunks */
    BinaryReader br(_stream);
    uint32_t tag_size = 3;
    if (!search_nal(br, true, tag_size))
        throw std::runtime_error("no nal unit found");
    uint64_t pos = br.pos();
    while (search_nal(br, true, tag_size)) {
        uint64_t size = br.pos() - pos - tag_size;
        _chunk_offsets.emplace_back(std::make_pair<uint64_t, uint64_t>(
                (uint64_t)pos, (uint64_t)size));
        pos = br.pos();
    }
    /* last one */
    uint64_t size = br.pos() - pos;
    _chunk_offsets.emplace_back(std::make_pair<uint64_t, uint64_t>(
            (uint64_t)pos, (uint64_t)size));
}

std::string BitStream::extract_stream(uint64_t position, uint64_t size) {
    BinaryReader br(_stream);
    br.seek(position);
    return br.read_bytes(size);
}

h264::h264(std::shared_ptr<MP4File> mp4) : _chunk_offsets(), _mp4(mp4) {
    if (!mp4) throw std::runtime_error("mp4 is nullptr");
    auto tracks = mp4->find_all("trak");
    std::shared_ptr<Box> box = nullptr;
    for (const auto & track : tracks) {
        auto b = track->find_first("avc1");
        if (b) {
            box = b;
            _trak_box = track;
            break;
        }
    }
    shared_ptr<PPS_NALUnit> pps;
    shared_ptr<SPS_NALUnit> sps;
    if (box) {
        Avc1 avc1 = Avc1(*box.get());
        auto pps_list = avc1.avcC().pps_units();
        auto sps_list = avc1.avcC().sps_units();
        pps = pps_list[0];
        sps = sps_list[0]; /* use the first one */

        _length_size = (uint8_t)(avc1.avcC().length_size_minus_one() + 1);
    }
    if (!pps || !sps)
        throw std::runtime_error("sps or pps not found in mp4 file");
    _sps = sps;
    _pps = pps;
}

void h264::index_nal() {
    auto box = _trak_box->find_first("stco");
    if (!box) box = _trak_box->find_first("co64");
    if (!box) throw std::runtime_error("stco/co64 not found");
    StcoBox stco = StcoBox(*box.get());
    /* ffmpeg will produce broken stco sometimes */
    if (stco.chunk_offsets().size() < 3) {
        /* this will only happen when all the mdat data are used for NAT */
        /* TODO: fix this with length size */
        box = _mp4->find_first("mdat");
        std::string str = _mp4->extract_stream(box->data_offset(), box->size());
        std::istringstream stream(str);
        BinaryReader br(stream);
        uint64_t offset = box->data_offset();
        _chunk_offsets.emplace_back(offset); /* the first one */
        std::cout << std::hex << offset << std::endl;
        while (!br.eof()) {
            uint64_t size = read_nal_size(br);
            if (!size) return;
            offset += size;
            _chunk_offsets.emplace_back(offset);
            std::cout << "offset: " << std::hex << offset << " size: " << size << std::endl;
            br.seek(br.pos() + size);
        }
    } else {
        for (uint64_t offset : stco.chunk_offsets()) {
            /* TODO: build a table from frame_num to a list of offsets */
            _chunk_offsets.emplace_back(offset);
        }
    }
}

h264::h264(std::shared_ptr<BitStream> stream)
        : _chunk_offsets(), _bit_stream(stream) {
    for (const auto & pair : stream->chunk_offsets()) {
        /* linear search. however, since most sps and pps are at the very
         * beginning, this is actually pretty fast
         */
        std::string data = stream->extract_stream(pair.first, pair.second);
        NALUnit unit(std::move(data));
        if (unit.nal_unit_type() == 7) {
            _sps = std::make_shared<SPS_NALUnit>(unit);
        } else if (unit.nal_unit_type() == 8) {
            _pps = std::make_shared<PPS_NALUnit>(unit);
        }
        if (_sps && _pps)
            return;
    }
}

uint64_t h264::read_nal_size(BinaryReader &br) {
    uint32_t unit_size = 0;
    if (_length_size == 4) {
        unit_size = br.read_uint32();
    } else if (_length_size == 3) {
        uint32_t hi_byte = br.read_uint8();
        uint32_t low_bits = br.read_uint16();
        unit_size = hi_byte << 16 | low_bits;
    } else if (_length_size == 2) {
        unit_size = br.read_uint16();
    } else if (_length_size == 1) {
        unit_size = br.read_uint8();
    } else {
        throw std::runtime_error("unsupported length size");
    }
    return unit_size;
}

void h264::load_frame(uint64_t frame_num) {
    /* TODO: this is not actually frame_num, need to strip down slice_header
     * TODO: to obtain the actual frame_num (see TODO in index_nal()
     */
    std::string nal_data;
    if (_bit_stream) {
        uint64_t pos, size;
        std::tie(pos, size) = _bit_stream->chunk_offsets()[frame_num];
        nal_data = _bit_stream->extract_stream(pos, size);
    } else {
        if (_chunk_offsets.empty())
            index_nal();
        uint64_t offset = _chunk_offsets[frame_num];
        std::string size_string = _mp4->extract_stream(offset, _length_size);
        std::istringstream stream(size_string);
        BinaryReader br(stream);
        uint64_t unit_size = read_nal_size(br);
        nal_data = _mp4->extract_stream(offset + _length_size,
                                                    unit_size);
    }
    ParserContext ctx(_sps, _pps);
    Slice_NALUnit slice(std::move(nal_data));
    slice.parse(ctx);
}

void h264::process_inter_mb(ParserContext &ctx) {
    /* Section 8.4 */
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    uint64_t mb_type = mb->mb_type;
    uint64_t numMbPart = NumMbPart(mb_type);
    /* baseline profile won't have B slice */
    if (mb->slice_type == SliceType::TYPE_B)
        throw NotImplemented("B Slice");
    for (uint32_t mbPartIdx = 0; mbPartIdx < numMbPart; mbPartIdx++) {
        uint64_t partWidth = 0;
        uint64_t partHeight = 0;
        uint64_t numSubMbParts = 0;
        if (mb_type != P_8x8 && mb_type != P_8x8ref0) {
            numSubMbParts = 1;
            partWidth = MbPartWidth(mb_type);
            partHeight = MbPartHeight(mb_type);
        } else if (mb_type == P_8x8 || mb_type == P_8x8ref0) {
            throw NotImplemented("numSubMbParts");
        } else {
            partWidth = 4;
            partHeight = 4;
            numSubMbParts = 4;
        }
        uint64_t partWidthC = partWidth / ctx.SubWidthC();
        uint64_t partHeightC = partHeight / ctx.SubHeightC();

        uint64_t MvCnt = 0;
    }
}