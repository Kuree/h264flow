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
#include <algorithm>
#include <string>
#include "nal.hh"
#include "util.hh"
#include "../util/exception.hh"


using std::shared_ptr;
using std::make_shared;


NALUnit::NALUnit(BinaryReader & br, uint32_t size, bool unescape)
        : _nal_ref_idc(), _nal_unit_type(), _data() {
    decode_header(br);
    if (unescape) {
        /* TODO: re-enable unescape in BitReader */
        // std::ostringstream stream;
        // BinaryWriter bw(stream);
        // unescape_rbsp(br, bw, size - 1);
        _data = br.read_bytes(size - 1);
    } else {
        _data = br.read_bytes(size - 1);
    }
}

NALUnit::NALUnit(std::string data, bool unescape): _nal_ref_idc(),
                                                   _nal_unit_type(), _data() {
    std::istringstream stream(data);
    BinaryReader br(stream);
    decode_header(br);
    if (unescape) {
        std::ostringstream s;
        BinaryWriter bw(s);
        unescape_rbsp(br, bw, data.length() - 1);
        _data = s.str();
    } else {
        _data = br.read_bytes(data.length() - 1);
    }
}


void NALUnit::decode_header(BinaryReader & br) {
    uint8_t tmp = br.read_uint8();
    if (tmp & 0x80)
        throw std::runtime_error("forbidden 0 bit is set");
    _nal_ref_idc = tmp >> 5;
    _nal_unit_type = static_cast<uint8_t >(tmp & 0x1F);
}

void SPS_NALUnit::parse() {
    std::stringstream stream(_data);
    BinaryReader br(stream);

    profile_idc_ = br.read_uint8();
    flags_ = br.read_uint8();
    level_idc_ = br.read_uint8();
    sps_id_ = br.read_ue();
    if (profile_idc_ == 100 || profile_idc_ == 110 || profile_idc_ == 122 ||
        profile_idc_ == 244 || profile_idc_ == 44 || profile_idc_ ==  83 ||
        profile_idc_ == 86 || profile_idc_ == 118 || profile_idc_ == 128) {
        chroma_format_idc_ = br.read_ue();
        if (chroma_format_idc_ == 3)
            separate_colour_plane_flag_ = br.read_bit(); /* 3 -> 4:4:4 */
        else
            separate_colour_plane_flag_ = false; /* 1->4:2:0    2-> 4:2:2 */


        bit_depth_luma_minus8_ = br.read_ue();
        bit_depth_chroma_minus8_ = br.read_ue();
        qpprime_y_zero_transform_bypass_flag_ = br.read_bit_as_bool();
        seq_scaling_matrix_present_flag_ = br.read_bit_as_bool();
        if (seq_scaling_matrix_present_flag_)
            throw NotImplemented("seq_scaling_matrix_present_flag");
    }
    log2_max_frame_num_minus4_ = br.read_ue();
    pic_order_cnt_type_ = br.read_ue();
    if (pic_order_cnt_type_ == 0) {
        log2_max_pic_order_cnt_lsb_minus4_ = br.read_ue();
    } else if (pic_order_cnt_type_ == 1) {
        delta_pic_order_always_zero_flag_ = br.read_bit_as_bool();
        offset_for_non_ref_pic_ = br.read_se();
        offset_for_top_to_bottom_field_ = br.read_se();
        num_ref_frames_in_pic_order_cnt_cycle_ = br.read_ue();
        for (uint32_t i = 0 ; i < num_ref_frames_in_pic_order_cnt_cycle_; i++)
            offset_for_ref_frame_.emplace_back(br.read_se());
    }
    max_num_ref_frames_ = br.read_ue();
    gaps_in_frame_num_value_allowed_flag_ = br.read_bit_as_bool();
    pic_width_in_mbs_minus1_ = br.read_ue();
    pic_height_in_map_units_minus1_ = br.read_ue();
    frame_mbs_only_flag_ = br.read_bit_as_bool();
    if (!frame_mbs_only_flag_)
        mb_adaptive_frame_field_flag_ = br.read_bit_as_bool();
    direct_8x8_inference_flag_ = br.read_bit_as_bool();
    frame_cropping_flag_ = br.read_bit_as_bool();
    if (frame_cropping_flag_) {
        frame_crop_left_offset_ = br.read_ue();
        frame_crop_right_offset_ = br.read_ue();
        frame_crop_top_offset_ = br.read_ue();
        frame_crop_bottom_offset_ = br.read_ue();
    }

    vui_parameters_present_flag_ = br.read_bit_as_bool();
    if (vui_parameters_present_flag_) {
        // not parsed yet
    }
}

SPS_NALUnit::SPS_NALUnit(std::string data) : NALUnit(std::move(data)),
                                             offset_for_ref_frame_() {
    parse();
}

SPS_NALUnit::SPS_NALUnit(NALUnit & unit) : NALUnit(unit),
                                           offset_for_ref_frame_() {
    parse();
}

PPS_NALUnit::PPS_NALUnit(std::string data) : NALUnit(std::move(data)),
                                             run_length_minus1_(),
                                             top_left_(), bottom_right_(),
                                             slice_group_id_() {
    parse();
}

PPS_NALUnit::PPS_NALUnit(NALUnit &unit) : NALUnit(unit) , run_length_minus1_(),
                                          top_left_(), bottom_right_(),
                                          slice_group_id_() {
    parse();
}

void PPS_NALUnit::parse() {
    BitReader br(_data);

    pps_id_ = br.read_ue();
    sps_id_ = br.read_ue();
    entropy_coding_mode_flag_ = br.read_bit_as_bool();
    if (entropy_coding_mode_flag_)
        throw NotImplemented("entropy_coding_mode_flag");
    bottom_field_pic_order_in_frame_present_flag_ = br.read_bit_as_bool();
    num_slice_groups_minus1_ = br.read_ue();
    if (num_slice_groups_minus1_ > 0) {
        slice_group_map_type_ = br.read_ue();
        if (slice_group_map_type_ == 0) {
            for (uint64_t i = 0; i <= num_slice_groups_minus1_; i++) {
                run_length_minus1_.emplace_back(br.read_ue());
            }
        } else if (slice_group_map_type_ == 2) {
            for (uint64_t i = 0; i <= num_slice_groups_minus1_; i++) {
                top_left_.emplace_back(br.read_ue());
                bottom_right_.emplace_back(br.read_ue());
            }
        } else if (slice_group_map_type_ == 3 || slice_group_map_type_ == 4 ||
                   slice_group_map_type_ == 5 || slice_group_map_type_ == 6) {
            slice_group_change_direction_flag_ = br.read_bit_as_bool();
            slice_group_change_rate_minus1_ = br.read_ue();
        } else if (slice_group_map_type_ == 6) {
            /* this requires pps input, currently not supported */
            throw NotImplemented("_slice_group_map_type == 6");
            // pic_size_in_map_units_minus1 = br.read_ue();
            // for (uint64_t i = 0; i <= pic_size_in_map_units_minus1; i++) {
            //     slice_group_id.emplace_back(br.read_ue());
            // }
        }
    }
    num_ref_idx_l0_default_active_minus1_ = br.read_ue();
    num_ref_idx_l1_default_active_minus1_ = br.read_ue();
    weighted_pred_flag_ = br.read_bit_as_bool();
    weighted_bipred_idc_ = br.read_bits(2);
    pic_init_qp_minus26_ = br.read_se();
    pic_init_qs_minus26_ = br.read_se();
    chroma_qp_index_offset_ = br.read_se();
    deblocking_filter_control_present_flag_ = br.read_bit_as_bool();
    constrained_intra_pred_flag_ = br.read_bit_as_bool();
    redundant_pic_cnt_present_flag_ = br.read_bit_as_bool();
    /* the rest is not parsed */
}

Slice_NALUnit::Slice_NALUnit(std::string data) : NALUnit(std::move(data)) {
    _header = std::make_shared<SliceHeader>(*this, _data);
    _slice_data = std::make_shared<SliceData>(*this);
}

Slice_NALUnit::Slice_NALUnit(NALUnit &unit) : NALUnit(unit) {
    _header = std::make_shared<SliceHeader>(*this, _data);
    _slice_data = std::make_shared<SliceData>(*this);
}

void Slice_NALUnit::parse(ParserContext & ctx) {;
    BitReader br(_data);
    _header->parse(ctx, br);
    ctx.set_header(_header);
    _slice_data->parse(ctx, br);
}

void SliceHeader::parse(ParserContext &ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;

    /* assign default value */
    num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1();
    num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1();

    first_mb_in_slice = br.read_ue();
    slice_type = br.read_ue();
    pps_id = br.read_ue();
    if (pps_id != pps->pps_id())
        throw std::runtime_error("pps id does not match");
    if (sps->separate_colour_plane_flag())
        colour_plane_id = static_cast<uint8_t >(br.read_bits(2));
    frame_num = br.read_bits(sps->log2_max_frame_num_minus4() + 4);
    if (!sps->frame_mbs_only_flag()) {
        field_pic_flag = br.read_bit_as_bool();
        if (field_pic_flag)
            bottom_field_flag = br.read_bit_as_bool();
    }
    if (_nal.idr_pic_flag())
        idr_pic_id = br.read_ue();
    if (sps->pic_order_cnt_type() == 0)
        pic_order_cnt_lsb = br.read_bits(
                sps->log2_max_pic_order_cnt_lsb_minus4() + 4);
    if (pps->bottom_field_pic_order_in_frame_present_flag() && !field_pic_flag )
        delta_pic_order_cnt_bottom = br.read_se();
    if (sps->pic_order_cnt_type() == 1 &&
            !sps->delta_pic_order_always_zero_flag()) {
        delta_pic_order_cnt[0] = br.read_se();
        if (pps->bottom_field_pic_order_in_frame_present_flag() &&
                !field_pic_flag )
            delta_pic_order_cnt[1] = br.read_se();
    }
    if (pps->redundant_pic_cnt_present_flag())
        redundant_pic_cnt = br.read_ue();
    if (slice_type == SliceType::TYPE_B)
        direct_spatial_mv_pred_flag = br.read_bit_as_bool();
    if ((slice_type == SliceType::TYPE_P) || (slice_type == SliceType::TYPE_SP)
        ||  (slice_type == SliceType::TYPE_B)) {
        num_ref_idx_active_override_flag = br.read_bit_as_bool();
        if (num_ref_idx_active_override_flag) {
            num_ref_idx_l0_active_minus1 = br.read_ue();
            if (slice_type == SliceType::TYPE_B)
                num_ref_idx_l1_active_minus1 = br.read_ue();
        }
    }

    if (_nal.nal_unit_type() == 20)
        throw NotImplemented("ref_pic_list_mvc_modification");

    rplm = RefPicListModification(slice_type, br);
    if ((pps->weighted_pred_flag() && (slice_type == SliceType::TYPE_P ||
            slice_type == SliceType::TYPE_SP)) ||
            (pps->weighted_bipred_idc() == 1
             && (slice_type == SliceType::TYPE_B))) {
        pwt = PredWeightTable(sps, pps, slice_type, br);
    }
    if (_nal.nal_ref_idc())
        drpm = DecRefPicMarking(_nal, br);
    if (pps->entropy_coding_mode_flag() && slice_type != SliceType::TYPE_I &&
            slice_type != SliceType::TYPE_SI)
        cabac_init_idc = br.read_ue();
    slice_qp_delta = br.read_se();
    if (slice_type == SliceType::TYPE_SP || slice_type == SliceType::TYPE_SI) {
        if (slice_type == SliceType::TYPE_SP)
            sp_for_switch_flag = br.read_bit_as_bool();
        slice_qs_delta = br.read_se();
    }

    if (pps->deblocking_filter_control_present_flag()) {
        disable_deblocking_filter_idc = br.read_ue();
        if (disable_deblocking_filter_idc != 1) {
            slice_alpha_c0_offset_div2 = br.read_se();
            slice_beta_offset_div2 = br.read_se();
        }
    }

    if (pps->num_slice_groups_minus1() > 0 && pps->slice_group_map_type() >= 3
        && pps->slice_group_map_type() <= 5) {
        uint64_t pic_size_in_map_units = pps->pic_size_in_map_units_minus1()
                                         + 1;
        uint64_t slice_group_change_rate =
                pps->slice_group_change_rate_minus1() + 1;
        uint64_t bits = intlog2(pic_size_in_map_units /
                                        slice_group_change_rate + 1);
        slice_group_change_cycle = br.read_bits(bits);
    }
}

RefPicListModification::RefPicListModification()
        : modification_of_pic_nums_idc(), abs_diff_pic_num_minus1(),
          long_term_pic_num(), abs_diff_view_idx_minus1() {}

RefPicListModification::RefPicListModification(uint64_t slice_type,
                                               BitReader &br)
        :RefPicListModification() {
    if (slice_type % 5 != 2 && slice_type % 5 != 4) {
        ref_pic_list_modification_flag_l0 = br.read_bit_as_bool();
        if (ref_pic_list_modification_flag_l0) {
            uint64_t mod = 0;
            do {
                mod = br.read_ue();
                modification_of_pic_nums_idc.emplace_back(mod);
                if (mod == 0 || mod == 1)
                    abs_diff_pic_num_minus1.emplace_back(br.read_ue());
                else if (mod == 2)
                    long_term_pic_num.emplace_back(br.read_ue());
                else if (mod == 4 || mod == 5)
                    abs_diff_view_idx_minus1.emplace_back(br.read_ue());
            } while (mod != 3);
        }
    }
    if (slice_type % 5 == 1) {
        ref_pic_list_modification_flag_l1 = br.read_bit_as_bool();
        if (ref_pic_list_modification_flag_l1) {
            uint64_t mod = 0;
            do {
                mod = br.read_ue();
                modification_of_pic_nums_idc.emplace_back(mod);
                if (mod == 0 || mod == 1)
                    abs_diff_pic_num_minus1.emplace_back(br.read_ue());
                else if (mod == 2)
                    long_term_pic_num.emplace_back(br.read_ue());
                else if (mod == 4 || mod == 5)
                    abs_diff_view_idx_minus1.emplace_back(br.read_ue());
            } while (mod != 3);
        }
    }
}

PredWeightTable::PredWeightTable() : luma_weight_l0(), luma_offset_l0(),
                                     luma_weight_l1(), luma_offset_l1(),
                                     chroma_weight_l0(), chroma_offset_l0(),
                                     chroma_weight_l1(), chroma_offset_l1() {}

PredWeightTable::PredWeightTable(std::shared_ptr<SPS_NALUnit> sps,
                                 std::shared_ptr<PPS_NALUnit> pps,
                                 uint64_t slice_type,
                                 BitReader &br) : PredWeightTable() {
    luma_log2_weight_denom = br.read_ue();
    if (sps->chroma_array_type())
        chroma_log2_weight_denom = br.read_ue();
    uint64_t num_ref_idx_l0 = pps->num_ref_idx_l0_default_active_minus1() + 1;
    uint64_t num_ref_idx_l1 = pps->num_ref_idx_l1_default_active_minus1() + 1;
    luma_weight_l0 = std::vector<int64_t>(num_ref_idx_l0);
    luma_offset_l0 = std::vector<int64_t>(num_ref_idx_l0);
    luma_weight_l1 = std::vector<int64_t>(num_ref_idx_l1);
    luma_offset_l1 = std::vector<int64_t>(num_ref_idx_l1);
    chroma_weight_l0 = std::vector<std::array<int64_t, 2>>(num_ref_idx_l0);
    chroma_offset_l0 = std::vector<std::array<int64_t, 2>>(num_ref_idx_l0);
    chroma_weight_l1 = std::vector<std::array<int64_t, 2>>(num_ref_idx_l1);
    chroma_offset_l1 = std::vector<std::array<int64_t, 2>>(num_ref_idx_l1);
    for (uint32_t i = 0; i < num_ref_idx_l0; i++) {
        bool luma_weight_l0_flag = br.read_bit_as_bool();
        if (luma_weight_l0_flag) {
            luma_weight_l0[i] = br.read_se();
            luma_offset_l0[i] = br.read_se();
        }
        if (sps->chroma_array_type()) {
            bool chroma_weight_l0_flag = br.read_bit_as_bool();
            if (chroma_weight_l0_flag) {
                for (int j = 0; j < 2; j++) {
                    chroma_weight_l0[i][j] = br.read_se();
                    chroma_offset_l0[i][j] = br.read_se();
                }
            }
        }
    }

    if (slice_type % 5 == 1) {
        for (uint32_t i = 0; i < num_ref_idx_l1; i++) {
            bool luma_weight_l1_flag = br.read_bit_as_bool();
            if (luma_weight_l1_flag) {
                luma_weight_l1[i] = br.read_se();
                luma_offset_l1[i] = br.read_se();
            }
            if (sps->chroma_array_type()) {
                bool chroma_weight_l1_flag = br.read_bit_as_bool();
                if (chroma_weight_l1_flag) {
                    for (int j = 0; j < 2; j++) {
                        chroma_weight_l1[i][j] = br.read_se();
                        chroma_offset_l1[i][j] = br.read_se();
                    }
                }
            }
        }
    }
}

DecRefPicMarking::DecRefPicMarking() :memory_management_control_operation(),
                                      difference_of_pic_nums_minus1(),
                                      long_term_pic_num(),
                                      long_term_frame_idx(),
                                      max_long_term_frame_idx_plus1() {}

DecRefPicMarking::DecRefPicMarking(const NALUnit &unit, BitReader &br)
        : DecRefPicMarking() {
    if (unit.idr_pic_flag()) {
        no_output_of_prior_pics_flag = br.read_bit_as_bool();
        long_term_reference_flag = br.read_bit_as_bool();
    } else {
        adaptive_ref_pic_marking_mode_flag = br.read_bit_as_bool();
        if (adaptive_ref_pic_marking_mode_flag) {
            uint64_t op = 0;
            do {
                op = br.read_ue();
                memory_management_control_operation.emplace_back(op);
                if (op == 1 || op == 3)
                    difference_of_pic_nums_minus1.emplace_back(br.read_ue());
                if (op == 2)
                    long_term_pic_num.emplace_back(br.read_ue());
                if (op == 3 || op == 6)
                    long_term_frame_idx.emplace_back(br.read_ue());
                if (op == 4)
                    max_long_term_frame_idx_plus1.emplace_back(br.read_ue());
            } while (op);
        }
    }
}

void SliceData::find_trailing_bit() {
    BitReader br(_nal.data());
    uint64_t pos = br.size() - 1;
    /* search from the back */
    while (true) {
        br.seek(pos);
        uint8_t  tmp = br.read_uint8();
        for (int i = 0; i < 8; i++) {
            if (tmp & (1 << i)) {
                _trailing_bit = pos * 8 + (7 - i);
                return;
            }
        }
        pos--;
    }
}

bool SliceData::more_rbsp_data(BitReader &br) {
    if (!_trailing_bit)
        find_trailing_bit();
    return (br.pos() * 8 + br.bit_pos()) < _trailing_bit;
}

void SliceData::parse(ParserContext & ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    if (pps->entropy_coding_mode_flag())
        throw NotImplemented("entropy_coding_mode_flag");
    bool mbaff_frame_flag = this->mbaff_frame_flag(sps, header);
    uint64_t curr_mb_addr = header->first_mb_in_slice * (1 + mbaff_frame_flag);
    bool more_data_flag = true;
    bool prev_mb_skipped = false;
    do {
        bool mb_skip_flag;
        uint64_t mb_skip_run;
        if (header->slice_type != SliceType::TYPE_I
            && header->slice_type != SliceType::TYPE_SI) {
            if (!pps->entropy_coding_mode_flag()) {
                mb_skip_run = br.read_ue();
                prev_mb_skipped = mb_skip_run > 0;
                for (uint64_t i = 0; i < mb_skip_run; i++) {
                    std::shared_ptr<MacroBlock> block = make_shared<MacroBlock>
                            (ctx, false, curr_mb_addr);
                    ctx.mb_array[curr_mb_addr++] = block;
                    // curr_mb_addr = next_mb_addr(curr_mb_addr, ctx);
                }
                if (mb_skip_run > 0)
                    more_data_flag = more_rbsp_data(br);
            } else {
                mb_skip_flag = br.read_bit_as_bool();
                more_data_flag = !mb_skip_flag;
            }
        }
        if (more_data_flag) {
            bool mb_field_decoding_flag = false;
            if (mbaff_frame_flag && (curr_mb_addr % 2 == 0
                                     || (curr_mb_addr % 2 == 1
                                         && prev_mb_skipped)))
                mb_field_decoding_flag = br.read_bit_as_bool();
            std::shared_ptr<MacroBlock> block = make_shared<MacroBlock>
                    (ctx, mb_field_decoding_flag, curr_mb_addr);
            ctx.mb = block;
            ctx.mb_array[curr_mb_addr++] = block;
            ctx.mb->parse(ctx, br);
        }
        if (!pps->entropy_coding_mode_flag()) {
            more_data_flag = more_rbsp_data(br);
        } else {
            throw NotImplemented("entropy_coding_mode_flag");
        }
        // curr_mb_addr = next_mb_addr(curr_mb_addr,ctx);
    } while (more_data_flag);
    /* sanity check */
    if (curr_mb_addr != ctx.PicSizeInMbs())
        throw std::runtime_error("slice data parsing unfinished");
}

uint64_t SliceData::next_mb_addr(uint64_t n, ParserContext &) {
    // std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    // std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    // std::shared_ptr<SliceHeader> header = ctx.header();
    /* Based on eqn 7-24 -> 7-28 and 8-16 */
    // uint64_t i = n + 1;
    // std::vector<uint64_t> MbToSliceGroupMap = slice_group_map(sps, pps);
    // while (i < ctx.PicSizeInMbs() && MbToSliceGroupMap[i] !=
    // MbToSliceGroupMap[n])
    //    i++;
    // return i;
    return n + 1;
}


std::vector<uint64_t> SliceData::slice_group_map(
        std::shared_ptr<SPS_NALUnit> sps, std::shared_ptr<PPS_NALUnit> pps) {
    uint64_t PicHeightInMapUnits = sps->pic_height_in_map_units_minus1() + 1;
    uint64_t PicWidthInMbs = sps->pic_width_in_mbs_minus1() + 1;
    uint64_t PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
    uint64_t i = 0;
    std::vector<uint64_t> mapUnitToSliceGroupMap(PicSizeInMapUnits);

    uint64_t num_slice_groups = pps->num_slice_groups_minus1() + 1;
    if (num_slice_groups == 1) {
        for (i = 0; i < PicSizeInMapUnits; i++)
            mapUnitToSliceGroupMap[i] = 0;
        return mapUnitToSliceGroupMap;
    }
    if (pps->slice_group_map_type() == 0) {
        do {
            for (uint64_t iGroup = 0; iGroup <= num_slice_groups
                                      && i < PicSizeInMapUnits;
                 i += pps->run_length_minus1()[iGroup++] + 1) {
                for (uint64_t j = 0; j <= pps->run_length_minus1()[iGroup]
                                     && i + j < PicSizeInMapUnits; j++)
                    mapUnitToSliceGroupMap[i + j] = iGroup;
            }
        } while (i < PicSizeInMapUnits);
    } else if (pps->slice_group_map_type() == 1) {
        for (i = 0; i < PicSizeInMapUnits; i++ )
            mapUnitToSliceGroupMap[i] = ((i % PicWidthInMbs) +
                    (((i / PicWidthInMbs) * num_slice_groups) / 2))
                                        % num_slice_groups;
    } else if (pps->slice_group_map_type() == 2) {
        for (i = 0; i < PicSizeInMapUnits; i++)
            mapUnitToSliceGroupMap[i] = num_slice_groups - 1;
        for (int iGroup = static_cast<int>(num_slice_groups) - 2;
             iGroup >= 0; iGroup--) {
            uint64_t yTopLeft = pps->top_left()[iGroup] / PicWidthInMbs;
            uint64_t xTopLeft = pps->top_left()[iGroup] % PicWidthInMbs;
            uint64_t yBottomRight = pps->bottom_right()[iGroup] / PicWidthInMbs;
            uint64_t xBottomRight = pps->bottom_right()[iGroup] % PicWidthInMbs;
            for (uint64_t y = yTopLeft; y <= yBottomRight; y++)
                for (uint64_t x = xTopLeft; x <= xBottomRight; x++)
                    mapUnitToSliceGroupMap[y * PicWidthInMbs + x] =
                            (uint32_t)iGroup;
        }
    }
    return mapUnitToSliceGroupMap;
}

MacroBlock::MacroBlock(bool mb_field_decoding_flag, uint64_t curr_mb_addr)
        : mb_pred(), sub_mb_preds(),
          mb_field_decoding_flag(mb_field_decoding_flag),
          mb_addr(curr_mb_addr) {
    memset(TotalCoeffs_luma, 0, sizeof(int) * 16);
    memset(TotalCoeffs_chroma, 0, sizeof(int) * 8); /* 4 * 2 */
    memset(LumaLevel4x4, 0, sizeof(int) * 256); /* 16 * 16 */
    memset(LumaLevel8x8, 0, sizeof(int) * 256); /* 4 * 64 */
    memset(Intra16x16DCLevel, 0, sizeof(int) * 16);
    memset(Intra16x16ACLevel, 0, sizeof(int) * 240); /* 16 * 15 */
    memset(ChromaDCLevel, 0, sizeof(int) * 8);
    memset(ChromaACLevel, 0, sizeof(int) * 120); /* 2 * 4 * 15 */

    memset(mvL, 0, sizeof(int) * 64);  /* 2 * 4 * 4 * 2 */
    memset(predFlagL, 0, sizeof(int) * 8);  /* 2 * 4 */
    memset(refIdxL, 0, sizeof(int) * 8);  /* 2 * 4 */
}

MacroBlock::MacroBlock(ParserContext &ctx, bool mb_field_decoding_flag,
                       uint64_t curr_mb_addr) :
        MacroBlock(mb_field_decoding_flag, curr_mb_addr) {
    /* compute neighbours */
    compute_mb_neighbours(ctx);
    assign_pos(ctx);
}

void MacroBlock::parse(ParserContext & ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    slice_type = header->slice_type;

    mb_type = br.read_ue();
    if (mb_type == I_PCM) {
        br.set_bit_pos(0);
        uint64_t bit_depth_luma = 8 + sps->bit_depth_luma_minus8();
        /* PCM luma */
        for (int i = 0; i < 256; i++) {
            br.read_bits(bit_depth_luma);
        }
        uint64_t bit_depth_chroma = 8 + sps->bit_depth_chroma_minus8();
        /* PCM chroma */
        for (uint32_t i = 0; i < 2 * ctx.MbWidthC() * ctx.MbHeightC(); i++)
            br.read_bits(bit_depth_chroma);
    } else {
        bool noSubMbPartSizeLessThan8x8Flag = true;
        if (mb_type != I_NxN
            && MbPartPredMode(mb_type, 0, header->slice_type) != Intra_16x16
            && NumMbPart(mb_type) == 4) {
            SubMbPred sub_mb_pred;
            sub_mb_pred.parse(ctx, br);
            for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
                if (sub_mb_pred.sub_mb_type[mbPartIdx] != B_Direct_8x8)
                    noSubMbPartSizeLessThan8x8Flag = false;
                else if (!sps->direct_8x8_inference_flag())
                    noSubMbPartSizeLessThan8x8Flag = false;
            }
        } else {
            if (pps->transform_8x8_mode_flag() && mb_type == I_NxN) {
                /* Note: this is only true for !entropy */
                if (pps->entropy_coding_mode_flag())
                    throw NotImplemented("entropy_coding_mode not implemented");
                transform_size_8x8_flag = br.read_bit_as_bool();
            }
            // mb_pred
            mb_pred = std::make_unique<MbPred>();
            mb_pred->parse(ctx, br);
        }

        if (MbPartPredMode(mb_type, 0, header->slice_type) != Intra_16x16) {
            uint64_t coded_block_pattern = br.read_ue();
            if (coded_block_pattern > 47)
                throw std::runtime_error("incorrect coded block pattern");
            coded_block_pattern = MbPartPredMode(mb_type, 0, header->slice_type)
                                  == Intra_4x4?
                    codeNum_to_coded_block_pattern_intra[coded_block_pattern] :
                                  codeNum_to_coded_block_pattern_inter[
                                          coded_block_pattern];
            CodedBlockPatternLuma = coded_block_pattern % 16;
            CodedBlockPatternChroma = coded_block_pattern / 16;

            if (CodedBlockPatternLuma > 0 &&
                pps->transform_8x8_mode_flag() && mb_type != I_NxN &&
                noSubMbPartSizeLessThan8x8Flag &&
                (mb_type != B_Direct_16x16
                 || sps->direct_8x8_inference_flag())) {
                transform_size_8x8_flag = br.read_bit_as_bool();
            }
        } else if (header->slice_type == SliceType::TYPE_I && mb_type > 4) {
            /* TODO: this is only a temporary fix. clean this up */
            if (mb_type > 12) {
                CodedBlockPatternLuma = 15;
                if (mb_type > 20)
                    CodedBlockPatternChroma = 2;
                else if (mb_type > 16)
                    CodedBlockPatternChroma = 1;
            } else {
                 if (mb_type > 8)
                     CodedBlockPatternChroma = 2;
                 else if (mb_type > 4)
                     CodedBlockPatternChroma = 1;
            }
        }

        int mb_part = MbPartPredMode(mb_type, 0, header->slice_type);
        bool is_intra = mb_part == Intra_16x16 || Intra_4x4;
        if (CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 ||
                is_intra) {
            mb_qp_delta = br.read_se();
            /* residual here */
            residual = std::make_unique<Residual>(0, 15);
            residual->parse(ctx, br);
        }
    }
    /* based on mb_type */
    compute_mb_index(ctx);
}

void MacroBlock::compute_mb_neighbours(ParserContext &ctx) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    uint64_t PicWidthInMbs = sps->pic_width_in_mbs_minus1() + 1;
    if (mb_addr % PicWidthInMbs != 0)
        mbAddrA = mb_addr - 1;
    else
        mbAddrA = -1;

    if (mb_addr >= PicWidthInMbs)
        mbAddrB = mb_addr - PicWidthInMbs;
    else
        mbAddrB = -1;
    if (mb_addr >= PicWidthInMbs &&
        (mb_addr + 1) % PicWidthInMbs != 0) {
        mbAddrC = mb_addr - PicWidthInMbs + 1;
    } else {
        mbAddrC = -1;
    }
    if (mb_addr > PicWidthInMbs &&
        mb_addr % PicWidthInMbs != 0) {
        mbAddrD = mb_addr - PicWidthInMbs - 1;
    } else {
        mbAddrD = -1;
    }
    /* will be overridden for non-skipped mb */
    compute_mb_index(ctx);
}

void MacroBlock::compute_mb_index(ParserContext &ctx) {
    /* compute MbPartIdx table */
    uint64_t numMbPart = NumMbPart(mb_type);
    // if (numMbPart == 4)
    //     throw NotImplemented("numMbPart == 4");
    for (uint32_t mbPartIdx = 0; mbPartIdx < numMbPart; mbPartIdx++) {
        int x = 0; int y = 0;
        if (numMbPart == 2) {
            if (MbPartWidth(mb_type) == 8) {
                x = 8; y = 0;
            } else {
                x = 0; y = 8;
            }
        }
        x /= 4;
        y /= 4;
        /* minimum is 4 x 4 pixel block */
        for (uint32_t j = 0; j < MbPartHeight(mb_type) / 4; j++) {
            for (uint32_t i = 0; i < MbPartWidth(mb_type) / 4; i++) {
                mbPartIdxTable[x + i][y + j] = mbPartIdx;
            }
        }
    }

    int64_t addrs[4] = {mbAddrA, mbAddrB, mbAddrC, mbAddrD};
    int64_t * addr_index[4] =
            {&mbPartIdxA, &mbPartIdxB, &mbPartIdxC, &mbPartIdxD};
    for (int i = 0; i < 4; i++) {
        int64_t mb_addr = addrs[i];
        if (mb_addr == -1) continue;
        auto mb = ctx.mb_array[mb_addr];
        uint64_t x = (mb->_pos_x + 16) % 16;
        uint64_t y = (mb->_pos_y + 16) % 16;
        x /= 4;
        y /= 4;
        *addr_index[i] = mb->mbPartIdxTable[x][y];
    }

    if ((mb_type == Intra_16x16 || mb_type == Intra_4x4) && mbAddrC == -1) {
        mbPartIdxC = mbPartIdxD;
    }
}

void MacroBlock::assign_pos(ParserContext & ctx) {
    uint64_t PicWidthInMbs = ctx.PicWidthInMbs();
    uint64_t FrameHeightInMbs = ctx.FrameHeightInMbs();
    if (mb_addr <= (PicWidthInMbs * FrameHeightInMbs)) {
        _pos_x = mb_addr % PicWidthInMbs;
        _pos_y = mb_addr / PicWidthInMbs;
    } else {
        throw std::runtime_error("mb_addr out of range");
    }
}

MbPred::MbPred() {
    for (int i = 0; i < 16; i++) {
        prev_intra4x4_pred_mode_flag[i] = false;
        rem_intra4x4_pred_mode[i] = 0;
    }
}

void MbPred::parse(ParserContext &ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    auto type = (uint32_t) MbPartPredMode(mb->mb_type, 0, header->slice_type);

    /* TODO: Intra_8x8 is not implemented */
    if (type == Intra_4x4 || type == Intra_16x16) {
        if (type == Intra_4x4) {
            for (int luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
                prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] =
                        br.read_bit_as_bool();
                if (!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
                    rem_intra4x4_pred_mode[luma4x4BlkIdx] =
                            (uint8_t)br.read_bits(3);
                }
            }
        }
        if (sps->chroma_array_type() == 1 || sps->chroma_array_type() == 2)
            intra_chroma_pred_mode = br.read_ue();
    } else if (MbPartPredMode(mb->mb_type, 0, header->slice_type) != Direct) {
        uint64_t num_mb_part = NumMbPart(mb->mb_type);
        uint64_t range_te0 = header->num_ref_idx_l0_active_minus1 + 1;
        uint64_t range_te1 = header->num_ref_idx_l1_active_minus1 + 1;
        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) {
            if ((range_te0 > 1 ||
                    mb->mb_field_decoding_flag != header->field_pic_flag) &&
                MbPartPredMode(mb->mb_type, mbPartIdx,
                               header->slice_type) != Pred_L1)
            ref_idx_l0[mbPartIdx] = br.read_te(range_te0);
        }

        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) {
            if ((range_te1 > 1 ||
                 mb->mb_field_decoding_flag != header->field_pic_flag) &&
                MbPartPredMode(mb->mb_type, mbPartIdx, header->slice_type) !=
                Pred_L0)
                ref_idx_l1[mbPartIdx] = br.read_te(range_te1);
        }

        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) {
            if (MbPartPredMode(mb->mb_type, mbPartIdx, header->slice_type) !=
                Pred_L1) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l0[mbPartIdx][0][compIdx] = br.read_se();
            }
        }

        for (uint32_t mbPartIdx = 0; mbPartIdx < NumMbPart(mb->mb_type);
             mbPartIdx++) {
            if (MbPartPredMode(mb->mb_type, mbPartIdx, header->slice_type) !=
                Pred_L0) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l1[mbPartIdx][0][compIdx] = br.read_se();
            }
        }
    }
}


void SubMbPred::parse(ParserContext &ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    uint64_t te0 = header->num_ref_idx_l0_active_minus1 + 1;
    uint64_t te1 = header->num_ref_idx_l1_active_minus1 + 1;
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        sub_mb_type[mbPartIdx] = br.read_ue();
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if ((te0 > 1 ||
             mb->mb_field_decoding_flag != header->field_pic_flag) &&
            mb->mb_type != P_8x8ref0 &&
            sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header->slice_type)
            != Pred_L1) {
            ref_idx_l0[mbPartIdx] = br.read_te(te0);
        }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if ((te1 > 1 ||
             mb->mb_field_decoding_flag != header->field_pic_flag) &&
            mb->mb_type != P_8x8ref0 &&
            sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header->slice_type)
            != Pred_L0) {
            ref_idx_l1[mbPartIdx] = br.read_te(te1);
        }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header->slice_type)
            != Pred_L1) {
            for (uint32_t subMbPartIdx = 0; subMbPartIdx <
                                            NumSubMbPart(sub_mb_type[mbPartIdx],
                                                         header->slice_type);
                 subMbPartIdx++) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l0[mbPartIdx][subMbPartIdx][compIdx] = br.read_se();
            }
        }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header->slice_type)
            != Pred_L0) {
            for (uint32_t subMbPartIdx = 0; subMbPartIdx <
                                            NumSubMbPart(sub_mb_type[mbPartIdx],
                                                         header->slice_type);
                 subMbPartIdx++) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l1[mbPartIdx][subMbPartIdx][compIdx] = br.read_se();
            }
        }
    }
}

void Residual::parse(ParserContext &ctx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    // residual_block_cavlc
    residual_luma(ctx, 0, 15, br);
    residual_chroma(ctx, 0, 15, br);
}

void Residual::residual_luma(ParserContext &ctx, const int startIdx,
                             const int endIdx, BitReader &br) {
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    std::shared_ptr<SliceHeader> header = ctx.header();
    if (startIdx == 0 && MbPartPredMode(mb->mb_type, 0, header->slice_type)
                         == Intra_16x16) {
        if (ctx.pps->entropy_coding_mode_flag()) {
            throw NotImplemented("entropy_coding_mode_flag");
        } else {
            auto block = std::make_shared<ResidualBlock>(
                    0, 15, 16, BlockType::blk_LUMA_16x16_DC, 0);
            block->parse(ctx,  mb->Intra16x16DCLevel, br);
            residual_blocks.emplace_back(block);
        }
    }

    /* some other hacks to make it consistent with JM */
    // if (mb->mb_type == 23 || mb->mb_type == 18 || mb->mb_type == 26
    //     || mb->mb_type == 22 || mb->mb_type == 27) {
    //     mb->CodedBlockPatternLuma = 0xF;
    // }
    if (MbPartHeight(mb->mb_type) == 15) {
        mb->CodedBlockPatternLuma = 0xF;
    }

    int blkIdx = 0;
    for (int i8x8 = 0; i8x8 < 4; i8x8++) {
        if (!mb->transform_size_8x8_flag
            || !ctx.pps->entropy_coding_mode_flag()) {
            for (int i4x4 = 0; i4x4 < 4; i4x4++) {
                blkIdx = i8x8 * 4 + i4x4;

                if (mb->CodedBlockPatternLuma & (1 << i8x8)) {
                    if (MbPartPredMode(mb->mb_type, 0, header->slice_type)
                        == Intra_16x16) {
                        if (ctx.pps->entropy_coding_mode_flag()) {
                            throw NotImplemented("entropy_coding_mode_flag");
                        } else {
                            auto block = std::make_shared<ResidualBlock>(
                                    std::max(0, startIdx - 1), endIdx - 1, 15,
                                    BlockType::blk_LUMA_16x16_AC, blkIdx);
                            block->parse(ctx, mb->Intra16x16ACLevel[blkIdx],
                                         br);
                            residual_blocks.emplace_back(block);
                        }
                    } else {
                        if (ctx.pps->entropy_coding_mode_flag()) {
                            throw NotImplemented("entropy_coding_mode_flag");
                        } else {
                            auto block = std::make_shared<ResidualBlock>(
                                    startIdx, endIdx, 16,
                                    BlockType::blk_LUMA_4x4, blkIdx);
                            block->parse(ctx, mb->LumaLevel4x4[blkIdx],
                                         br);
                            residual_blocks.emplace_back(block);
                        }
                    }
                } else if (MbPartPredMode(mb->mb_type, 0, header->slice_type)
                           == 1) {
                    for (int i = 0; i < 15; i++) {
                        mb->Intra16x16ACLevel[blkIdx][i] = 0;
                        mb->TotalCoeffs_luma[blkIdx] = 0;
                    }
                } else {
                    for (int i = 0; i < 16; i++) {
                        mb->LumaLevel4x4[blkIdx][i] = 0;
                        mb->TotalCoeffs_luma[blkIdx] = 0;
                    }
                }

                if (!ctx.pps->entropy_coding_mode_flag() &&
                    mb->transform_size_8x8_flag) {
                    for (int i = 0; i < 16; i++) {
                        mb->LumaLevel8x8[i8x8][4 * i + i4x4] =
                                mb->LumaLevel4x4[blkIdx][i];
                    }
                }
            }
        } else if (mb->CodedBlockPatternLuma & (1 << i8x8)) {
            if (ctx.pps->entropy_coding_mode_flag()) {
                throw NotImplemented("entropy_coding_mode_flag");
            } else {
                auto block = std::make_shared<ResidualBlock>(
                        4 * startIdx, 4 * endIdx + 3, 64,
                        BlockType::blk_LUMA_8x8, i8x8);
                block->parse(ctx, mb->LumaLevel8x8[i8x8],
                             br);
                residual_blocks.emplace_back(block);
            }
        } else {
            for (int i = 0; i < 64; i++) {
                mb->LumaLevel8x8[i8x8][i] = 0;
            }
        }
    }
}

void Residual::residual_chroma(ParserContext &ctx, const int startIdx,
                               const int endIdx, BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    std::shared_ptr<SliceHeader> header = ctx.header();

    /* FIXME: this is a hack to make it the same with JM reference player */
    bool intra_dc = false;
    bool intra_ac = false;
    if (MbPartWidth(mb->mb_type) == 2) {
        intra_dc = true;
        intra_ac = true;
    } else if (MbPartWidth(mb->mb_type) == 1) {
        intra_dc = true;
    }

    uint64_t chroma_array_type = sps->chroma_array_type();
    if (chroma_array_type == 1 || chroma_array_type == 2) {
        int NumC8x8 = 4 / (ctx.SubWidthC() * ctx.SubHeightC());
        for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
            if ((mb->CodedBlockPatternChroma & 3 || intra_dc) &&
                    (startIdx == 0)) {
                if (ctx.pps->entropy_coding_mode_flag()) {
                    throw NotImplemented("entropy_coding_mode_flag");
                } else {
                    auto block = std::make_shared<ResidualBlock>(
                            0, 4 * NumC8x8 - 1, 4 * NumC8x8,
                            BlockType::blk_CHROMA_DC_Cb + iCbCr, 0);
                    block->parse(ctx, mb->ChromaDCLevel[iCbCr],
                                 br);
                    residual_blocks.emplace_back(block);
                }
            } else {
                for (int i = 0; i < (4 * NumC8x8); i++) {
                    mb->ChromaDCLevel[iCbCr][i] = 0;
                }
            }
        }

        for (int iCbCr = 0; iCbCr < 2; iCbCr++) {
            for (int i8x8 = 0; i8x8 < NumC8x8; i8x8++) {
                for (int i4x4 = 0; i4x4 < 4; i4x4++) {
                    int blkIdx = i8x8*4 + i4x4;
                    if (mb->CodedBlockPatternChroma & 2 || intra_ac) {
                        if (ctx.pps->entropy_coding_mode_flag()) {
                            throw NotImplemented("entropy_coding_mode_flag");
                        } else {
                            auto block = std::make_shared<ResidualBlock>(
                                    std::max(0, startIdx - 1), endIdx - 1, 15,
                                    BlockType::blk_CHROMA_AC_Cb +
                                            iCbCr, blkIdx);
                            block->parse(ctx, mb->ChromaACLevel[iCbCr][blkIdx],
                                         br);
                            residual_blocks.emplace_back(block);
                        }
                    } else {
                        for (int i = 0; i < 15; i++) {
                            mb->ChromaACLevel[iCbCr][blkIdx][i] = 0;
                        }
                    }
                }
            }
        }
    }
}

// taken from https://github.com/emericg/MiniVideo/
void ResidualBlock::parse(ParserContext &ctx, int *coeffLevel,
                          BitReader &br) {
    std::shared_ptr<SPS_NALUnit> sps = ctx.sps;
    std::shared_ptr<PPS_NALUnit> pps = ctx.pps;
    std::shared_ptr<SliceHeader> header = ctx.header();
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    int mbAddrA_temp = -1, mbAddrB_temp = -1;
    int blkA = 0, blkB = 0;
    int nA = 0, nB = 0, nC = 0;  // Number of coefficients (TotalCoeff) in
    // neighboring blocks

    // 9.2.1 Parsing process for total number of transform coefficient
    // levels and trailing ones
    if (block_type == BlockType::blk_CHROMA_DC_Cb
        || block_type == BlockType::blk_CHROMA_DC_Cr) {
        if (sps->chroma_array_type() == 1)
            nC = -1;
        else  // if (dc->ChromaArrayType == 2)
            nC = -2;
    } else {
        // 4
        if (block_type == BlockType::blk_LUMA_16x16_DC
            || block_type == BlockType::blk_LUMA_16x16_AC
            || block_type == BlockType::blk_LUMA_4x4
            || block_type == BlockType::blk_LUMA_8x8) {
            deriv_4x4lumablocks(ctx, block_index, mbAddrA_temp,
                                blkA, mbAddrB_temp, blkB);
        } else if (block_type == BlockType::blk_CHROMA_AC_Cb
                   || block_type == BlockType::blk_CHROMA_AC_Cr) {
            deriv_4x4chromablocks(ctx, block_index, mbAddrA_temp, blkA,
                                  mbAddrB_temp, blkB);
        }
        if (mbAddrA_temp != -1) {  // 5 6 - A
            if (((header->slice_type == 0 || header->slice_type == 5)
                 && ctx.mb_array[mbAddrA_temp]->mb_type == P_Skip)
                || ((header->slice_type == 1 || header->slice_type == 6)
                    && ctx.mb_array[mbAddrA_temp]->mb_type == B_Skip)) {
                nA = 0;
            } else if ((header->slice_type == 2 || header->slice_type == 7)
                       && ctx.mb_array[mbAddrA_temp]->mb_type == I_PCM) {
                nA = 16;
            } else {
                if (static_cast<int>(block_type) < 4)
                    nA = ctx.mb_array[mbAddrA_temp]->TotalCoeffs_luma[blkA];
                else if (block_type == BlockType::blk_CHROMA_AC_Cb)
                    nA = ctx.mb_array[mbAddrA_temp]->
                            TotalCoeffs_chroma[0][blkA];
                else if (block_type == BlockType::blk_CHROMA_AC_Cr)
                    nA = ctx.mb_array[mbAddrA_temp]->
                            TotalCoeffs_chroma[1][blkA];
                else
                    throw NotImplemented("block_type > 5");
            }
        }
        if (mbAddrB_temp != -1) {  // 5 6 - B
            if (((header->slice_type == 0 || header->slice_type == 5)
                 && ctx.mb_array[mbAddrB_temp]->mb_type == P_Skip) ||
                ((header->slice_type == 1 || header->slice_type == 6)
                 && ctx.mb_array[mbAddrB_temp]->mb_type == B_Skip)) {
                nB = 0;
            } else if ((header->slice_type == 2 || header->slice_type == 7)
                     && ctx.mb_array[mbAddrB_temp]->mb_type == I_PCM) {
                nB = 16;
            } else {
                if (static_cast<int>(block_type) < 4)
                    nB = ctx.mb_array[mbAddrB_temp]->TotalCoeffs_luma[blkB];
                else if (block_type  == BlockType::blk_CHROMA_AC_Cb)
                    nB = ctx.mb_array[mbAddrB_temp]->
                            TotalCoeffs_chroma[0][blkB];
                else if (block_type  == BlockType::blk_CHROMA_AC_Cr)
                    nB = ctx.mb_array[mbAddrB_temp]->
                            TotalCoeffs_chroma[1][blkB];
                else
                    throw NotImplemented("block_type > 5");
            }
        }

        // 7
        if (mbAddrA_temp != -1 && mbAddrB_temp != -1)
            nC = (nA + nB + 1) >> 1;
        else if (mbAddrA_temp != -1 && mbAddrB_temp == -1)
            nC = nA;
        else if (mbAddrA_temp == -1 && mbAddrB_temp != -1)
            nC = nB;
        else
            nC = 0;
    }
    uint8_t coeff_token = read_coeff_token(nC, br);
    uint8_t TotalCoeffs   = coeff_token >> 2;
    uint8_t TrailingOnes = coeff_token % 4;

    if (static_cast<uint32_t>(block_type) < 4)
        mb->TotalCoeffs_luma[block_index] = TotalCoeffs;
    else if (block_type == BlockType::blk_CHROMA_AC_Cb)
        mb->TotalCoeffs_chroma[0][block_index] = TotalCoeffs;
    else if (block_type == BlockType::blk_CHROMA_AC_Cr)
        mb->TotalCoeffs_chroma[1][block_index] = TotalCoeffs;
    // else
    //    throw std::runtime_error("Could not save TotalCoeffs");

    if (TotalCoeffs > 0 && TotalCoeffs <= max_num_coeff) {
        // 9.2.2 Parsing process for level information
        int level[16];  // maximum value for TotalCoeffs
        int run[16];  // maximum value for TotalCoeffs
        int suffixLength = 0;

        if (TotalCoeffs > 10 && TrailingOnes < 3)
            suffixLength = 1;


        for (int i = 0; i < TotalCoeffs; i++) {
            if (i < TrailingOnes) {
                bool trailing_ones_sign_flag = br.read_bit();
                level[i] = 1 - 2 * trailing_ones_sign_flag;
            } else {
                int level_prefix = read_ce_levelprefix(br);
                int levelCode = std::min(15, level_prefix) << suffixLength;

                if (suffixLength > 0 || level_prefix >= 14) {
                    int levelSuffixSize = suffixLength;

                    if (level_prefix == 14 && suffixLength == 0)
                        levelSuffixSize = 4;
                    else if (level_prefix > 14)
                        levelSuffixSize = level_prefix - 3;

                    auto level_suffix = br.read_bits(
                            static_cast<uint32_t>(levelSuffixSize));
                    levelCode += level_suffix;
                }

                if (level_prefix >= 15 && suffixLength == 0)
                    levelCode += 15;

                if (level_prefix >= 16)
                    levelCode += (1 << (level_prefix - 3)) - 4096;

                if (i == TrailingOnes && TrailingOnes < 3)
                    levelCode += 2;

                if (levelCode % 2 == 0)
                    level[i] = (levelCode + 2) >> 1;
                else
                    level[i] = (-levelCode - 1) >> 1;

                if (suffixLength == 0)
                    suffixLength = 1;

                {
                    int vtest = (3 << (suffixLength - 1));
                    if ((abs(level[i]) > vtest) && (suffixLength < 6)) {
                        suffixLength++;
                    }
                }
            }
        }

        // 9.2.3 Parsing process for run information
        int zerosLeft = 0;
        if (TotalCoeffs < end_index - start_index + 1) {
            int vlcnum = TotalCoeffs - 1;
            int total_zeros = 0;

            if (block_type != BlockType ::blk_CHROMA_DC_Cb
                && block_type != BlockType::blk_CHROMA_DC_Cr)
                total_zeros = read_ce_totalzeros(br, vlcnum, 0);
            else
                total_zeros = read_ce_totalzeros(br, vlcnum, 1);

            zerosLeft = total_zeros;
        } else {
            zerosLeft = 0;
        }

        for (int i = 0; i < TotalCoeffs - 1; i++) {
            if (zerosLeft > 0) {
                int vlcnum = std::min(zerosLeft - 1, RUNBEFORE_NUM_M1);
                int run_before = read_ce_runbefore(br, vlcnum);
                run[i] = run_before;
            } else {
                run[i] = 0;
            }

            zerosLeft -= run[i];
        }

        // Decoded coefficients :
        run[TotalCoeffs - 1] = zerosLeft;
        int coeffNum = -1;
        for (int i = TotalCoeffs - 1; i >= 0; i--) {
            coeffNum += run[i] + 1;
            coeffLevel[start_index + coeffNum] = level[i];
        }
    }
}

void ResidualBlock::deriv_4x4lumablocks(ParserContext &ctx,
                                        const int luma4x4BlkIdx,
                                        int &mbAddrA, int &luma4x4BlkIdxA,
                                        int &mbAddrB, int &luma4x4BlkIdxB) {
    int x = 0, y = 0;
    InverseLuma4x4BlkScan(luma4x4BlkIdx, x, y);

    int xA = x - 1;
    int yA = y;

    int xB = x;
    int yB = y - 1;

    int xW = 0, yW = 0;

    // luma4x4BlkIdxA derivation
    deriv_neighbouringlocations(ctx, true, xA, yA, mbAddrA, xW, yW);
    if (mbAddrA > -1)
        luma4x4BlkIdxA = deriv_4x4lumablock_indices(xW, yW);
    else
        luma4x4BlkIdxA = -1;

    // luma4x4BlkIdxB derivation
    deriv_neighbouringlocations(ctx, true, xB, yB, mbAddrB, xW, yW);
    if (mbAddrB > -1)
        luma4x4BlkIdxB = deriv_4x4lumablock_indices(xW, yW);
    else
        luma4x4BlkIdxB = -1;
}

inline void ResidualBlock::InverseLuma4x4BlkScan(const int luma4x4BlkIdx,
                                                 int &x, int &y) {
    x = InverseRasterScan_x(luma4x4BlkIdx / 4, 8, 8, 16)
        + InverseRasterScan_x(luma4x4BlkIdx % 4, 4, 4, 8);
    y = InverseRasterScan_y(luma4x4BlkIdx / 4, 8, 8, 16)
        + InverseRasterScan_y(luma4x4BlkIdx % 4, 4, 4, 8);
}

void ResidualBlock::deriv_neighbouringlocations(ParserContext &ctx,
                                                const bool lumaBlock,
                                                const int xN, const int yN,
                                                int &mbAddrN, int &xW,
                                                int &yW) {
    std::shared_ptr<MacroBlock> mb = ctx.mb;
    int maxW = 16;
    int maxH = 16;
    if (!lumaBlock) {
        maxW = ctx.MbWidthC();
        maxH = ctx.MbHeightC();
    }
    // Checking neighbour's availability with neighbourmacroblock_availability()
    // Now use info contained in each macroblock, to avoid useless computation

    // Specification of mbAddrN (Table 6-3 with optimizations)
    if (yN > -1) {
        if (xN < 0)
            mbAddrN = static_cast<int>(mb->mbAddrA);
        else if (xN < maxW)
            mbAddrN = static_cast<int>(mb->mb_addr);
        else
            return;
    } else {
        if (xN < 0)
            mbAddrN = static_cast<int>(mb->mbAddrD);
        else if (xN < maxW)
            mbAddrN = static_cast<int>(mb->mbAddrB);
        else
            mbAddrN = static_cast<int>(mb->mbAddrC);
    }
    xW = (xN + maxW) % maxW;
    yW = (yN + maxH) % maxH;
}

void ResidualBlock::deriv_4x4chromablocks(ParserContext &ctx,
                                          const int chroma4x4BlkIdx,
                                          int &mbAddrA, int &chroma4x4BlkIdxA,
                                          int &mbAddrB,
                                          int &chroma4x4BlkIdxB) {
    int x = InverseRasterScan_x(chroma4x4BlkIdx, 4, 4, 8);
    int y = InverseRasterScan_y(chroma4x4BlkIdx, 4, 4, 8);
    int xA = x - 1;
    int yA = y;

    int xB = x;
    int yB = y - 1;

    int xW = 0, yW = 0;

    // chroma4x4BlkIdxA derivation
    deriv_neighbouringlocations(ctx, false, xA, yA, mbAddrA, xW, yW);
    if (mbAddrA > -1)
        chroma4x4BlkIdxA = deriv_4x4chromablock_indices(xW, yW);
    else
        chroma4x4BlkIdxA = -1;

    // chroma4x4BlkIdxB derivation
    deriv_neighbouringlocations(ctx, false, xB, yB, mbAddrB, xW, yW);
    if (mbAddrB > -1)
        chroma4x4BlkIdxB = deriv_4x4chromablock_indices(xW, yW);
    else
        chroma4x4BlkIdxB = -1;
}

uint32_t ParserContext::SubHeightC() {
    if (sps->chroma_format_idc() == 1)
        return 2;
    else if (sps->chroma_format_idc() == 2)
        return 1;
    else if (sps->chroma_format_idc() == 3 &&
             !sps->separate_colour_plane_flag())
        return 1;
    else
        throw std::runtime_error("unsupported SubHeightC");
}

uint32_t ParserContext::SubWidthC() {
    if (sps->chroma_format_idc() == 1 || sps->chroma_format_idc() == 2)
        return 2;
    else if (sps->chroma_format_idc() == 3 &&
             sps->separate_colour_plane_flag())
        return 1;
    else
        throw std::runtime_error("unsupported SubWidthC");
}

void ParserContext::set_header(std::shared_ptr<SliceHeader> header) {
    _header = std::move(header);
    mb_array = std::vector<std::shared_ptr<MacroBlock>>(PicSizeInMbs());
}

uint32_t ParserContext::Height() {
    return (uint32_t)((FrameHeightInMbs() * 16)
                      - (sps->frame_crop_top_offset() * 2)
                      - (sps->frame_crop_bottom_offset() * 2));
}

uint32_t ParserContext::Width() {
    return (uint32_t)((PicWidthInMbs() * 16)
                      - sps->frame_crop_right_offset() * 2
                      - sps->frame_crop_left_offset()*2);
}
