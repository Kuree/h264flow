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

class h264 {
public:
    h264(std::shared_ptr<MP4File> mp4);

    void index_nal();
    void load_frame(uint64_t frame_num);
private:
    uint8_t _length_size = 4;
    std::vector<uint64_t> _chunk_offsets;
    std::shared_ptr<MP4File> _mp4 = nullptr;
    std::shared_ptr<SPS_NALUnit> _sps = nullptr;
    std::shared_ptr<PPS_NALUnit> _pps = nullptr;
    std::shared_ptr<Box> _trak_box = nullptr;

};


#endif //H264FLOW_H264_HH
