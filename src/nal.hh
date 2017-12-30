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

#ifndef H264FLOW_NAL_HH
#define H264FLOW_NAL_HH

#include "io.hh"
#include <string>
#include <memory>
#include <vector>

class MacroBlock;

enum class SliceType {
    TYPE_P = 0,
    TYPE_B = 1,
    TYPE_I = 2,
    TYPE_EP = 0,
    TYPE_EB = 1,
    TYPE_EI = 2,
    TYPE_SP = 3,
    TYPE_SI = 4,
};

inline bool operator==(uint64_t lhs, SliceType rhs) {
    uint64_t value = lhs % 5;
    return value == (uint64_t)rhs;
}

inline bool operator==(SliceType lhs, uint64_t rhs) {
    uint64_t value = rhs % 5;
    return value == (uint64_t)lhs;
}

inline bool operator!=(uint64_t lhs, SliceType rhs) {
    uint64_t value = lhs % 5;
    return value != (uint64_t)rhs;
}

inline bool operator!=(SliceType lhs, uint64_t rhs) {
    uint64_t value = rhs % 5;
    return value != (uint64_t)lhs;
}

class NALUnit {
public:
    explicit NALUnit(std::string data);
    NALUnit(BinaryReader & br, uint32_t size) : NALUnit(br, size, true) {}
    NALUnit(BinaryReader & br, uint32_t size, bool unescape);
    NALUnit(NALUnit & unit): _nal_ref_idc(unit._nal_ref_idc),
                                      _nal_unit_type(unit._nal_unit_type),
                                      _data(unit._data) { parse(); }

    uint8_t nal_ref_idc() const { return _nal_ref_idc; }
    uint8_t  nal_unit_type() const { return _nal_unit_type; }
    uint32_t size() { return static_cast<uint32_t>(_data.length() + 1); }

    bool idr_pic_flag() const { return _nal_unit_type == 5; }

    virtual ~NALUnit() = default;

protected:
    uint8_t _nal_ref_idc;
    uint8_t _nal_unit_type;
    std::string _data;

    virtual void parse() {}

private:
    void decode_header(BinaryReader & br);

};

class SPS_NALUnit : public NALUnit {
public:
    explicit SPS_NALUnit(NALUnit & unit);
    explicit SPS_NALUnit(std::string data);

    uint8_t profile_idc() const { return _profile_idc; }
    uint8_t flags() const { return _flags; }
    uint8_t level_idc() const { return _level_idc; }
    uint64_t sps_id() const { return _sps_id; }
    uint64_t chroma_format_idc() const { return _chroma_format_idc; }
    bool separate_colour_plane_flag() const
    { return _separate_colour_plane_flag; }
    uint64_t bit_depth_luma_minus8() const { return _bit_depth_luma_minus8; }
    uint64_t bit_depth_chroma_minus8() const
    { return _bit_depth_chroma_minus8; }
    bool qpprime_y_zero_transform_bypass_flag() const
    { return _qpprime_y_zero_transform_bypass_flag; }
    bool seq_scaling_matrix_present_flag() const
    { return _seq_scaling_matrix_present_flag; }
    uint64_t log2_max_frame_num_minus4() const
    { return _log2_max_frame_num_minus4; }
    uint64_t pic_order_cnt_type() const { return _pic_order_cnt_type; }
    uint64_t log2_max_pic_order_cnt_lsb_minus4() const
    { return _log2_max_pic_order_cnt_lsb_minus4; }
    bool delta_pic_order_always_zero_flag() const
    { return _delta_pic_order_always_zero_flag; }
    int64_t offset_for_non_ref_pic() { return _offset_for_non_ref_pic; }
    int64_t offset_for_top_to_bottom_field() const
    { return _offset_for_top_to_bottom_field; }
    uint64_t num_ref_frames_in_pic_order_cnt_cycle() const
    { return _num_ref_frames_in_pic_order_cnt_cycle; }
    std::vector<uint64_t> offset_for_ref_frame() const
    { return _offset_for_ref_frame; }
    uint64_t max_num_ref_frames() const { return _max_num_ref_frames; }
    bool gaps_in_frame_num_value_allowed_flag() const
    { return _gaps_in_frame_num_value_allowed_flag; }
    uint64_t pic_width_in_mbs_minus1() const
    { return _pic_width_in_mbs_minus1; }
    uint64_t pic_height_in_map_units_minus1() const
    { return _pic_height_in_map_units_minus1; }
    bool frame_mbs_only_flag() const { return _frame_mbs_only_flag; }
    bool mb_adaptive_frame_field_flag() const
    { return _mb_adaptive_frame_field_flag; }
    bool direct_8x8_inference_flag() const
    { return _direct_8x8_inference_flag; }
    bool frame_cropping_flag() const { return _frame_cropping_flag; }
    uint64_t frame_crop_left_offset() const { return _frame_crop_left_offset; }
    uint64_t frame_crop_right_offset() const
    { return _frame_crop_right_offset; }
    uint64_t frame_crop_top_offset() const { return _frame_crop_top_offset; }
    uint64_t frame_crop_bottom_offset() const
    { return _frame_crop_bottom_offset; }
    bool vui_parameters_present_flag() const
    { return _vui_parameters_present_flag; }

    uint64_t chroma_array_type() const {
        return _separate_colour_plane_flag ? 0 : _chroma_format_idc;
    }

protected:
    void parse() override;

private:
    uint8_t _profile_idc = 0;
    uint8_t _flags = 0;
    uint8_t _level_idc = 0;
    uint64_t _sps_id = 0;
    uint64_t _chroma_format_idc = 0;
    bool _separate_colour_plane_flag = false;
    uint64_t _bit_depth_luma_minus8 = 0;
    uint64_t _bit_depth_chroma_minus8 = 0;
    bool _qpprime_y_zero_transform_bypass_flag = false;
    bool _seq_scaling_matrix_present_flag = false;
    uint64_t _log2_max_frame_num_minus4 = 0;
    uint64_t _pic_order_cnt_type = 0;
    uint64_t _log2_max_pic_order_cnt_lsb_minus4 = 0;
    bool _delta_pic_order_always_zero_flag = false;
    int64_t _offset_for_non_ref_pic = 0;
    int64_t _offset_for_top_to_bottom_field = 0;
    uint64_t _num_ref_frames_in_pic_order_cnt_cycle = 0;
    std::vector<uint64_t> _offset_for_ref_frame;
    uint64_t _max_num_ref_frames = 0;
    bool _gaps_in_frame_num_value_allowed_flag = false;
    uint64_t _pic_width_in_mbs_minus1 = 0;
    uint64_t _pic_height_in_map_units_minus1 = 0;
    bool _frame_mbs_only_flag = false;
    bool _mb_adaptive_frame_field_flag = false;
    bool _direct_8x8_inference_flag = false;
    bool _frame_cropping_flag = false;
    uint64_t _frame_crop_left_offset = 0;
    uint64_t _frame_crop_right_offset = 0;
    uint64_t _frame_crop_top_offset = 0;
    uint64_t _frame_crop_bottom_offset = 0;
    bool _vui_parameters_present_flag = false;
};


class PPS_NALUnit : public NALUnit {
public:
    explicit PPS_NALUnit(NALUnit & unit);
    explicit PPS_NALUnit(std::string data);

    uint64_t pps_id() const { return _pps_id; }
    uint64_t sps_id() const { return _sps_id; }
    bool entropy_coding_mode_flag() const { return _entropy_coding_mode_flag; }
    bool bottom_field_pic_order_in_frame_present_flag() const
    { return _bottom_field_pic_order_in_frame_present_flag; }
    uint64_t num_slice_groups_minus1() const
    { return _num_slice_groups_minus1; }
    uint64_t slice_group_map_type() const { return _slice_group_map_type; }
    std::vector<uint64_t> run_length_minus1() const
    { return _run_length_minus1; }
    std::vector<uint64_t> top_left() const { return _top_left; }
    std::vector<uint64_t> bottom_right() const { return _bottom_right; }
    bool slice_group_change_direction_flag() const
    { return _slice_group_change_direction_flag; }
    uint64_t slice_group_change_rate_minus1() const
    { return _slice_group_change_rate_minus1; }
    uint64_t pic_size_in_map_units_minus1() const
    { return _pic_size_in_map_units_minus1; }
    std::vector<uint64_t> slice_group_id() const { return _slice_group_id; }
    uint64_t num_ref_idx_l0_default_active_minus1() const
    { return _num_ref_idx_l0_default_active_minus1; }
    uint64_t num_ref_idx_l1_default_active_minus1() const
    { return _num_ref_idx_l1_default_active_minus1; }
    bool weighted_pred_flag() const { return _weighted_pred_flag; }
    uint64_t weighted_bipred_idc() const { return _weighted_bipred_idc; }
    int64_t pic_init_qp_minus26() const { return _pic_init_qp_minus26;}
    int64_t pic_init_qs_minus26() const { return _pic_init_qs_minus26; }
    int64_t chroma_qp_index_offset() const { return _chroma_qp_index_offset; }
    bool deblocking_filter_control_present_flag() const
    {return _deblocking_filter_control_present_flag; }
    bool constrained_intra_pred_flag() const
    { return _constrained_intra_pred_flag; }
    bool redundant_pic_cnt_present_flag() const
    {return _redundant_pic_cnt_present_flag; }
    bool transform_8x8_mode_flag() const { return _transform_8x8_mode_flag; }
    bool pic_scaling_matrix_present_flag() const
    { return _pic_scaling_matrix_present_flag; }

protected:
    void parse() override;

private:
    uint64_t _pps_id = 0;
    uint64_t _sps_id = 0;
    bool _entropy_coding_mode_flag = false;
    bool _bottom_field_pic_order_in_frame_present_flag = false;
    uint64_t _num_slice_groups_minus1 = 0;
    uint64_t _slice_group_map_type = 0;
    std::vector<uint64_t> _run_length_minus1;
    std::vector<uint64_t> _top_left;
    std::vector<uint64_t> _bottom_right;
    bool _slice_group_change_direction_flag = false;
    uint64_t _slice_group_change_rate_minus1 = 0;
    uint64_t _pic_size_in_map_units_minus1 = 0;
    std::vector<uint64_t> _slice_group_id;
    uint64_t _num_ref_idx_l0_default_active_minus1 = 0;
    uint64_t _num_ref_idx_l1_default_active_minus1 = 0;
    bool _weighted_pred_flag = false;
    uint64_t _weighted_bipred_idc = 0;
    int64_t _pic_init_qp_minus26 = 0;
    int64_t _pic_init_qs_minus26 = 0;
    int64_t _chroma_qp_index_offset = 0;
    bool _deblocking_filter_control_present_flag = false;
    bool _constrained_intra_pred_flag = false;
    bool _redundant_pic_cnt_present_flag = false;
    bool _transform_8x8_mode_flag = false;
    bool _pic_scaling_matrix_present_flag = false;
};

class RefPicListModification {
public:
    /* TODO: refactor this after decoding is finished */
    RefPicListModification();
    RefPicListModification(uint64_t slice_type, BinaryReader & br);
    bool ref_pic_list_modification_flag_l0 = false;
    bool ref_pic_list_modification_flag_l1 = false;
    std::vector<uint64_t> modification_of_pic_nums_idc;
    std::vector<uint64_t> abs_diff_pic_num_minus1;
    std::vector<uint64_t> long_term_pic_num;
    std::vector<uint64_t> abs_diff_view_idx_minus1;
};

class PredWeightTable {
public:
    /* TODO: refactor this after decoding is finished */
    PredWeightTable();
    PredWeightTable(std::shared_ptr<SPS_NALUnit> sps,
                    std::shared_ptr<PPS_NALUnit> pps,
                    uint64_t slice_type,
                    BinaryReader & br);
    uint64_t luma_log2_weight_denom = 0;
    uint64_t chroma_log2_weight_denom = 0;
    std::vector<int64_t> luma_weight_l0;
    std::vector<int64_t> luma_offset_l0;
    std::vector<int64_t> luma_weight_l1;
    std::vector<int64_t> luma_offset_l1;
    std::vector<std::array<int64_t, 2>> chroma_weight_l0;
    std::vector<std::array<int64_t, 2>> chroma_offset_l0;
    std::vector<std::array<int64_t, 2>>chroma_weight_l1;
    std::vector<std::array<int64_t, 2>> chroma_offset_l1;
};

class DecRefPicMarking {
public:
    DecRefPicMarking();
    DecRefPicMarking(const NALUnit &unit, BinaryReader &br);
    bool no_output_of_prior_pics_flag = false;
    bool long_term_reference_flag = false;
    bool adaptive_ref_pic_marking_mode_flag = false;
    std::vector<uint64_t> memory_management_control_operation;
    std::vector<uint64_t> difference_of_pic_nums_minus1;
    std::vector<uint64_t> long_term_pic_num;
    std::vector<uint64_t> long_term_frame_idx;
    std::vector<uint64_t> max_long_term_frame_idx_plus1;
};

class SliceHeader {
public:
    // SliceHeader();
    SliceHeader(const NALUnit & nal, std::string & data): _data(data),
                                                          _nal(nal), rplm(),
                                                          pwt(), drpm()
    {}
    uint32_t header_size() const { return _header_size; }

    void parse(std::shared_ptr<SPS_NALUnit> sps,
               std::shared_ptr<PPS_NALUnit> pps);
private:
    uint32_t _header_size = 0;
    std::string _data;
    const NALUnit & _nal;

public:
    uint64_t first_mb_in_slice = 0;
    uint64_t slice_type = 0;
    uint64_t pps_id = 0;
    uint8_t colour_plane_id = 0;
    uint64_t frame_num = 0;
    bool field_pic_flag = false;
    bool bottom_field_flag = false;
    uint64_t idr_pic_id = 0;
    uint64_t pic_order_cnt_lsb = 0;
    int64_t delta_pic_order_cnt_bottom = 0;
    int64_t delta_pic_order_cnt[2] = {0, 0};
    uint64_t redundant_pic_cnt = 0;
    bool direct_spatial_mv_pred_flag = false;
    bool num_ref_idx_active_override_flag = false;
    uint64_t num_ref_idx_l0_active_minus1 = 0;
    uint64_t num_ref_idx_l1_active_minus1 = 0;
    uint64_t cabac_init_idc = 0;
    int64_t slice_qp_delta = 0;
    bool sp_for_switch_flag = false;
    int64_t slice_qs_delta = 0;
    uint64_t disable_deblocking_filter_idc = 0;
    int64_t slice_alpha_c0_offset_div2 = 0;
    int64_t slice_beta_offset_div2 = 0;
    uint64_t slice_group_change_cycle = 0;

    RefPicListModification rplm;
    PredWeightTable pwt;
    DecRefPicMarking drpm;
};


class SliceData {
public:
    explicit SliceData(const NALUnit & nal) : _nal(nal) {}
    void parse(std::shared_ptr<SPS_NALUnit> sps,
               std::shared_ptr<PPS_NALUnit> pps, const SliceHeader & header,
               BinaryReader & br);
private:
    const NALUnit & _nal;

    bool mbaff_frame_flag(std::shared_ptr<SPS_NALUnit> sps,
                          const SliceHeader & header) {
        return  sps->mb_adaptive_frame_field_flag() && !header.field_pic_flag;
    }

    uint64_t next_mb_addr(uint64_t n, std::shared_ptr<SPS_NALUnit> sps,
                          std::shared_ptr<PPS_NALUnit> pps,
                          const SliceHeader & header);

    std::vector<uint64_t> slice_group_map(std::shared_ptr<SPS_NALUnit> sps,
                                          std::shared_ptr<PPS_NALUnit> pps);

    bool more_rbsp_data(BinaryReader & br) { return !br.eof(); }
};


class MbPred {
public:
    MbPred();

    void parse(std::shared_ptr<SPS_NALUnit> sps,
               std::shared_ptr<PPS_NALUnit> pps,
               SliceHeader & header, BinaryReader &br, MacroBlock & mb);

    bool prev_intra4x4_pred_mode_flag[16];
    uint8_t rem_intra4x4_pred_mode[16];
    uint64_t ref_idx_l0[4];
    uint64_t ref_idx_l1[4];
    int64_t mvd_l0[4][1][2];
    int64_t mvd_l1[4][1][2];
    uint64_t intra_chroma_pred_mode = 0;
};


class SubMbPred {
public:
    SubMbPred() = default;
    void parse(std::shared_ptr<SPS_NALUnit> sps,
              std::shared_ptr<PPS_NALUnit> pps,
              SliceHeader &header,
              MacroBlock &mb,
              BinaryReader &br);
    uint64_t sub_mb_type[4];
    uint64_t ref_idx_l0[4];
    uint64_t ref_idx_l1[4];
    int64_t mvd_l0[4][4][2];
    int64_t mvd_l1[4][4][2];
};

class Residual {
public:

};

class MacroBlock {
public:
    MacroBlock(bool mb_field_decoding_flag) : mb_preds(),
                                              sub_mb_preds(),
                                              mb_field_decoding_flag(
                                                      mb_field_decoding_flag) {}
    void parse(std::shared_ptr<SPS_NALUnit> sps,
               std::shared_ptr<PPS_NALUnit> pps,
               SliceHeader & header, BinaryReader &br);
    uint64_t mb_type = 0;
    bool transform_size_8x8_flag = false;

    std::vector<std::shared_ptr<MbPred>> mb_preds;
    std::vector<std::shared_ptr<SubMbPred>> sub_mb_preds;
    bool mb_field_decoding_flag;
    int64_t mb_qp_delta = 0;

};

class Slice_NALUnit : public NALUnit {
public:
    explicit Slice_NALUnit(std::string data);
    explicit Slice_NALUnit(NALUnit & unit);
    explicit Slice_NALUnit(std::shared_ptr<NALUnit> & unit) :
            Slice_NALUnit(*unit.get()) {}

    void parse(std::shared_ptr<SPS_NALUnit> sps,
               std::shared_ptr<PPS_NALUnit> pps);

    SliceHeader header() { return _header; }
private:
    SliceHeader _header;
    SliceData _slice_data;
};

#endif //H264FLOW_NAL_HH
