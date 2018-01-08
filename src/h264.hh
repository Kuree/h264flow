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

class h264 {
public:
    h264(std::shared_ptr<MP4File> mp4);
    h264(std::shared_ptr<BitStream> stream);

    void index_nal();
    void load_frame(uint64_t frame_num);
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
    void process_luma_mv(ParserContext &ctx, int (mvLA)[2],
                         int (mvLB)[2], int (mvLC)[2], int refIdxLA,
                         int refIdxLB, int refIdxLC, int (&mvL)[2]);

    void process_luma_mv(ParserContext &ctx, int listSuffixFlag, int (&mvL)[2]);
};


#endif //H264FLOW_H264_HH
