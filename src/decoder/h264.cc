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
#include "../util/exception.hh"
#include "../util/filesystem.hh"

using std::shared_ptr;
using std::runtime_error;

BitStream::BitStream(std::string filename) : stream_(), chunk_offsets_() {
    if (!file_exists(filename))
        throw std::runtime_error(filename + " not found");
    stream_.open(filename);

    /* index chunks */
    BinaryReader br(stream_);
    uint32_t tag_size = 3;
    if (!search_nal(br, true, tag_size))
        throw std::runtime_error("no nal unit found");
    uint64_t pos = br.pos();
    while (search_nal(br, true, tag_size)) {
        uint64_t size = br.pos() - pos - tag_size;
        chunk_offsets_.emplace_back(std::make_pair(pos, size));
        pos = br.pos();
    }
    /* last one */
    uint64_t size = br.pos() - pos;
    chunk_offsets_.emplace_back(std::make_pair(pos, size));
}

std::string BitStream::extract_stream(uint64_t position, uint64_t size) {
    BinaryReader br(stream_);
    br.seek(position);
    return br.read_bytes(size);
}

h264::h264(const std::string &filename) : chunk_offsets_() {
    auto ext = file_extension(filename);
    if (ext == ".mp4") {
        mp4_ = std::make_shared<MP4File>(filename);
        load_mp4();
    } else if (ext == ".264" || ext == ".h264") {
        bit_stream_ = std::make_shared<BitStream>(filename);
        load_bitstream();
    } else {
        throw std::runtime_error("unsupported media file extension");
    }
}

h264::h264(std::shared_ptr<MP4File> mp4) : chunk_offsets_(),
                                           mp4_(std::move(mp4)) {
    load_mp4();
}

void h264::load_mp4() {
    if (!mp4_) throw std::runtime_error("mp4 is nullptr");
    auto tracks = mp4_->find_all("trak");
    std::shared_ptr<Box> box = nullptr;
    for (const auto & track : tracks) {
        auto b = track->find_first("avc1");
        if (b) {
            box = b;
            trak_box_ = track;
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

        length_size_ = (uint8_t)(avc1.avcC().length_size_minus_one() + 1);
    }
    if (!pps || !sps)
        throw std::runtime_error("sps or pps not found in mp4 file");
    sps_ = sps;
    pps_ = pps;
}

void h264::index_nal() {
    if (bit_stream_) return;
    auto box = trak_box_->find_first("stco");
    if (!box) box = trak_box_->find_first("co64");
    if (!box) throw std::runtime_error("stco/co64 not found");
    StcoBox stco = StcoBox(*box.get());
    box = trak_box_->find_first("stsc");
    if (!box) throw std::runtime_error("stsc not found");
    StscBox stsc = StscBox(box);
    box = trak_box_->find_first("stsz");
    if (!box) throw std::runtime_error("stsz not found");
    StszBox stsz = StszBox(box);

    auto sample_entries = stsz.entries();
    chunk_offsets_ = std::vector<uint64_t>(sample_entries.size());
    uint32_t chunk_count = 0;
    uint32_t stsc_index = 0;
    uint32_t samples_per_chunk = stsc.entries()[stsc_index].samples_per_chunk;
    uint64_t in_chunk_offset = 0;
    for (uint32_t i = 0; i < sample_entries.size(); i++) {
        uint32_t sample_size = sample_entries[i];
        uint64_t result = stco.chunk_offsets()[chunk_count] + in_chunk_offset;
        chunk_offsets_[i] = result;
        in_chunk_offset += sample_size;

        if (!samples_per_chunk) {
            /* switch chunk */
            chunk_count++;
            in_chunk_offset = 0;
            if (stsc_index + 1 < stsc.entries().size()
                && stsc.entries()[stsc_index + 1].first_chunk - 1
                   == chunk_count) {
                stsc_index++;
                samples_per_chunk =
                        stsc.entries()[stsc_index].samples_per_chunk;
            }
        }
    }
}

h264::h264(std::shared_ptr<BitStream> stream)
        : chunk_offsets_(), bit_stream_(std::move(stream)) {
    load_bitstream();
}

void h264::load_bitstream() {
    for (const auto & pair : bit_stream_->chunk_offsets()) {
        /* linear search. however, since most sps and pps are at the very
         * beginning, this is actually pretty fast
         */
        std::string data = bit_stream_->extract_stream(pair.first, pair.second);
        NALUnit unit(std::move(data));
        if (unit.nal_unit_type() == 7) {
            sps_ = std::make_shared<SPS_NALUnit>(unit);
        } else if (unit.nal_unit_type() == 8) {
            pps_ = std::make_shared<PPS_NALUnit>(unit);
        }
        if (sps_ && pps_)
            return;
    }
}

uint64_t h264::read_nal_size(BinaryReader &br) {
    uint32_t unit_size = 0;
    if (length_size_ == 4) {
        unit_size = br.read_uint32();
    } else if (length_size_ == 3) {
        uint32_t hi_byte = br.read_uint8();
        uint32_t low_bits = br.read_uint16();
        unit_size = hi_byte << 16 | low_bits;
    } else if (length_size_ == 2) {
        unit_size = br.read_uint16();
    } else if (length_size_ == 1) {
        unit_size = br.read_uint8();
    } else {
        throw std::runtime_error("unsupported length size");
    }
    return unit_size;
}

uint64_t h264::index_size() {
    if (bit_stream_) {
        return bit_stream_->chunk_offsets().size();
    } else {
        if (chunk_offsets_.empty())
            index_nal();
        return chunk_offsets_.size();
    }
}

MvFrame h264::load_frame(uint64_t frame_num) {
    std::string nal_data;
    if (bit_stream_) {
        uint64_t pos, size;
        std::tie(pos, size) = bit_stream_->chunk_offsets()[frame_num];
        nal_data = bit_stream_->extract_stream(pos, size);
    } else {
        if (chunk_offsets_.empty())
            index_nal();
        uint64_t offset = chunk_offsets_[frame_num];
        std::string size_string = mp4_->extract_stream(offset, length_size_);
        std::istringstream stream(size_string);
        BinaryReader br(stream);
        uint64_t unit_size = read_nal_size(br);
        nal_data = mp4_->extract_stream(offset + length_size_,
                                                    unit_size);
    }
    ParserContext ctx(sps_, pps_);
    /* test the slice type */
    if (!is_p_slice(static_cast<uint8_t>(nal_data[0])))
        return MvFrame(ctx.Width(), ctx.Height(), ctx.Width() / 16,
                       ctx.Height() / 16, false);
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
            // uint64_t partWidthC = partWidth / ctx.SubWidthC();
            // uint64_t partHeightC = partHeight / ctx.SubHeightC();

            // uint64_t MvCnt = 0;

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
                        refIdxL0 = static_cast<int>(
                                mb->mb_pred->ref_idx_l0[mbPartIdx]);
                        predFlagL0 = true;
                    } else {
                        refIdxL0 = -1;
                        predFlagL0 = false;
                    }

                    if (mb_pred_mode == Pred_L1 || mb_pred_mode == BiPred) {
                        refIdxL1 = static_cast<int>(
                                mb->mb_pred->ref_idx_l1[mbPartIdx]);
                        predFlagL1 = true;
                    } else {
                        refIdxL1 = -1;
                        predFlagL1 = false;
                    }

                    if (predFlagL0) {
                        int mvpL0[2] = {0, 0};
                        process_luma_mv(ctx, mbPartIdx, 0, mvpL0);
                        mvL0[0] = static_cast<int>(
                                mvpL0[0] + mb->mb_pred->mvd_l0[mbPartIdx]
                                           [subMbPartIdx][0]);
                        mvL0[1] =  static_cast<int>(
                                mvpL0[1] + mb->mb_pred->mvd_l0[mbPartIdx]
                                           [subMbPartIdx][1]);
                    }

                    if (predFlagL1) {
                        int mvpL1[2] = {0, 0};
                        process_luma_mv(ctx, mbPartIdx, 1, mvpL1);
                        mvL1[0] = static_cast<int>(
                                mvpL1[0] + mb->mb_pred->mvd_l1[mbPartIdx]
                                           [subMbPartIdx][0]);
                        mvL1[1] = static_cast<int>(
                                mvpL1[1] + mb->mb_pred->mvd_l1[mbPartIdx]
                                           [subMbPartIdx][1]);
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

void h264::get_mv_neighbor_part(ParserContext &ctx, int listSuffixFlag,
                                int (&mvLA)[2], int (&mvLB)[2], int (&mvLC)[2],
                                int &refIdxLA, int &refIdxLB, int &refIdxLC) {
    int64_t mbAddrA = ctx.mb->mbAddrA;
    int64_t mbAddrB = ctx.mb->mbAddrB;
    int64_t mbAddrC = ctx.mb->mbAddrC;

    auto mbPartIdxA = ctx.mb->mbPartIdxA;
    auto mbPartIdxB = ctx.mb->mbPartIdxB;
    auto mbPartIdxC = ctx.mb->mbPartIdxC;

    uint64_t slice_type = ctx.header()->slice_type;

    if (mbAddrA == -1
        || is_mb_intra(ctx.mb_array[mbAddrA]->mb_type, slice_type)
        || ctx.mb_array[mbAddrA]->predFlagL[listSuffixFlag][mbPartIdxA] == 0) {
        mvLA[0] = 0; mvLA[1] = 0;
        refIdxLA = -1;
    } else {
        int *m = ctx.mb_array[mbAddrA]->mvL[listSuffixFlag][mbPartIdxA][0];
        mvLA[0] = m[0]; mvLA[1] = m[1];
        refIdxLA = ctx.mb_array[mbAddrA]->refIdxL[listSuffixFlag][mbPartIdxA];
    }

    if (mbAddrB == -1
        || is_mb_intra(ctx.mb_array[mbAddrB]->mb_type, slice_type)
        || ctx.mb_array[mbAddrB]->predFlagL[listSuffixFlag][mbPartIdxB] == 0) {
        mvLB[0] = 0; mvLB[1] = 0;
        refIdxLB = -1;
    } else {
        int *m = ctx.mb_array[mbAddrB]->mvL[listSuffixFlag][mbPartIdxB][0];
        mvLB[0] = m[0]; mvLB[1] = m[1];
        refIdxLB = ctx.mb_array[mbAddrB]->refIdxL[listSuffixFlag][mbPartIdxB];
    }

    if (mbAddrC == -1
        || is_mb_intra(ctx.mb_array[mbAddrC]->mb_type, slice_type)
        || ctx.mb_array[mbAddrC]->predFlagL[listSuffixFlag][mbPartIdxC] == 0) {
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

void h264::process_luma_mv(ParserContext &ctx,  uint32_t mbPartIdx,
                           int (mvLA)[2], int (mvLB)[2], int (mvLC)[2],
                           int refIdxLA, int refIdxLB, int refIdxLC,
                           int (&mvL)[2]) {
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
        if (refIdxLA != -1 && refIdxLB == -1 && refIdxLC == -1) {
            mvL[0] = mvLA[0];
            mvL[1] = mvLA[1];
        } else if (refIdxLA == -1 && refIdxLB != -1 && refIdxLC == -1) {
            mvL[0] = mvLB[0];
            mvL[1] = mvLB[1];
        } else if (refIdxLA == -1 && refIdxLB == -1 && refIdxLC != -1) {
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

MvFrame::MvFrame(ParserContext &ctx) : mvs_() {
    height_ = ctx.Height();
    width_ = ctx.Width();
    mb_width_ = (uint32_t)ctx.PicWidthInMbs();
    mb_height_ = (uint32_t)ctx.PicHeightInMapUnits();
    mvs_ = std::vector<std::vector<MotionVector>>(
            mb_height_, std::vector<MotionVector>(mb_width_));
    for (uint32_t i = 0; i < mb_height_; i++) {
        for (uint32_t j = 0; j < mb_width_; j++) {
            /* compute mb_addr */
            uint32_t mb_addr = i * mb_width_ + j;
            std::shared_ptr<MacroBlock> mb = ctx.mb_array[mb_addr];
            if (mb->pos_x() != j || mb->pos_y() != i)
                throw std::runtime_error("pos does not match");
            MotionVector mv {
                    -mb->mvL[0][0][0][0] / 4.0f,
                    -mb->mvL[0][0][0][1] / 4.0f,
                    mb->pos_x() * 16,
                    mb->pos_y() * 16,
            };
            mv.energy = uint32_t(mv.mvL0[0] * mv.mvL0[0] +
                                         mv.mvL0[1] * mv.mvL0[1]);
            mvs_[i][j] = mv;
        }
    }
}

MvFrame::MvFrame(const MvFrame &frame) : height_(frame.height_),
                                         width_(frame.width_),
                                         mb_width_(frame.mb_width_),
                                         mb_height_(frame.mb_height_), mvs_() {
    mvs_ = std::vector<std::vector<MotionVector>>(
            mb_height_, std::vector<MotionVector>(mb_width_));

    for (uint32_t i = 0; i < mb_height_; i++) {
        for (uint32_t j = 0; j < mb_width_; j++) {
            mvs_[i][j] = frame.mvs_[i][j];
        }
    }
}

MvFrame::MvFrame(uint32_t pic_width, uint32_t pic_height, uint32_t mb_width,
                 uint32_t mb_height, bool p_frame)
        : height_(pic_height), width_(pic_width), mb_width_(mb_width),
          mb_height_(mb_height), mvs_(), p_frame_(p_frame) {
    mvs_ = std::vector<std::vector<MotionVector>>(
            mb_height_, std::vector<MotionVector>(mb_width_));
    for (uint32_t i = 0; i < mb_height_; i++) {
        for (uint32_t j = 0; j < mb_width_; j++) {
            MotionVector mv;
            mv.x = j;
            mv.y = i;
            mvs_[i][j] = mv;
        }
    }
}

MotionVector MvFrame::get_mv(uint32_t x, uint32_t y) const {
    return mvs_[y][x];
}

MotionVector MvFrame::get_mv(uint32_t mb_addr) const {
    uint32_t j = mb_addr % mb_width_;
    uint32_t i = mb_addr / mb_width_;
    return mvs_[i][j];
}
