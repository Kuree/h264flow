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

    std::vector<std::vector<std::pair<int, int>>> get_mv();
    /* return in raster order. has be called after MV is obtained */
    /* TODO: fix this */
    std::vector<uint8_t > get_luma();
    int current_frame_num() const { return video_frame_count; }
    ~LibAvFlow();

private:
    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *video_dec_ctx = nullptr;
    AVStream *video_stream = nullptr;

    int video_stream_idx = -1;
    AVFrame *frame = NULL;
    int video_frame_count = 0;

    int height = 0;
    int width = 0;

    void clean_up();

    void open_codec_context(AVFormatContext *fmt_ctx, enum AVMediaType type);

    std::vector<std::vector<std::pair<int, int>>>
    decode_pkt(const AVPacket *pkt);

};


#endif //H264FLOW_LIBAVFLOW_HH
