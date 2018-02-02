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


/* Copyright (c) 2012 Stefano Sabatini
 * Copyright (c) 2014 Clément Bœsch
 * Copyright (c) 2017 Keyi Zhang
 * */

#include "libavflow.hh"
#include <algorithm>

/* NOTICE:
 * CANNOT GUARANTEE IT'S MEMORY LEAK FREE!
 * */

LibAvFlow::LibAvFlow(const std::string &filename) {
    av_register_all();

    if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Could not open source file " + filename);

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
        throw std::runtime_error("Could not find stream information");

    open_codec_context(fmt_ctx, AVMEDIA_TYPE_VIDEO);

    if (!video_stream) {
        clean_up();
        throw std::runtime_error(
                "Could not find video stream in the input, aborting");
    }

    frame = av_frame_alloc();
    if (!frame) {
        clean_up();
        throw std::runtime_error("Could not allocate frame");
    }
}

std::vector<std::vector<std::pair<int, int>>>
LibAvFlow::decode_pkt(const AVPacket *pkt) {
    int ret = avcodec_send_packet(video_dec_ctx, pkt);
    if (ret < 0)
        throw std::runtime_error("Error while sending a packet to the decoder");
    std::vector<std::vector<std::pair<int, int>>> result;
    while (ret >= 0)  {
        ret = avcodec_receive_frame(video_dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            throw std::runtime_error("Error while receiving a frame from the "
                                             "decoder");
        }

        if (ret >= 0) {
            AVFrameSideData *sd;
            video_frame_count++;
            height = static_cast<uint32_t>(frame->height);
            width = static_cast<uint32_t>(frame->width);
            result = std::vector<std::vector<std::pair<int, int>>>(height,
                    std::vector<std::pair<int, int>>(width));

            sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
            if (sd) {
                auto *mvs = (const AVMotionVector *) sd->data;
                int num_mvs = sd->size / sizeof(*mvs);
                for (int i = 0; i < num_mvs; i++) {
                    AVMotionVector amv = mvs[i];
                    int mv_x = amv.dst_x - amv.src_x;
                    int mv_y = amv.dst_y - amv.src_y;
                    /* fill in the motion vectors */
                    for (int y = 0; y < amv.h; y++) {
                        for (int x = 0; x < amv.w; x++) {
                            result[y + amv.dst_y - amv.h / 2][x + amv.dst_x - amv.w / 2].first = mv_x;
                            result[y + amv.dst_y - amv.h / 2][x + amv.dst_x - amv.w / 2].second = mv_y;
                        }
                    }
                }
            }
            av_frame_unref(frame);
        } else {
            throw std::runtime_error("wrong ret");
        }
    }
    return result;
}

void LibAvFlow::open_codec_context(AVFormatContext *fmt_ctx,
                                   enum AVMediaType type) {
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = nullptr;
    AVCodec *dec = nullptr;
    AVDictionary *opts = nullptr;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, &dec, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file",
                av_get_media_type_string(type));
    } else {
        int stream_idx = ret;
        st = fmt_ctx->streams[stream_idx];

        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx) {
            fprintf(stderr, "Failed to allocate codec\n");
        }

        ret = avcodec_parameters_to_context(dec_ctx, st->codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters to codec context\n");
        }

        /* Init the video decoder */
        av_dict_set(&opts, "flags2", "+export_mvs", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
        }

        video_stream_idx = stream_idx;
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dec_ctx = dec_ctx;
    }
}

void LibAvFlow::clean_up() {
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
}

LibAvFlow::~LibAvFlow() {
    clean_up();
}

std::vector<std::vector<std::pair<int, int>>> LibAvFlow::get_mv() {
    AVPacket pkt = {nullptr};
    std::vector<std::vector<std::pair<int, int>>> result;
    int old_frame_num = video_frame_count;
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == video_stream_idx) {
            result = decode_pkt(&pkt);
            av_packet_unref(&pkt);
            if (old_frame_num != video_frame_count)
                break;
        }
    }
    return result;
}

std::vector<uint8_t> LibAvFlow::get_luma() {
    if (!frame) {
        return std::vector<uint8_t>();
    }
    int line_size = frame->linesize[0];
    if (width != line_size)
        throw std::runtime_error("line size does not equal to width");
    return std::vector<uint8_t>(frame->data[0], frame->data[0] + width * height);
}
