#include "video_encoder.h"

#include <stdexcept>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace {

void throwOnFfmpegError(int errorCode, const std::string& action) {
    char buffer[256] = {0};
    av_strerror(errorCode, buffer, sizeof(buffer));
    throw std::runtime_error(action + ": " + std::string(buffer));
}

}  // namespace

VideoEncoder::VideoEncoder(const EncoderOptions& options) : options_(options) {
    const AVCodec* codec = avcodec_find_encoder_by_name(options.codecName.c_str());
    if (codec == nullptr) {
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (codec == nullptr) {
        throw std::runtime_error("No H.264 encoder found in FFmpeg");
    }

    codecContext_ = avcodec_alloc_context3(codec);
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (codecContext_ == nullptr || frame_ == nullptr || packet_ == nullptr) {
        throw std::runtime_error("Failed to allocate FFmpeg encoder resources");
    }

    codecContext_->width = options.width;
    codecContext_->height = options.height;
    codecContext_->time_base = AVRational{1, options.fps};
    codecContext_->framerate = AVRational{options.fps, 1};
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext_->bit_rate = static_cast<long long>(options.targetBytesPerSec) * 8LL;
    codecContext_->gop_size = options.gop;
    codecContext_->max_b_frames = 0;

    av_opt_set(codecContext_->priv_data, "preset", "veryfast", 0);
    av_opt_set(codecContext_->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codecContext_->priv_data, "profile", "baseline", 0);
    av_opt_set(codecContext_->priv_data, "x264-params", "repeat-headers=1:keyint=30:min-keyint=30:scenecut=0", 0);

    const int openRet = avcodec_open2(codecContext_, codec, nullptr);
    if (openRet < 0) {
        throwOnFfmpegError(openRet, "avcodec_open2 failed");
    }

    frame_->format = codecContext_->pix_fmt;
    frame_->width = codecContext_->width;
    frame_->height = codecContext_->height;

    const int bufferRet = av_frame_get_buffer(frame_, 32);
    if (bufferRet < 0) {
        throwOnFfmpegError(bufferRet, "av_frame_get_buffer failed");
    }

    swsContext_ = sws_getContext(
        options.width,
        options.height,
        AV_PIX_FMT_BGR24,
        options.width,
        options.height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    if (swsContext_ == nullptr) {
        throw std::runtime_error("Failed to create swscale context");
    }
}

VideoEncoder::~VideoEncoder() {
    if (swsContext_ != nullptr) {
        sws_freeContext(swsContext_);
    }
    av_packet_free(&packet_);
    av_frame_free(&frame_);
    avcodec_free_context(&codecContext_);
}

void VideoEncoder::encode(const cv::Mat& bgrFrame, const PacketCallback& callback) {
    if (bgrFrame.cols != options_.width || bgrFrame.rows != options_.height || bgrFrame.type() != CV_8UC3) {
        throw std::runtime_error("VideoEncoder received unexpected frame format");
    }

    const int writableRet = av_frame_make_writable(frame_);
    if (writableRet < 0) {
        throwOnFfmpegError(writableRet, "av_frame_make_writable failed");
    }

    const std::uint8_t* srcSlices[1] = {bgrFrame.data};
    const int srcStride[1] = {static_cast<int>(bgrFrame.step)};
    sws_scale(swsContext_, srcSlices, srcStride, 0, options_.height, frame_->data, frame_->linesize);

    frame_->pts = nextPts_++;
    const int sendRet = avcodec_send_frame(codecContext_, frame_);
    if (sendRet < 0) {
        throwOnFfmpegError(sendRet, "avcodec_send_frame failed");
    }

    drain(callback);
}

void VideoEncoder::flush(const PacketCallback& callback) {
    const int sendRet = avcodec_send_frame(codecContext_, nullptr);
    if (sendRet >= 0) {
        drain(callback);
    }
}

void VideoEncoder::drain(const PacketCallback& callback) {
    while (true) {
        const int receiveRet = avcodec_receive_packet(codecContext_, packet_);
        if (receiveRet == AVERROR(EAGAIN) || receiveRet == AVERROR_EOF) {
            return;
        }
        if (receiveRet < 0) {
            throwOnFfmpegError(receiveRet, "avcodec_receive_packet failed");
        }

        callback(packet_->data, static_cast<std::size_t>(packet_->size), (packet_->flags & AV_PKT_FLAG_KEY) != 0);
        av_packet_unref(packet_);
    }
}
