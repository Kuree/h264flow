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

#include <sstream>
#include "h264.hh"
#include "util.hh"
#include "consts.hh"

using std::shared_ptr;
using std::runtime_error;

BitStream::BitStream(std::string filename) : _stream(), _chunk_offsets() {
    if (!file_exists(filename))
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

h264::h264(const std::string &filename) : _chunk_offsets() {
    auto ext = file_extension(filename);
    if (ext == ".mp4") {
        _mp4 = std::make_shared<MP4File>(filename);
        load_mp4();
    } else if(ext == ".264" || ext == ".h264") {
        _bit_stream = std::make_shared<BitStream>(filename);
        load_bitstream();
    } else {
        throw std::runtime_error("unsupported media file extension");
    }
}

h264::h264(std::shared_ptr<MP4File> mp4) : _chunk_offsets(), _mp4(std::move(mp4)) {
    load_mp4();
}

void h264::load_mp4() {
    if (!_mp4) throw std::runtime_error("mp4 is nullptr");
    auto tracks = _mp4->find_all("trak");
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
    if (_bit_stream) return;
    auto box = _trak_box->find_first("stco");
    if (!box) box = _trak_box->find_first("co64");
    if (!box) throw std::runtime_error("stco/co64 not found");
    StcoBox stco = StcoBox(*box.get());
    box = _trak_box->find_first("stsc");
    if (!box) throw std::runtime_error("stsc not found");
    StscBox stsc = StscBox(box);
    box = _trak_box->find_first("stsz");
    if (!box) throw std::runtime_error("stsz not found");
    StszBox stsz = StszBox(box);

    auto sample_entries = stsz.entries();
    _chunk_offsets = std::vector<uint64_t>(sample_entries.size());
    uint32_t chunk_count = 0;
    uint32_t stsc_index = 0;
    uint32_t samples_per_chunk = stsc.entries()[stsc_index].samples_per_chunk;
    uint64_t in_chunk_offset = 0;
    for (uint32_t i = 0; i < sample_entries.size(); i++) {
        uint32_t sample_size = sample_entries[i];
        uint64_t result = stco.chunk_offsets()[chunk_count] + in_chunk_offset;
        _chunk_offsets[i] = result;
        in_chunk_offset += sample_size;

        if (!samples_per_chunk) {
            /* switch chunk */
            chunk_count++;
            in_chunk_offset = 0;
            if (stsc_index + 1 < stsc.entries().size()
                && stsc.entries()[stsc_index + 1].first_chunk - 1 == chunk_count) {
                stsc_index++;
                samples_per_chunk = stsc.entries()[stsc_index].samples_per_chunk;
            }
        }
    }

}

h264::h264(std::shared_ptr<BitStream> stream)
        : _chunk_offsets(), _bit_stream(stream) {
    load_bitstream();
}

void h264::load_bitstream() {
    for (const auto & pair : _bit_stream->chunk_offsets()) {
        /* linear search. however, since most sps and pps are at the very
         * beginning, this is actually pretty fast
         */
        std::string data = _bit_stream->extract_stream(pair.first, pair.second);
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

uint64_t h264::index_size() {
    if (_bit_stream) {
        return _bit_stream->chunk_offsets().size();
    } else {
        if (!_chunk_offsets.size())
            index_nal();
        return _chunk_offsets.size();
    }
}

MvFrame h264::load_frame(uint64_t frame_num) {
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
    /* test the slice type */
    if (!is_p_slice(nal_data[0]))
        return MvFrame(ctx.Width(), ctx.Height(), ctx.Width() / 16,  ctx.Height() / 16, false);
    Slice_NALUnit slice(std::move(nal_data));
    slice.parse(ctx);

    process_inter_mb(ctx);
    return MvFrame(ctx);
}

/* adapted from py264 */
void h264::process_inter_mb(ParserContext &ctx) {
    /* only to work with 4:2:0 */
    if (ctx.sps->chroma_array_type() != 1)
        throw NotImplemented("chroma_array_type != 1");
    /* Section 8.4 */
    for (const auto & mb : ctx.mb_array) {
        ctx.mb = mb;
        uint64_t mb_type = mb->mb_type;
        uint64_t slice_type = ctx.header()->slice_type;
        uint64_t numMbPart = NumMbPart(mb_type);
        /* baseline profile won't have B slice */
        if (mb->slice_type == SliceType::TYPE_B)
            throw NotImplemented("B Slice");
        for (uint32_t mbPartIdx = 0; mbPartIdx < numMbPart; mbPartIdx++) {
            uint64_t numSubMbParts = 0;
            if (mb_type != P_8x8 && mb_type != P_8x8ref0) {
                numSubMbParts = 1;
            } else if (mb_type == P_8x8 || mb_type == P_8x8ref0) {
                throw NotImplemented("numSubMbParts");
            } else {
                throw NotImplemented("numSubMbParts = 4");
            }
            //uint64_t partWidthC = partWidth / ctx.SubWidthC();
            //uint64_t partHeightC = partHeight / ctx.SubHeightC();

            //uint64_t MvCnt = 0;

            int refIdxL0 = -1;
            int refIdxL1 = -1;

            for (uint32_t subMbPartIdx = 0; subMbPartIdx < numSubMbParts;
                 subMbPartIdx++) {
                bool predFlagL0 = false;
                bool predFlagL1 = false;
                int mvL0[2] = {0, 0};
                int mvL1[2] = {0, 0};
                /* section 8.4.1.1 */
                if (mb_type == P_Skip) {
                    int refIdxLA = -1;
                    int refIdxLB = -1;
                    int refIdxLC = -1;
                    int mvL0A[2];
                    int mvL0B[2];
                    int mvL0C[2];
                    get_mv_neighbor_part(ctx, 0, mvL0A, mvL0B, mvL0C, refIdxLA,
                                         refIdxLB, refIdxLC);
                    if (mb->mbAddrA == -1 || mb->mbAddrB == -1
                        || (refIdxLA == 0 && mvL0A[0] == 0 && mvL0A[1] == 0)
                        || (refIdxLB == 0 && mvL0B[0] == 0 && mvL0B[1] == 0)) {
                        mvL0[0] = 0;
                        mvL0[1] = 0;
                    } else {
                        process_luma_mv(ctx, mbPartIdx, mvL0A, mvL0B, mvL0C,
                                        refIdxLA, refIdxLB, refIdxLC, mvL0);
                    }
                    refIdxL0 = 0;
                    predFlagL0 = true;
                    predFlagL1 = false;
                } else {
                    /* no B slice */
                    /* num mb part == 1 */
                    auto mb_pred_mode = MbPartPredMode(mb_type, mbPartIdx,
                                                       slice_type);
                    if (mb_pred_mode == Pred_L0 || mb_pred_mode == BiPred) {
                        refIdxL0 = (int) mb->mb_pred->ref_idx_l0[mbPartIdx];
                        predFlagL0 = true;
                    } else {
                        refIdxL0 = -1;
                        predFlagL0 = false;
                    }

                    if (mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred) {
                        refIdxL1 = (int) mb->mb_pred->ref_idx_l1[mbPartIdx];
                        predFlagL1 = true;
                    } else {
                        refIdxL1 = -1;
                        predFlagL1 = false;
                    }

                    if (predFlagL0) {
                        int mvpL0[2] = {0, 0};
                        process_luma_mv(ctx, mbPartIdx, 0, mvpL0);
                        mvL0[0] = (int) (mvpL0[0] +
                                         mb->mb_pred->mvd_l0[mbPartIdx][subMbPartIdx][0]);
                        mvL0[1] = (int) (mvpL0[1] +
                                         mb->mb_pred->mvd_l0[mbPartIdx][subMbPartIdx][1]);
                    }

                    if (predFlagL1) {
                        int mvpL1[2] = {0, 0};
                        process_luma_mv(ctx, mbPartIdx, 1, mvpL1);
                        mvL1[0] = (int) (mvpL1[0] +
                                         mb->mb_pred->mvd_l1[mbPartIdx][subMbPartIdx][0]);
                        mvL1[1] = (int) (mvpL1[1] +
                                         mb->mb_pred->mvd_l1[mbPartIdx][subMbPartIdx][1]);
                    }
                }
                mb->mvL[0][mbPartIdx][subMbPartIdx][0] = mvL0[0];
                mb->mvL[0][mbPartIdx][subMbPartIdx][1] = mvL0[1];
                mb->mvL[1][mbPartIdx][subMbPartIdx][0] = mvL1[1];
                mb->mvL[1][mbPartIdx][subMbPartIdx][1] = mvL1[1];
                mb->refIdxL[0][mbPartIdx] = refIdxL0;
                mb->refIdxL[1][mbPartIdx] = refIdxL1;
                mb->predFlagL[0][mbPartIdx] = predFlagL0;
                mb->predFlagL[1][mbPartIdx] = predFlagL1;
            }
        }
    }
}

void h264::get_mv_neighbor_part(ParserContext &ctx, int listSuffixFlag, int (&mvLA)[2],
                                int (&mvLB)[2], int (&mvLC)[2], int &refIdxLA,
                                int &refIdxLB, int &refIdxLC) {
    int64_t mbAddrA = ctx.mb->mbAddrA;
    int64_t mbAddrB = ctx.mb->mbAddrB;
    int64_t mbAddrC = ctx.mb->mbAddrC;

    auto mbPartIdxA = ctx.mb->mbPartIdxA;
    auto mbPartIdxB = ctx.mb->mbPartIdxB;
    auto mbPartIdxC = ctx.mb->mbPartIdxC;

    uint64_t slice_type = ctx.header()->slice_type;

    if (mbAddrA == -1 || is_mb_intra(ctx.mb_array[mbAddrA]->mb_type, slice_type) ||
            ctx.mb_array[mbAddrA]->predFlagL[listSuffixFlag][mbPartIdxA] == 0) {
        mvLA[0] = 0; mvLA[1] = 0;
        refIdxLA = -1;
    } else {
        int *m = ctx.mb_array[mbAddrA]->mvL[listSuffixFlag][mbPartIdxA][0];
        mvLA[0] = m[0]; mvLA[1] = m[1];
        refIdxLA = ctx.mb_array[mbAddrA]->refIdxL[listSuffixFlag][mbPartIdxA];
    }

    if (mbAddrB == -1 || is_mb_intra(ctx.mb_array[mbAddrB]->mb_type, slice_type) ||
        ctx.mb_array[mbAddrB]->predFlagL[listSuffixFlag][mbPartIdxB] == 0) {
        mvLB[0] = 0; mvLB[1] = 0;
        refIdxLB = -1;
    } else {
        int *m = ctx.mb_array[mbAddrB]->mvL[listSuffixFlag][mbPartIdxB][0];
        mvLB[0] = m[0]; mvLB[1] = m[1];
        refIdxLB = ctx.mb_array[mbAddrB]->refIdxL[listSuffixFlag][mbPartIdxB];
    }

    if (mbAddrC == -1 || is_mb_intra(ctx.mb_array[mbAddrC]->mb_type, slice_type) ||
        ctx.mb_array[mbAddrC]->predFlagL[listSuffixFlag][mbPartIdxC] == 0) {
        mvLC[0] = 0; mvLC[1] = 0;
        refIdxLC = -1;
    } else {
        int *m = ctx.mb_array[mbAddrC]->mvL[listSuffixFlag][mbPartIdxC][0];
        mvLC[0] = m[0]; mvLC[1] = m[1];
        refIdxLC = ctx.mb_array[mbAddrC]->refIdxL[listSuffixFlag][mbPartIdxC];
    }
}

void h264::process_luma_mv(ParserContext &ctx, uint32_t mbPartIdx,
                           int listSuffixFlag, int (&mvL)[2]) {
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    int refIdxLA = -1;
    int refIdxLB = -1;
    int refIdxLC = -1;
    int mvL0A[2];
    int mvL0B[2];
    int mvL0C[2];
    get_mv_neighbor_part(ctx, listSuffixFlag, mvL0A, mvL0B, mvL0C,
                         refIdxLA, refIdxLB, refIdxLC);
    process_luma_mv(ctx, mbPartIdx, mvL0A, mvL0B, mvL0C, refIdxLA,
                    refIdxLB, refIdxLC, mvL);

}

void h264::process_luma_mv(ParserContext &ctx,  uint32_t mbPartIdx, int (mvLA)[2],
                           int (mvLB)[2], int (mvLC)[2], int refIdxLA,
                           int refIdxLB, int refIdxLC, int (&mvL)[2]) {
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    uint64_t mbPartWidth = MbPartWidth(mb->mb_type);
    uint64_t mbPartHeight = MbPartHeight(mb->mb_type);
    if (mbPartWidth == 16 && mbPartHeight == 8 && mbPartIdx == 0) {
        mvL[0] = mvLB[0]; mvL[1] = mvLB[1];
    } else if (mbPartWidth == 16 && mbPartHeight == 8 && mbPartIdx == 1) {
        mvL[0] = mvLA[0]; mvL[1] = mvLA[1];
    } else if (mbPartWidth == 8 && mbPartHeight == 16 && mbPartIdx == 0) {
        mvL[0] = mvLA[0]; mvL[1] = mvLA[1];
    } else if (mbPartWidth == 8 && mbPartHeight == 16 && mbPartIdx == 1) {
        mvL[0] = mvLC[0]; mvL[1] = mvLC[1];
    } else {
        /* 8.4.1.3.1*/
        if (refIdxLA != -1 and refIdxLB == -1 and refIdxLC == -1) {
            mvL[0] = mvLA[0];
            mvL[1] = mvLA[1];
        } else if (refIdxLA == -1 and refIdxLB != -1 and refIdxLC == -1) {
            mvL[0] = mvLB[0];
            mvL[1] = mvLB[1];
        } else if (refIdxLA == -1 and refIdxLB == -1 and refIdxLC != -1) {
            mvL[0] = mvLC[0];
            mvL[1] = mvLC[1];
        } else {
            mvL[0] = std::max(std::min(mvLA[0], mvLB[0]),
                              std::min(std::max(mvLA[0], mvLB[0]), mvLC[0]));
            mvL[1] = std::max(std::min(mvLA[1], mvLB[1]),
                              std::min(std::max(mvLA[1], mvLB[1]), mvLC[1]));
        }
    }
}

MvFrame::MvFrame(ParserContext &ctx) : _mvs() {
    _height = ctx.Height();
    _width = ctx.Width();
    _mb_width = (uint32_t)ctx.PicWidthInMbs();
    _mb_height = (uint32_t)ctx.PicHeightInMapUnits();
    _mvs = std::vector<std::vector<MotionVector>>(_mb_height,
                                                  std::vector<MotionVector>(_mb_width));
    for (uint32_t i = 0; i < _mb_height; i++) {
        for (uint32_t j = 0; j < _mb_width; j++) {
            /* compute mb_addr */
            uint32_t mb_addr = i * _mb_width + j;
            std::shared_ptr<MacroBlock> mb = ctx.mb_array[mb_addr];
            if (mb->pos_x() != j || mb->pos_y() != i)
                throw std::runtime_error("pos does not match");
            MotionVector mv {
                    -mb->mvL[0][0][0][0] / 4,
                    -mb->mvL[0][0][0][1] / 4,
                    mb->pos_x() * 16,
                    mb->pos_y() * 16,
            };
            mv.energy = uint32_t(mv.mvL0[0] * mv.mvL0[0] +
                                         mv.mvL0[1] * mv.mvL0[1]);
            _mvs[i][j] = mv;
        }
    }
}

MvFrame::MvFrame(uint32_t pic_width, uint32_t pic_height, uint32_t mb_width,
                 uint32_t mb_height, bool p_frame)
        : _height(pic_height), _width(pic_width), _mb_width(mb_width),
          _mb_height(mb_height), _mvs(), _p_frame(p_frame) {
    _mvs = std::vector<std::vector<MotionVector>>(_mb_height,
                                                  std::vector<MotionVector>(_mb_width));
}

MotionVector MvFrame::get_mv(uint32_t x, uint32_t y) {
    return _mvs[y][x];
}

MotionVector MvFrame::get_mv(uint32_t mb_addr) {
    uint32_t j = mb_addr % _mb_width;
    uint32_t i = mb_addr / _mb_width;
    return _mvs[i][j];
}