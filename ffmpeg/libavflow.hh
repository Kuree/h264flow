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

#ifndef H264FLOW_LIBAVFLOW_HH
#define H264FLOW_LIBAVFLOW_HH

extern "C" {
#include <libavutil/motion_vector.h>
#include <libavformat/avformat.h>
}

#include <iostream>
#include <vector>


class LibAvFlow {
public:
    explicit LibAvFlow(const std::string &filename);

    inline std::vector<std::vector<std::pair<int, int>>> get_mv() const
    { return mv_data_; }
    inline std::vector<uint8_t > get_luma() const { return luma_data_; }
    int current_frame_num() const { return video_frame_count; }
    ~LibAvFlow();
    /* this one has to be called first */
    bool decode_frame();
    inline int64_t total_frames() const { return total_frames_; }
    inline int width() { return width_; }
    inline int height() { return height_; }

private:
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_dec_ctx = nullptr;
    AVStream *video_stream = nullptr;

    int video_stream_idx = -1;
    AVFrame *frame = nullptr;
    int video_frame_count = 0;

    int height_ = 0;
    int width_ = 0;

    int64_t total_frames_ = 0;
    bool cached_state_ = false;

    void clean_up();

    void open_codec_context(AVFormatContext *fmt_ctx, enum AVMediaType type);

    void decode_pkt(const AVPacket *pkt);

    std::vector<uint8_t> luma_data_;
    std::vector<std::vector<std::pair<int, int>>> mv_data_;
};


#endif //H264FLOW_LIBAVFLOW_HH
