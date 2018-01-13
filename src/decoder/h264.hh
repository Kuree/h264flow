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
#include "util.hh"

class BitStream {
public:
    explicit BitStream(std::string filename);

    ~BitStream() { _stream.close(); }

    std::vector<std::pair<uint64_t, uint64_t>> chunk_offsets()
    { return _chunk_offsets; }

    std::string extract_stream(uint64_t position, uint64_t size);
private:
    std::ifstream _stream;
    std::vector<std::pair<uint64_t, uint64_t>> _chunk_offsets;
};

struct MotionVector {
    /* packed together. assume their memory is continuous */
    int mvL0[2] = {0, 0};
    int mvL1[2] = {0, 0};
    uint32_t x = 0;
    uint32_t y = 0;

    inline double motion_distance_L0() const
    { return std::sqrt(mvL0[0] * mvL0[0] + mvL0[1] * mvL0[1]); }
};

class MvFrame {
public:
    explicit MvFrame(ParserContext &ctx);
    MvFrame(uint32_t pic_width, uint32_t pic_height, uint32_t mb_width,
            uint32_t mb_height, bool p_frame = false);
    MotionVector get_mv(uint32_t mb_addr);
    MotionVector get_mv(uint32_t x, uint32_t y);
    inline void set_mv(uint32_t x, uint32_t y, MotionVector mv)
    { _mvs[y][x] = mv; }
    inline std::vector<MotionVector> get_row(uint32_t row)
    { return _mvs[row]; }

    inline uint32_t height() const { return _height; }
    inline uint32_t width() const { return _width; }
    inline uint32_t mb_height() const { return _mb_height; }
    inline uint32_t mb_width() const { return _mb_width; }

    inline std::vector<MotionVector>operator[](uint32_t y) { return _mvs[y]; }

    inline bool p_frame() { return _p_frame; }

private:
    uint32_t _height = 0;
    uint32_t _width = 0;
    uint32_t _mb_width = 0;
    uint32_t _mb_height = 0;
    std::vector<std::vector<MotionVector>> _mvs;
    bool _p_frame = true;
};

class h264 {
public:
    explicit h264(const std::string &filename);
    explicit h264(std::shared_ptr<MP4File> mp4);
    explicit h264(std::shared_ptr<BitStream> stream);

    void index_nal();
    MvFrame load_frame(uint64_t frame_num);
    uint64_t index_size();
private:
    uint8_t _length_size = 4;
    std::vector<uint64_t> _chunk_offsets;
    std::shared_ptr<MP4File> _mp4 = nullptr;
    std::shared_ptr<SPS_NALUnit> _sps = nullptr;
    std::shared_ptr<PPS_NALUnit> _pps = nullptr;
    std::shared_ptr<Box> _trak_box = nullptr;
    std::shared_ptr<BitStream> _bit_stream = nullptr;

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
