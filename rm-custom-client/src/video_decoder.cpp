#include "video_decoder.h"

#include <QtCore/QByteArray>

#include <cstdint>
#include <memory>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace {

QString ffmpegErrorString(int errorCode) {
    char buffer[256] = {0};
    av_strerror(errorCode, buffer, sizeof(buffer));
    return QString::fromLocal8Bit(buffer);
}

}  // namespace

VideoDecoder::VideoDecoder(AVCodecID codecId, QObject* parent) : QObject(parent), codecId_(codecId) {
    initDecoder();
}

VideoDecoder::~VideoDecoder() {
    releaseDecoder();
}

void VideoDecoder::initDecoder() {
    const AVCodec* codec = avcodec_find_decoder(codecId_);
    if (codec == nullptr) {
        throw std::runtime_error("FFmpeg decoder not found");
    }

    parser_ = av_parser_init(codecId_);
    codecContext_ = avcodec_alloc_context3(codec);
    frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (parser_ == nullptr || codecContext_ == nullptr || frame_ == nullptr || packet_ == nullptr) {
        throw std::runtime_error("Failed to allocate FFmpeg decode resources");
    }

    if (codecId_ == AV_CODEC_ID_HEVC) {
        codecContext_->thread_count = 4;
        codecContext_->flags2 |= AV_CODEC_FLAG2_CHUNKS;
    }

    const int ret = avcodec_open2(codecContext_, codec, nullptr);
    if (ret < 0) {
        throw std::runtime_error(QStringLiteral("avcodec_open2 failed: %1").arg(ffmpegErrorString(ret)).toStdString());
    }

    emit logMessage(QStringLiteral("[信息] 解码器初始化完成，codecId=%1，thread_count=%2，chunks=%3")
                        .arg(static_cast<int>(codecId_))
                        .arg(codecContext_->thread_count)
                        .arg((codecContext_->flags2 & AV_CODEC_FLAG2_CHUNKS) != 0 ? QStringLiteral("开") : QStringLiteral("关")));
}

void VideoDecoder::releaseDecoder() {
    if (swsContext_ != nullptr) {
        sws_freeContext(swsContext_);
        swsContext_ = nullptr;
    }
    av_parser_close(parser_);
    parser_ = nullptr;
    av_packet_free(&packet_);
    av_frame_free(&frame_);
    avcodec_free_context(&codecContext_);
}

void VideoDecoder::feedBytes(const QByteArray& bytes) {
    if (bytes.isEmpty() || parser_ == nullptr || codecContext_ == nullptr) {
        return;
    }

    const std::uint8_t* data = reinterpret_cast<const std::uint8_t*>(bytes.constData());
    int remaining = bytes.size();

    while (remaining > 0) {
        std::uint8_t* outData = nullptr;
        int outSize = 0;
        const int consumed = av_parser_parse2(
            parser_,
            codecContext_,
            &outData,
            &outSize,
            data,
            remaining,
            AV_NOPTS_VALUE,
            AV_NOPTS_VALUE,
            0);

        if (consumed < 0) {
            emit logMessage(QStringLiteral("[错误] 解码器解析码流失败: %1").arg(ffmpegErrorString(consumed)));
            return;
        }

        data += consumed;
        remaining -= consumed;

        if (outSize == 0) {
            continue;
        }

        av_packet_unref(packet_);
        packet_->data = outData;
        packet_->size = outSize;
        const int sendRet = avcodec_send_packet(codecContext_, packet_);
        if (sendRet < 0) {
            emit logMessage(QStringLiteral("[错误] 解码器送入码流失败: %1").arg(ffmpegErrorString(sendRet)));
            continue;
        }
        drainDecoder();
    }
}

void VideoDecoder::reset() {
    if (codecContext_ != nullptr) {
        avcodec_flush_buffers(codecContext_);
    }
    if (parser_ != nullptr) {
        av_parser_close(parser_);
    }
    parser_ = av_parser_init(codecId_);
    emit logMessage(QStringLiteral("[警告] 解码器已重置，等待新的有效码流"));
}

void VideoDecoder::drainDecoder() {
    while (true) {
        const int ret = avcodec_receive_frame(codecContext_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }
        if (ret < 0) {
            emit logMessage(QStringLiteral("[错误] 解码器取出图像帧失败: %1").arg(ffmpegErrorString(ret)));
            return;
        }

        swsContext_ = sws_getCachedContext(
            swsContext_,
            frame_->width,
            frame_->height,
            static_cast<AVPixelFormat>(frame_->format),
            frame_->width,
            frame_->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);
        if (swsContext_ == nullptr) {
            emit logMessage(QStringLiteral("[错误] 图像颜色空间转换上下文创建失败"));
            return;
        }

        QImage image(frame_->width, frame_->height, QImage::Format_RGB888);
        uint8_t* destData[4] = {image.bits(), nullptr, nullptr, nullptr};
        int destLinesize[4] = {static_cast<int>(image.bytesPerLine()), 0, 0, 0};
        sws_scale(
            swsContext_,
            frame_->data,
            frame_->linesize,
            0,
            frame_->height,
            destData,
            destLinesize);
        static int decodedFrames = 0;
        ++decodedFrames;
        if (decodedFrames == 1 || decodedFrames % 100 == 0) {
            emit logMessage(QStringLiteral("[信息] 已成功解码图像帧 %1，尺寸 %2x%3")
                                .arg(decodedFrames)
                                .arg(frame_->width)
                                .arg(frame_->height));
        }
        emit frameDecoded(image);
    }
}
