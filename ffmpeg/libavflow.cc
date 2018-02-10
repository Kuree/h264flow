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

void LibAvFlow::decode_pkt(const AVPacket *pkt) {
    int ret = 0;
    if (!cached_state_) {
        ret = avcodec_send_packet(video_dec_ctx, pkt);
        if (ret < 0)
            throw std::runtime_error(
                    "Error while sending a packet to the decoder");
    }
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

            int line_size = frame->linesize[0];
            luma_data_ =  std::vector<uint8_t>(width_ * height_);
            for (int i = 0; i < height_; i++) {
                uint8_t * dst_data = luma_data_.data() + i * width_;
                uint8_t * src_data = frame->data[0] + i * line_size;
                memcpy(dst_data, src_data, width_);
            }

            sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
            if (sd) {
                auto *mvs = (const AVMotionVector *) sd->data;
                int num_mvs = sd->size / sizeof(*mvs);
                raw_mv_data = std::vector<AVMotionVector>(mvs, mvs + num_mvs);
            } else {
                /* no motion vector data */
                raw_mv_data = std::vector<AVMotionVector>();
            }
            break;
        } else {
            throw std::runtime_error("wrong ret");
        }
    }
}

std::vector<std::vector<std::pair<int, int>>> LibAvFlow::get_mv() {
    if (!raw_mv_data.empty()) {
        auto mv_data = std::vector<std::vector<std::pair<int, int>>>(
                height_,std::vector<std::pair<int, int>>(width_));

        for (const auto amv : raw_mv_data) {
            int mv_x = amv.dst_x - amv.src_x;
            int mv_y = amv.dst_y - amv.src_y;
            if (amv.source > 0) {
                mv_x = -mv_x;
                mv_y = -mv_y;
            }
            /* fill in the motion vectors */
            for (int y = 0; y < amv.h; y++) {
                for (int x = 0; x < amv.w; x++) {
                    int px = x + amv.src_x - amv.w / 2;
                    int py = y + amv.src_y - amv.h / 2;
                    if (px < 0 || py < 0 || px >= width_ || py >= height_)
                        continue;
                    mv_data[py][px].first = mv_x;
                    mv_data[py][px].second = mv_y;
                }
            }
        }
        return mv_data;
    } else {
        return std::vector<std::vector<std::pair<int, int>>>();
    }
}

std::vector<std::vector<bool>> LibAvFlow::get_mv_bitmap() {
    if (!raw_mv_data.empty()) {
        /* divided by 4 instead of 8 because in B-frame there can be 4x4
         * partition */
        auto result = std::vector<std::vector<bool>>(
                height_ / 4, std::vector<bool>(width_ / 4, false));
        for (const auto & amv : raw_mv_data) {
            int pos_x = amv.source < 0 ? amv.dst_x : amv.src_x;
            int pos_y = amv.source < 0 ? amv.dst_y : amv.src_y;
            for (int y = 0; y < amv.h / 4; y++) {
                for (int x = 0; x < amv.w / 4; x++) {
                    int px = x + pos_x - amv.w / 2 / 4;
                    int py = y + pos_y - amv.h / 2 / 4;
                    if (px < 0 || py < 0 || px > width_ || py > height_)
                        continue;
                    result[py][px] = true;
                }
            }
        }
        return result;
    } else {
        return std::vector<std::vector<bool>>();
    }
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
        throw std::runtime_error("Could not find %s stream in input file");
    } else {
        int stream_idx = ret;
        st = fmt_ctx->streams[stream_idx];

        total_frames_ = st->nb_frames;

        dec_ctx = avcodec_alloc_context3(dec);
        if (!dec_ctx) {
            fprintf(stderr, "Failed to allocate codec\n");
        }

        ret = avcodec_parameters_to_context(dec_ctx, st->codecpar);
        if (ret < 0) {
            throw std::runtime_error("Failed to copy codec parameters to codec context");
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
        height_ = dec_ctx->height;
        width_ = dec_ctx->width;
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

bool LibAvFlow::decode_frame() {
    AVPacket pkt = {nullptr};
    std::vector<std::vector<std::pair<int, int>>> result;
    int old_frame_num = video_frame_count;
    if (!cached_state_) {
        while (av_read_frame(fmt_ctx, &pkt) >= 0) {
            if (pkt.stream_index == video_stream_idx) {
                decode_pkt(&pkt);
                av_packet_unref(&pkt);
                if (old_frame_num != video_frame_count)
                    return true;
            }
        }
    }
    /* try to flush cached frames */
    pkt.data = nullptr;
    pkt.size = 0;
    decode_pkt(&pkt);
    cached_state_ = true;
    return old_frame_num != video_frame_count;
}