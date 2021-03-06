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


#include <string>
#include <memory>
#include <vector>
#include <cmath>
#include <utility>
#include "io.hh"
#include "consts.hh"

class MacroBlock;
class ParserContext;
class Slice_NALUnit;

/* taken from https://github.com/emericg/MiniVideo/ */
enum class BlockType {
    blk_LUMA_8x8      = 0,
    blk_LUMA_4x4      = 1,
    blk_LUMA_16x16_DC = 2,
    blk_LUMA_16x16_AC = 3,
    blk_CHROMA_DC_Cb  = 4,
    blk_CHROMA_DC_Cr  = 5,
    blk_CHROMA_AC_Cb  = 6,
    blk_CHROMA_AC_Cr  = 7,

    blk_UNKNOWN       = 999
};


inline BlockType operator+(BlockType block, int i) {
    auto result = static_cast<int>(block) + i;
    return static_cast<BlockType>(result);
}

class NALUnit {
public:
    explicit NALUnit(std::string data, bool unescape = true);
    NALUnit(BinaryReader & br, uint32_t size) : NALUnit(br, size, true) {}
    NALUnit(BinaryReader & br, uint32_t size, bool unescape);
    NALUnit(NALUnit & unit): _nal_ref_idc(unit._nal_ref_idc),
                                      _nal_unit_type(unit._nal_unit_type),
                                      _data(unit._data) { parse(); }

    uint8_t nal_ref_idc() const { return _nal_ref_idc; }
    uint8_t  nal_unit_type() const { return _nal_unit_type; }
    uint32_t size() { return static_cast<uint32_t>(_data.length() + 1); }

    bool idr_pic_flag() const { return _nal_unit_type == 5; }
    /* TODO: fix this */
    const std::string data() const { return _data; }

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

    uint8_t profile_idc() const { return profile_idc_; }
    uint8_t flags() const { return flags_; }
    uint8_t level_idc() const { return level_idc_; }
    uint64_t sps_id() const { return sps_id_; }
    uint64_t chroma_format_idc() const { return chroma_format_idc_; }
    bool separate_colour_plane_flag() const
    { return separate_colour_plane_flag_; }
    uint64_t bit_depth_luma_minus8() const { return bit_depth_luma_minus8_; }
    uint64_t bit_depth_chroma_minus8() const
    { return bit_depth_chroma_minus8_; }
    bool qpprime_y_zero_transform_bypass_flag() const
    { return qpprime_y_zero_transform_bypass_flag_; }
    bool seq_scaling_matrix_present_flag() const
    { return seq_scaling_matrix_present_flag_; }
    uint64_t log2_max_frame_num_minus4() const
    { return log2_max_frame_num_minus4_; }
    uint64_t pic_order_cnt_type() const { return pic_order_cnt_type_; }
    uint64_t log2_max_pic_order_cnt_lsb_minus4() const
    { return log2_max_pic_order_cnt_lsb_minus4_; }
    bool delta_pic_order_always_zero_flag() const
    { return delta_pic_order_always_zero_flag_; }
    int64_t offset_for_non_ref_pic() { return offset_for_non_ref_pic_; }
    int64_t offset_for_top_to_bottom_field() const
    { return offset_for_top_to_bottom_field_; }
    uint64_t num_ref_frames_in_pic_order_cnt_cycle() const
    { return num_ref_frames_in_pic_order_cnt_cycle_; }
    std::vector<uint64_t> offset_for_ref_frame() const
    { return offset_for_ref_frame_; }
    uint64_t max_num_ref_frames() const { return max_num_ref_frames_; }
    bool gaps_in_frame_num_value_allowed_flag() const
    { return gaps_in_frame_num_value_allowed_flag_; }
    uint64_t pic_width_in_mbs_minus1() const
    { return pic_width_in_mbs_minus1_; }
    uint64_t pic_height_in_map_units_minus1() const
    { return pic_height_in_map_units_minus1_; }
    bool frame_mbs_only_flag() const { return frame_mbs_only_flag_; }
    bool mb_adaptive_frame_field_flag() const
    { return mb_adaptive_frame_field_flag_; }
    bool direct_8x8_inference_flag() const
    { return direct_8x8_inference_flag_; }
    bool frame_cropping_flag() const { return frame_cropping_flag_; }
    uint64_t frame_crop_left_offset() const { return frame_crop_left_offset_; }
    uint64_t frame_crop_right_offset() const
    { return frame_crop_right_offset_; }
    uint64_t frame_crop_top_offset() const { return frame_crop_top_offset_; }
    uint64_t frame_crop_bottom_offset() const
    { return frame_crop_bottom_offset_; }
    bool vui_parameters_present_flag() const
    { return vui_parameters_present_flag_; }

    uint64_t chroma_array_type() const {
        return separate_colour_plane_flag_ ? 0 : chroma_format_idc_;
    }

protected:
    void parse() override;

private:
    uint8_t profile_idc_ = 0;
    uint8_t flags_ = 0;
    uint8_t level_idc_ = 0;
    uint64_t sps_id_ = 0;
    /* chroma_format_idc is not present, it shall be
     * inferred to be equal to 1 (4:2:0 chroma format). */
    uint64_t chroma_format_idc_ = 1;
    bool separate_colour_plane_flag_ = false;
    uint64_t bit_depth_luma_minus8_ = 0;
    uint64_t bit_depth_chroma_minus8_ = 0;
    bool qpprime_y_zero_transform_bypass_flag_ = false;
    bool seq_scaling_matrix_present_flag_ = false;
    uint64_t log2_max_frame_num_minus4_ = 0;
    uint64_t pic_order_cnt_type_ = 0;
    uint64_t log2_max_pic_order_cnt_lsb_minus4_ = 0;
    bool delta_pic_order_always_zero_flag_ = false;
    int64_t offset_for_non_ref_pic_ = 0;
    int64_t offset_for_top_to_bottom_field_ = 0;
    uint64_t num_ref_frames_in_pic_order_cnt_cycle_ = 0;
    std::vector<uint64_t> offset_for_ref_frame_;
    uint64_t max_num_ref_frames_ = 0;
    bool gaps_in_frame_num_value_allowed_flag_ = false;
    uint64_t pic_width_in_mbs_minus1_ = 0;
    uint64_t pic_height_in_map_units_minus1_ = 0;
    bool frame_mbs_only_flag_ = false;
    bool mb_adaptive_frame_field_flag_ = false;
    bool direct_8x8_inference_flag_ = false;
    bool frame_cropping_flag_ = false;
    uint64_t frame_crop_left_offset_ = 0;
    uint64_t frame_crop_right_offset_ = 0;
    uint64_t frame_crop_top_offset_ = 0;
    uint64_t frame_crop_bottom_offset_ = 0;
    bool vui_parameters_present_flag_ = false;
};


class PPS_NALUnit : public NALUnit {
public:
    explicit PPS_NALUnit(NALUnit & unit);
    explicit PPS_NALUnit(std::string data);

    uint64_t pps_id() const { return pps_id_; }
    uint64_t sps_id() const { return sps_id_; }
    bool entropy_coding_mode_flag() const { return entropy_coding_mode_flag_; }
    bool bottom_field_pic_order_in_frame_present_flag() const
    { return bottom_field_pic_order_in_frame_present_flag_; }
    uint64_t num_slice_groups_minus1() const
    { return num_slice_groups_minus1_; }
    uint64_t slice_group_map_type() const { return slice_group_map_type_; }
    std::vector<uint64_t> run_length_minus1() const
    { return run_length_minus1_; }
    std::vector<uint64_t> top_left() const { return top_left_; }
    std::vector<uint64_t> bottom_right() const { return bottom_right_; }
    bool slice_group_change_direction_flag() const
    { return slice_group_change_direction_flag_; }
    uint64_t slice_group_change_rate_minus1() const
    { return slice_group_change_rate_minus1_; }
    uint64_t pic_size_in_map_units_minus1() const
    { return pic_size_in_map_units_minus1_; }
    std::vector<uint64_t> slice_group_id() const { return slice_group_id_; }
    uint64_t num_ref_idx_l0_default_active_minus1() const
    { return num_ref_idx_l0_default_active_minus1_; }
    uint64_t num_ref_idx_l1_default_active_minus1() const
    { return num_ref_idx_l1_default_active_minus1_; }
    bool weighted_pred_flag() const { return weighted_pred_flag_; }
    uint64_t weighted_bipred_idc() const { return weighted_bipred_idc_; }
    int64_t pic_init_qp_minus26() const { return pic_init_qp_minus26_;}
    int64_t pic_init_qs_minus26() const { return pic_init_qs_minus26_; }
    int64_t chroma_qp_index_offset() const { return chroma_qp_index_offset_; }
    bool deblocking_filter_control_present_flag() const
    {return deblocking_filter_control_present_flag_; }
    bool constrained_intra_pred_flag() const
    { return constrained_intra_pred_flag_; }
    bool redundant_pic_cnt_present_flag() const
    {return redundant_pic_cnt_present_flag_; }
    bool transform_8x8_mode_flag() const { return transform_8x8_mode_flag_; }
    bool pic_scaling_matrix_present_flag() const
    { return pic_scaling_matrix_present_flag_; }

protected:
    void parse() override;

private:
    uint64_t pps_id_ = 0;
    uint64_t sps_id_ = 0;
    bool entropy_coding_mode_flag_ = false;
    bool bottom_field_pic_order_in_frame_present_flag_ = false;
    uint64_t num_slice_groups_minus1_ = 0;
    uint64_t slice_group_map_type_ = 0;
    std::vector<uint64_t> run_length_minus1_;
    std::vector<uint64_t> top_left_;
    std::vector<uint64_t> bottom_right_;
    bool slice_group_change_direction_flag_ = false;
    uint64_t slice_group_change_rate_minus1_ = 0;
    uint64_t pic_size_in_map_units_minus1_ = 0;
    std::vector<uint64_t> slice_group_id_;
    uint64_t num_ref_idx_l0_default_active_minus1_ = 0;
    uint64_t num_ref_idx_l1_default_active_minus1_ = 0;
    bool weighted_pred_flag_ = false;
    uint64_t weighted_bipred_idc_ = 0;
    int64_t pic_init_qp_minus26_ = 0;
    int64_t pic_init_qs_minus26_ = 0;
    int64_t chroma_qp_index_offset_ = 0;
    bool deblocking_filter_control_present_flag_ = false;
    bool constrained_intra_pred_flag_ = false;
    bool redundant_pic_cnt_present_flag_ = false;
    bool transform_8x8_mode_flag_ = false;
    bool pic_scaling_matrix_present_flag_ = false;
};

class RefPicListModification {
public:
    /* TODO: refactor this after decoding is finished */
    RefPicListModification();
    RefPicListModification(uint64_t slice_type, BitReader & br);
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
                    BitReader & br);
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
    DecRefPicMarking(const NALUnit &unit, BitReader &br);
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
                                                          pwt(), drpm() {}

    std::pair<uint64_t, uint8_t> header_size() const
    { return std::make_pair<uint64_t, uint8_t>(
                (uint64_t)_header_size[0], (uint8_t)_header_size[1]); }

    void parse(ParserContext & ctx, BitReader &br);
private:
    uint64_t _header_size[2] = {0, 0};
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
    explicit SliceData(const Slice_NALUnit & nal) : _nal(nal) {}
    void parse(ParserContext & ctx, BitReader & br);
private:
    const Slice_NALUnit & _nal;

    bool mbaff_frame_flag(std::shared_ptr<SPS_NALUnit> sps,
                          const std::shared_ptr<SliceHeader> header) {
        return  sps->mb_adaptive_frame_field_flag() && !header->field_pic_flag;
    }

    uint64_t next_mb_addr(uint64_t n, ParserContext & ctx);

    std::vector<uint64_t> slice_group_map(std::shared_ptr<SPS_NALUnit> sps,
                                          std::shared_ptr<PPS_NALUnit> pps);

    bool more_rbsp_data(BitReader & br);
    void find_trailing_bit();
    uint64_t _trailing_bit = 0;
};


class MbPred {
public:
    MbPred();

    void parse(ParserContext &ctx, BitReader &br);

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
    void parse(ParserContext &ctx, BitReader &br);
    uint64_t sub_mb_type[4];
    uint64_t ref_idx_l0[4];
    uint64_t ref_idx_l1[4];
    int64_t mvd_l0[4][4][2];
    int64_t mvd_l1[4][4][2];
};

class ResidualBlock {
public:
    ResidualBlock(uint32_t start_index, uint32_t end_index,
                  uint32_t max_num_coeff, BlockType block_type,
                  uint32_t block_index)
            : start_index(start_index), end_index(end_index),
              max_num_coeff(max_num_coeff), block_type(block_type),
              block_index(block_index) {}

    void parse(ParserContext &ctx, int * coeffLevel, BitReader &br);

    uint32_t start_index;
    uint32_t end_index;
    uint32_t max_num_coeff;
    BlockType block_type;
    uint32_t block_index;
private:
    /* taken from https://github.com/emericg/MiniVideo */
    void deriv_4x4lumablocks(ParserContext &ctx,
                             const int luma4x4BlkIdx, int &mbAddrA,
                             int &luma4x4BlkIdxA, int &mbAddrB,
                             int &luma4x4BlkIdxB);
    inline void InverseLuma4x4BlkScan(const int luma4x4BlkIdx, int &x, int &y);
    void deriv_neighbouringlocations(ParserContext &ctx, const bool lumaBlock,
                                     const int xN, const int yN, int &mbAddrN,
                                     int &xW, int &yW);
    inline int deriv_4x4lumablock_indices(const int xP, const int yP) {
        return 8 * (yP / 8) + 4 * (xP / 8) +
                2 * ((yP % 8) / 4) + ((xP % 8) / 4);
    }
    void deriv_4x4chromablocks(ParserContext &ctx,
                               const int chroma4x4BlkIdx,
                               int &mbAddrA, int &chroma4x4BlkIdxA,
                               int &mbAddrB, int &chroma4x4BlkIdxB);

    /* From 'ITU-T H.264' recommendation:
     * 6.4.12.2 Derivation process for 4x4 chroma block indices.
     *
     * This subclause is only invoked when ChromaArrayType is equal to 1 or 2.
     * Be carefull, the equation used in this function is not the same as the one
     * described in the specification.
     */
    inline int deriv_4x4chromablock_indices(const int xP, const int yP) {
        //FIXME why this is not the same equation as in the 'ITU-T H.264' recommendation?
        return (int)(2.0 * floor((yP / 8.0) + 0.5) + floor((xP / 8.0) + 0.5));
    }
};

class Residual {
public:
    Residual(uint32_t start_index, uint32_t end_index)
            : start_index(start_index), end_index(end_index),
              residual_blocks() {}
    void parse(ParserContext &ctx, BitReader & br);

    void residual_luma(ParserContext &ctx, const int startIdx,
                       const int endIdx, BitReader &br);
    void residual_chroma(ParserContext &ctx, const int startIdx,
                       const int endIdx, BitReader &br);

    uint32_t start_index;
    uint32_t end_index;

    std::vector<std::shared_ptr<ResidualBlock>> residual_blocks;
};

class MacroBlock {
public:
    MacroBlock(bool mb_field_decoding_flag, uint64_t curr_mb_addr);
    MacroBlock(ParserContext & ctx, bool mb_field_decoding_flag,
               uint64_t curr_mb_addr);
    void parse(ParserContext &ctx, BitReader &br);
    uint64_t mb_type = P_Skip; /* default to skip */
    bool transform_size_8x8_flag = false;

    std::unique_ptr<MbPred> mb_pred;
    std::vector<std::unique_ptr<SubMbPred>> sub_mb_preds;
    std::unique_ptr<Residual> residual = nullptr;
    bool mb_field_decoding_flag;
    int64_t mb_qp_delta = 0;
    uint64_t mb_addr;
    int64_t mbAddrA = -1;
    int64_t mbAddrB = -1;
    int64_t mbAddrC = -1;
    int64_t mbAddrD = -1;

    int64_t mbPartIdxA = -1;
    int64_t mbPartIdxB = -1;
    int64_t mbPartIdxC = -1;
    int64_t mbPartIdxD = -1;

    int TotalCoeffs_luma[16];
    int TotalCoeffs_chroma[2][4];

    int LumaLevel4x4[16][16];               //!< An array of 16 blocks of (4x4) 16 coefficients
    int LumaLevel8x8[4][64];                //!< An array of 4 blocks of (8x8) 64 coefficients
    int Intra16x16DCLevel[16];              //!< An array of 16 luma DC coeff
    int Intra16x16ACLevel[16][15];          //!< An array of 16 blocks of (4*4 - 1) 15 AC coefficients

    int ChromaDCLevel[2][4];                //!< Store chroma DC coeff
    int ChromaACLevel[2][4][15];            //!< Store chroma AC coeff

    uint64_t CodedBlockPatternLuma = 0;
    uint64_t CodedBlockPatternChroma = 0;

    uint64_t slice_type = 0; /* will be assigned in parsing */

    int mvL[2][4][4][2];
    bool predFlagL[2][4];
    int refIdxL[2][4];

    uint64_t mbPartIdxTable [4][4];

    uint32_t pos_x() { return (uint32_t)_pos_x; }
    uint32_t pos_y() { return (uint32_t)_pos_y; }

private:
    void compute_mb_neighbours(ParserContext &ctx);
    void compute_mb_index(ParserContext &ctx);
    void assign_pos(ParserContext & ctx);

    uint64_t _pos_x = 0;
    uint64_t _pos_y = 0;

};

class Slice_NALUnit : public NALUnit {
public:
    explicit Slice_NALUnit(std::string data);
    explicit Slice_NALUnit(NALUnit & unit);
    explicit Slice_NALUnit(std::shared_ptr<NALUnit> & unit) :
            Slice_NALUnit(*unit.get()) {}

    void parse(ParserContext & ctx);

    std::shared_ptr<SliceHeader> header() { return _header; }
private:
    std::shared_ptr<SliceHeader> _header = nullptr;
    std::shared_ptr<SliceData> _slice_data = nullptr;
};

class ParserContext {
public:
    ParserContext(std::shared_ptr<SPS_NALUnit> sps,
                  std::shared_ptr<PPS_NALUnit> pps) : sps(sps), pps(pps),
                                                      mb_array() {}
    std::shared_ptr<SPS_NALUnit> sps;
    std::shared_ptr<PPS_NALUnit> pps;
    std::shared_ptr<MacroBlock> mb = nullptr;
    std::vector<std::shared_ptr<MacroBlock>> mb_array;

    inline uint64_t PicHeightInMapUnits()
    { return sps->pic_height_in_map_units_minus1() + 1; }
    inline uint64_t PicWidthInMbs()
    { return sps->pic_width_in_mbs_minus1() + 1; }
    inline uint64_t FrameHeightInMbs()
    { return  (2 - sps->frame_mbs_only_flag()) * PicHeightInMapUnits(); }
    inline uint64_t PicHeightInMbs()
    { return _header ? FrameHeightInMbs() / (1 + _header->field_pic_flag) : 0; }
    uint64_t PicSizeInMbs() { return PicWidthInMbs() * PicHeightInMbs(); }

    uint32_t SubHeightC();
    uint32_t SubWidthC();
    inline uint32_t MbWidthC() { return 16 / SubWidthC(); }
    inline uint32_t MbHeightC() { return 16 / SubHeightC(); }

    uint32_t Height();
    uint32_t Width();

    std::shared_ptr<SliceHeader> header() { return _header; }
    void set_header(std::shared_ptr<SliceHeader> header);

private:
    std::shared_ptr<SliceHeader> _header = nullptr;
};

#endif //H264FLOW_NAL_HH
