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

#include "nal.hh"
#include "util.hh"
#include "consts.hh"
#include <sstream>

using std::shared_ptr;
using std::make_shared;


NALUnit::NALUnit(BinaryReader & br, uint32_t size, bool unescape)
        : _nal_ref_idc(), _nal_unit_type(), _data() {
    decode_header(br);
    if (unescape) {
        std::ostringstream stream;
        BinaryWriter bw(stream);
        unescape_rbsp(br, bw, size - 1);
        _data = stream.str();
    } else {
        _data = br.read_bytes(size - 1);
    }
}

NALUnit::NALUnit(std::string data): _nal_ref_idc(),
                                    _nal_unit_type(), _data() {
    std::istringstream stream(data);
    BinaryReader br(stream);
    decode_header(br);
    _data = br.read_bytes(data.length() - 1);
}


void NALUnit::decode_header(BinaryReader & br) {
    uint8_t tmp = br.read_uint8();
    if (tmp & 0x80)
        throw std::runtime_error("forbidden 0 bit is set");
    _nal_ref_idc = tmp >> 5;
    _nal_unit_type = static_cast<uint8_t >(tmp & 0x1F);
}


SPS_NALUnit::SPS_NALUnit(std::string data) : NALUnit(std::move(data)),
                                             _offset_for_ref_frame() {
    parse();
}


void SPS_NALUnit::parse() {
    std::stringstream stream(_data);
    BinaryReader br(stream);

    _profile_idc = br.read_uint8();
    _flags = br.read_uint8();
    _level_idc = br.read_uint8();
    _sps_id = br.read_ue();
    if( _profile_idc == 100 || _profile_idc == 110 || _profile_idc == 122 ||
        _profile_idc == 244 || _profile_idc == 44 || _profile_idc ==  83 ||
        _profile_idc == 86 || _profile_idc == 118 || _profile_idc == 128) {
        _chroma_format_idc = br.read_ue();
        if (_chroma_format_idc == 3)
            _separate_colour_plane_flag = br.read_bit(); /* 3 -> 4:4:4 */
        else
            _separate_colour_plane_flag = false; /* 1->4:2:0    2-> 4:2:2 */


        _bit_depth_luma_minus8 = br.read_ue();
        _bit_depth_chroma_minus8 = br.read_ue();
        _qpprime_y_zero_transform_bypass_flag = br.read_bit_as_bool();
        _seq_scaling_matrix_present_flag = br.read_bit_as_bool();
        if (_seq_scaling_matrix_present_flag)
            throw NotImplemented("seq_scaling_matrix_present_flag");
    }
    _log2_max_frame_num_minus4 = br.read_ue();
    _pic_order_cnt_type = br.read_ue();
    if (_pic_order_cnt_type == 0) {
        _log2_max_pic_order_cnt_lsb_minus4 = br.read_ue();
    } else if (_pic_order_cnt_type == 1) {
        _delta_pic_order_always_zero_flag = br.read_bit_as_bool();
        _offset_for_non_ref_pic = br.read_se();
        _offset_for_top_to_bottom_field = br.read_se();
        _num_ref_frames_in_pic_order_cnt_cycle = br.read_ue();
        for (uint32_t i = 0 ; i < _num_ref_frames_in_pic_order_cnt_cycle; i++)
            _offset_for_ref_frame.emplace_back(br.read_se());
    }
    _max_num_ref_frames = br.read_ue();
    _gaps_in_frame_num_value_allowed_flag = br.read_bit_as_bool();
    _pic_width_in_mbs_minus1 = br.read_ue();
    _pic_height_in_map_units_minus1 = br.read_ue();
    _frame_mbs_only_flag = br.read_bit_as_bool();
    if (!_frame_mbs_only_flag)
        _mb_adaptive_frame_field_flag = br.read_bit_as_bool();
    _direct_8x8_inference_flag = br.read_bit_as_bool();
    _frame_cropping_flag = br.read_bit_as_bool();
    if (_frame_cropping_flag) {
        _frame_crop_left_offset = br.read_ue();
        _frame_crop_right_offset = br.read_ue();
        _frame_crop_top_offset = br.read_ue();
        _frame_crop_bottom_offset = br.read_ue();
    }

    _vui_parameters_present_flag = br.read_bit_as_bool();
    if (_vui_parameters_present_flag) {
        // not parsed yet
    }
}

SPS_NALUnit::SPS_NALUnit(NALUnit & unit) : NALUnit(unit),
                                           _offset_for_ref_frame() {
    parse();
}

PPS_NALUnit::PPS_NALUnit(std::string data) : NALUnit(std::move(data)), _run_length_minus1(),
                                             _top_left(), _bottom_right(),
                                             _slice_group_id(){
    parse();
}

PPS_NALUnit::PPS_NALUnit(NALUnit &unit) : NALUnit(unit) , _run_length_minus1(),
                                          _top_left(), _bottom_right(),
                                          _slice_group_id(){
    parse();
}

void PPS_NALUnit::parse() {
    std::stringstream stream(_data);
    BinaryReader br(stream);

    _pps_id = br.read_ue();
    _sps_id = br.read_ue();
    _entropy_coding_mode_flag = br.read_bit_as_bool();
    if (_entropy_coding_mode_flag)
        throw NotImplemented("entropy_coding_mode_flag");
    _bottom_field_pic_order_in_frame_present_flag = br.read_bit_as_bool();
    _num_slice_groups_minus1 = br.read_ue();
    if (_num_slice_groups_minus1 > 0) {
        _slice_group_map_type = br.read_ue();
        if (_slice_group_map_type == 0) {
            for (uint64_t i = 0; i <= _num_slice_groups_minus1; i++) {
                _run_length_minus1.emplace_back(br.read_ue());
            }
        } else if (_slice_group_map_type == 2) {
            for (uint64_t i = 0; i <= _num_slice_groups_minus1; i++) {
                _top_left.emplace_back(br.read_ue());
                _bottom_right.emplace_back(br.read_ue());
            }
        } else if (_slice_group_map_type == 3 || _slice_group_map_type == 4 ||
                   _slice_group_map_type == 5 || _slice_group_map_type == 6) {
            _slice_group_change_direction_flag = br.read_bit_as_bool();
            _slice_group_change_rate_minus1 = br.read_ue();
        } else if (_slice_group_map_type == 6) {
            /* this requires pps input, currently not supported */
            throw NotImplemented("_slice_group_map_type == 6");
            // pic_size_in_map_units_minus1 = br.read_ue();
            // for (uint64_t i = 0; i <= pic_size_in_map_units_minus1; i++) {
            //     slice_group_id.emplace_back(br.read_ue());
            // }
        }
    }
    _num_ref_idx_l0_default_active_minus1 = br.read_ue();
    _num_ref_idx_l1_default_active_minus1 = br.read_ue();
    _weighted_pred_flag = br.read_bit_as_bool();
    _weighted_bipred_idc = br.read_bits(2);
    _pic_init_qp_minus26 = br.read_se();
    _pic_init_qs_minus26 = br.read_se();
    _chroma_qp_index_offset = br.read_se();
    _deblocking_filter_control_present_flag = br.read_bit_as_bool();
    _constrained_intra_pred_flag = br.read_bit_as_bool();
    _redundant_pic_cnt_present_flag = br.read_bit_as_bool();
    if (!br.eof()) {
        _transform_8x8_mode_flag = br.read_bit_as_bool();
        _pic_scaling_matrix_present_flag = br.read_bit_as_bool();
        if (_pic_scaling_matrix_present_flag)
            throw NotImplemented("pic_scaling_matrix_present_flag");
        /* the rest is not parsed */
    }

}

Slice_NALUnit::Slice_NALUnit(std::string data) : NALUnit(std::move(data)),
                                                 _header(*this, _data),
                                                 _slice_data(*this) {}

Slice_NALUnit::Slice_NALUnit(NALUnit &unit) : NALUnit(unit),
                                              _header(*this, _data),
                                              _slice_data(*this) {}
void Slice_NALUnit::parse(std::shared_ptr<SPS_NALUnit> sps,
                          std::shared_ptr<PPS_NALUnit> pps) {
    _header.parse(std::move(sps), std::move(pps));

}

void SliceHeader::parse(std::shared_ptr<SPS_NALUnit> sps,
                        std::shared_ptr<PPS_NALUnit> pps) {
    std::stringstream stream(_data);
    BinaryReader br(stream);

    first_mb_in_slice = br.read_ue();
    slice_type = br.read_ue();
    pps_id = br.read_ue();
    if (pps_id != pps->pps_id())
        throw std::runtime_error("pps id does not match");
    if (sps->separate_colour_plane_flag())
        colour_plane_id = static_cast<uint8_t >(br.read_bits(2));
    frame_num = br.read_bits(sps->log2_max_frame_num_minus4() + 4);
    if(!sps->frame_mbs_only_flag()) {
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
        uint64_t bits = intlog2(pps->pic_size_in_map_units_minus1() +
                           pps->slice_group_change_rate_minus1() + 1);
        slice_group_change_cycle = br.read_bits(bits);
    }

    _header_size = br.pos();
}

RefPicListModification::RefPicListModification()
        : modification_of_pic_nums_idc(), abs_diff_pic_num_minus1(),
          long_term_pic_num(), abs_diff_view_idx_minus1() {}

RefPicListModification::RefPicListModification(uint64_t slice_type,
                                               BinaryReader &br)
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
                                 BinaryReader &br) : PredWeightTable() {
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
                for (int j = 0; j < 2; j++){
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
                    for (int j = 0; j < 2; j++){
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

DecRefPicMarking::DecRefPicMarking(const NALUnit &unit, BinaryReader &br)
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

void SliceData::parse(std::shared_ptr<SPS_NALUnit> sps,
                      std::shared_ptr<PPS_NALUnit> pps,
                      const SliceHeader &header,
                      BinaryReader &br) {
    if (pps->entropy_coding_mode_flag())
        br.reset_bit();
    bool mbaff_frame_flag = this->mbaff_frame_flag(sps, header);
    uint64_t curr_mb_addr = header.first_mb_in_slice * (1 + mbaff_frame_flag);
    bool more_data_flag = true;
    bool prev_mb_skipped = false;
    do {
        bool mb_skip_flag;
        uint64_t mb_skip_run;
        if (header.slice_type != SliceType::TYPE_I
            && header.slice_type != SliceType::TYPE_SI) {
            if (pps->entropy_coding_mode_flag()) {
                mb_skip_run = br.read_ue();
                prev_mb_skipped = mb_skip_run > 0;
                for (uint64_t i = 0; i < mb_skip_run; i++ )
                    curr_mb_addr = next_mb_addr(curr_mb_addr, sps, pps, header);
                if (mb_skip_run > 0)
                    more_data_flag = more_rbsp_data(br);
            } else {
                mb_skip_flag = br.read_bit_as_bool();
                more_data_flag = !mb_skip_flag;
            }
        }
        if (more_data_flag) {
            bool mb_field_decoding_flag = true;
            if (mbaff_frame_flag && (curr_mb_addr % 2 == 0
                                     || (curr_mb_addr % 2 == 1
                                         && prev_mb_skipped)))
                mb_field_decoding_flag = br.read_bit_as_bool();
            MacroBlock block(mb_field_decoding_flag);
        }
        if (!pps->entropy_coding_mode_flag()) {
            more_data_flag = !br.eof();

        } else {
            throw NotImplemented("entropy_coding_mode_flag");
            if (header.slice_type != SliceType::TYPE_I
                && header.slice_type != SliceType::TYPE_SI) {
                prev_mb_skipped = mb_skip_flag;
                if (mbaff_frame_flag && curr_mb_addr % 2 == 0) {
                    more_data_flag = true;
                } else {
                    /* TODO: not sure if that ae(v) is correct here */
                    bool end_of_slice_flag = br.read_bit_as_bool();
                    more_data_flag = !end_of_slice_flag;
                }
            }
        }
        curr_mb_addr = next_mb_addr(curr_mb_addr, sps, pps, header);
    } while (more_data_flag);
}

uint64_t SliceData::next_mb_addr(uint64_t n, std::shared_ptr<SPS_NALUnit> sps,
                                 std::shared_ptr<PPS_NALUnit> pps,
                                 const SliceHeader & header) {
    /* Based on eqn 7-24 -> 7-28 and 8-16 */
    uint64_t i = n + 1;
    /* TODO: need refactoring here */
    uint64_t PicHeightInMapUnits = sps->pic_height_in_map_units_minus1() + 1;
    uint64_t PicWidthInMbs = sps->pic_width_in_mbs_minus1() + 1;
    uint64_t FrameHeightInMbs = (2 - sps->frame_mbs_only_flag()) * PicHeightInMapUnits;
    uint64_t PicHeightInMbs = FrameHeightInMbs / ( 1 + header.field_pic_flag);
    uint64_t PicSizeInMbs = PicWidthInMbs * PicHeightInMbs;
    std::vector<uint64_t> MbToSliceGroupMap = slice_group_map(sps, pps);
    while( i < PicSizeInMbs && MbToSliceGroupMap[i] != MbToSliceGroupMap[n])
        i++;
    return i;
}


std::vector<uint64_t> SliceData::slice_group_map(
        std::shared_ptr<SPS_NALUnit> sps, std::shared_ptr<PPS_NALUnit> pps) {
    std::vector<uint64_t> mapUnitToSliceGroupMap(16 * 16);
    uint64_t PicHeightInMapUnits = sps->pic_height_in_map_units_minus1() + 1;
    uint64_t PicWidthInMbs = sps->pic_width_in_mbs_minus1() + 1;
    uint64_t PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
    uint64_t i = 0;

    uint64_t num_slice_groups = pps->num_slice_groups_minus1() + 1;

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
        for( i = 0; i < PicSizeInMapUnits; i++ )
            mapUnitToSliceGroupMap[i] = num_slice_groups - 1;
        for (int iGroup = (int)num_slice_groups - 2; iGroup >= 0; iGroup--) {
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

void MacroBlock::parse(std::shared_ptr<SPS_NALUnit> sps,
                       std::shared_ptr<PPS_NALUnit> pps, SliceHeader &header,
                       BinaryReader &br) {
    mb_type = br.read_ue();
    if (mb_type == I_PCM) {
        br.reset_bit(true);
        uint64_t bit_depth_luma = 8 + sps->bit_depth_luma_minus8();
        /* PCM luma */
        for (int i = 0; i < 256; i++) {
            br.read_bits(bit_depth_luma);
        }
        uint32_t SubWidthC = 0;
        uint32_t SubHeightC = 0;
        if (sps->chroma_format_idc() == 1 || sps->chroma_format_idc() == 2)
            SubWidthC = 2;
        else if (sps->chroma_format_idc() == 3 &&
                 sps->separate_colour_plane_flag())
            SubWidthC = 1;
        else
            throw std::runtime_error("unsupported SubWidthC");
        if (sps->chroma_format_idc() == 1)
            SubHeightC = 2;
        else if (sps->chroma_format_idc() == 2)
            SubHeightC = 1;
        else if (sps->chroma_format_idc() == 3 &&
                 !sps->separate_colour_plane_flag())
            SubHeightC = 1;
        else
            throw std::runtime_error("unsupported SubHeightC");

        uint32_t MbWidthC = 16 / SubWidthC;
        uint32_t MbHeightC = 16 / SubHeightC;
        uint64_t bit_depth_chroma = 8 + sps->bit_depth_chroma_minus8();
        /* PCM chroma */
        for (uint32_t i = 0; i < 2 * MbWidthC * MbHeightC; i++)
            br.read_bits(bit_depth_chroma);
    } else {
        bool noSubMbPartSizeLessThan8x8Flag = true;
        if (MbPartPredMode(mb_type, 0, header.slice_type) != Intra_4x4
            && MbPartPredMode(mb_type, 0, header.slice_type) != Intra_16x16
            && NumMbPart(mb_type) == 4) {
            auto sub_mb_pred = std::make_shared<SubMbPred>();
            sub_mb_pred->parse(sps, pps, header, *this, br);
        } else {
            if (pps->transform_8x8_mode_flag() && mb_type == I_NxN) {
                /* Note: this is only true for !entropy */
                if (pps->entropy_coding_mode_flag())
                    throw NotImplemented("entropy_coding_mode not implemented");
                transform_size_8x8_flag = br.read_bit_as_bool();
            }
            // mb_pred
            auto mb_pred = std::make_shared<MbPred>();
            mb_pred->parse(sps, pps, header, br, *this);
            mb_preds.emplace_back(mb_pred);
        }
        uint64_t CodedBlockPatternLuma = 0;
        uint64_t CodedBlockPatternChroma = 0;
        if (MbPartPredMode(mb_type, 0, header.slice_type) != Intra_16x16) {
            uint64_t coded_block_pattern = br.read_ue();
            if (coded_block_pattern > 47)
                throw std::runtime_error("incorrect coded block pattern");
            coded_block_pattern =
                    (uint64_t)codeNum_to_coded_block_pattern_intra[
                            coded_block_pattern];
            CodedBlockPatternLuma = coded_block_pattern % 16;
            CodedBlockPatternChroma = coded_block_pattern / 16;

            /* TODO: clea this up */
            uint64_t B_Direct_16x16 = 0;

            if (CodedBlockPatternLuma > 0 &&
                pps->transform_8x8_mode_flag() && mb_type != I_NxN &&
                noSubMbPartSizeLessThan8x8Flag &&
                (mb_type != B_Direct_16x16
                 || sps->direct_8x8_inference_flag())) {
                transform_size_8x8_flag = br.read_bit_as_bool();
                throw NotImplemented("transform_size_8x8_flag");
            }
        }
        if (CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 ||
            MbPartPredMode(mb_type, 0, header.slice_type) == Intra_16x16) {
            mb_qp_delta = br.read_se();
            /* residual here */
        }

    }
}

MbPred::MbPred() {
    for (int i = 0; i < 16; i++) {
        prev_intra4x4_pred_mode_flag[i] = false;
        rem_intra4x4_pred_mode[i] = 0;
    }
}

void MbPred::parse(std::shared_ptr<SPS_NALUnit> sps,
                   std::shared_ptr<PPS_NALUnit>   , SliceHeader &header,
                   BinaryReader &br, MacroBlock &mb) {
    uint32_t type = (uint32_t) MbPartPredMode(mb.mb_type, 0, header.slice_type);
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
    } else {
        uint64_t num_mb_part = NumMbPart(mb.mb_type);
        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) {
            if ((header.num_ref_idx_l0_active_minus1 > 0 ||
                  mb.mb_field_decoding_flag != header.field_pic_flag ) &&
                MbPartPredMode(mb.mb_type, mbPartIdx, header.slice_type) != Pred_L1)
            ref_idx_l0[mbPartIdx] = br.read_te();
        }

        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++)
            if ((header.num_ref_idx_l0_active_minus1 > 0 ||
                  mb.mb_field_decoding_flag != header.field_pic_flag ) &&
                MbPartPredMode(mb.mb_type, mbPartIdx, header.slice_type) != Pred_L0)
                ref_idx_l1[mbPartIdx] = br.read_te();

        for (uint32_t mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++)
            if (MbPartPredMode(mb.mb_type, mbPartIdx, header.slice_type) != Pred_L1 )
                for (int compIdx = 0; compIdx < 2; compIdx++ )
                    mvd_l0[mbPartIdx][0][compIdx] = br.read_se();

        for (uint32_t mbPartIdx = 0; mbPartIdx < NumMbPart(mb.mb_type); mbPartIdx++)
            if (MbPartPredMode(mb.mb_type, mbPartIdx, header.slice_type) != Pred_L0 )
                for (int compIdx = 0; compIdx < 2; compIdx++ )
                    mvd_l1[mbPartIdx][0][compIdx] = br.read_se();
    }
}


void SubMbPred::parse(std::shared_ptr<SPS_NALUnit>,
                      std::shared_ptr<PPS_NALUnit>, SliceHeader &header,
                      MacroBlock &mb, BinaryReader &br) {
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        sub_mb_type[mbPartIdx] = br.read_ue();
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if ((header.num_ref_idx_l0_active_minus1 > 0 ||
             mb.mb_field_decoding_flag != header.field_pic_flag) &&
            mb.mb_type != P_8x8ref0 &&
            sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header.slice_type)
            != Pred_L1) {
            ref_idx_l0[mbPartIdx] = br.read_te();
        }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if ((header.num_ref_idx_l1_active_minus1 > 0 ||
             mb.mb_field_decoding_flag != header.field_pic_flag) &&
            mb.mb_type != P_8x8ref0 &&
            sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header.slice_type)
            != Pred_L0) {
            ref_idx_l1[mbPartIdx] = br.read_te();
        }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header.slice_type) != Pred_L1)
            for (uint32_t subMbPartIdx = 0; subMbPartIdx <
                                       NumSubMbPart(sub_mb_type[mbPartIdx],
                                                    header.slice_type);
                 subMbPartIdx++) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l0[mbPartIdx][subMbPartIdx][compIdx] = br.read_se();
            }
    }
    for (int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++) {
        if (sub_mb_type[mbPartIdx] != B_Direct_8x8 &&
            SubMbPredMode(sub_mb_type[mbPartIdx], header.slice_type) != Pred_L0)
            for (uint32_t subMbPartIdx = 0; subMbPartIdx <
                                       NumSubMbPart(sub_mb_type[mbPartIdx],
                                                    header.slice_type);
                 subMbPartIdx++) {
                for (int compIdx = 0; compIdx < 2; compIdx++)
                    mvd_l1[mbPartIdx][subMbPartIdx][compIdx] = br.read_se();
            }
    }
}