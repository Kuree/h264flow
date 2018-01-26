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

#ifndef H264FLOW_H264_HH
#define H264FLOW_H264_HH

#include "mp4.hh"

#define MACROBLOCK_SIZE 16

class BitStream {
public:
    explicit BitStream(std::string filename);

    ~BitStream() { stream_.close(); }

    std::vector<std::pair<uint64_t, uint64_t>> chunk_offsets()
    { return chunk_offsets_; }

    std::string extract_stream(uint64_t position, uint64_t size);
private:
    std::ifstream stream_;
    std::vector<std::pair<uint64_t, uint64_t>> chunk_offsets_;
};

struct MotionVector {
    mutable float mvL0[2] = {0, 0};
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t energy = 0;
};

class MvFrame {
public:
    explicit MvFrame(ParserContext &ctx);
    MvFrame(uint32_t pic_width, uint32_t pic_height, uint32_t mb_width,
            uint32_t mb_height, bool p_frame = false);
    MvFrame() : mvs_() {}
    MvFrame(const MvFrame& frame);
    inline MotionVector get_mv(uint32_t mb_addr) const { return mvs_[mb_addr]; }
    MotionVector get_mv(uint32_t x, uint32_t y) const
    { return mvs_[y * mb_width_ + x]; }
    inline void set_mv(uint32_t x, uint32_t y, MotionVector mv)
    { mvs_[y * mb_width_ + x] = mv; }
    inline void set_mv(uint32_t mb_addr, MotionVector mv)
    { mvs_[mb_addr] = mv; }

    inline uint32_t height() const { return height_; }
    inline uint32_t width() const { return width_; }
    inline uint32_t mb_height() const { return mb_height_; }
    inline uint32_t mb_width() const { return mb_width_; }

    inline std::vector<MotionVector> operator[](const uint32_t &y) const
    { return std::vector<MotionVector>(&mvs_[y * mb_width_],
                                       &mvs_[(y + 1) * mb_width_]); }

    inline bool p_frame() const { return p_frame_; }

private:
    uint32_t height_ = 0;
    uint32_t width_ = 0;
    uint32_t mb_width_ = 0;
    uint32_t mb_height_ = 0;
    std::vector<MotionVector> mvs_;
    bool p_frame_ = true;
};

class h264 {
public:
    explicit h264(const std::string &filename);
    explicit h264(std::shared_ptr<MP4File> mp4);
    explicit h264(std::shared_ptr<BitStream> stream);

    void index_nal();
    std::pair<MvFrame, bool> load_frame(uint64_t frame_num);
    uint64_t index_size();
private:
    uint8_t length_size_ = 4;
    std::vector<uint64_t> chunk_offsets_;
    std::shared_ptr<MP4File> mp4_ = nullptr;
    std::shared_ptr<SPS_NALUnit> sps_ = nullptr;
    std::shared_ptr<PPS_NALUnit> pps_ = nullptr;
    std::shared_ptr<Box> trak_box_ = nullptr;
    std::shared_ptr<BitStream> bit_stream_ = nullptr;

    uint64_t read_nal_size(BinaryReader &br);

    void process_inter_mb(ParserContext &ctx);
    void get_mv_neighbor_part(ParserContext &ctx, int listSuffixFlag, int (&mvLA)[2],
                              int (&mvLB)[2], int (&mvLC)[2], int &refIdxLA,
                              int &refIdxLB, int &refIdxLC);
    void process_luma_mv(ParserContext &ctx,  uint32_t mbPartIdx,
                         int (mvLA)[2], int (mvLB)[2], int (mvLC)[2],
                         int refIdxLA, int refIdxLB, int refIdxLC,
                         int (&mvL)[2]);

    void process_luma_mv(ParserContext &ctx,  uint32_t mbPartIdx,
                         int listSuffixFlag, int (&mvL)[2]);

    void load_bitstream();
    void load_mp4();
};


#endif //H264FLOW_H264_HH
