#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include <opencv2/core.hpp>

struct EncoderOptions {
    int width = 300;
    int height = 300;
    int fps = 60;
    int targetBytesPerSec = 11000;
    int gop = 30;
    std::string codecName = "libx264";
};

class VideoEncoder {
public:
    using PacketCallback = std::function<void(const std::uint8_t* data, std::size_t size, bool keyframe)>;

    explicit VideoEncoder(const EncoderOptions& options);
    ~VideoEncoder();

    VideoEncoder(const VideoEncoder&) = delete;
    VideoEncoder& operator=(const VideoEncoder&) = delete;

    void encode(const cv::Mat& bgrFrame, const PacketCallback& callback);
    void flush(const PacketCallback& callback);

private:
    void drain(const PacketCallback& callback);

    EncoderOptions options_;
    struct AVCodecContext* codecContext_ = nullptr;
    struct AVFrame* frame_ = nullptr;
    struct AVPacket* packet_ = nullptr;
    struct SwsContext* swsContext_ = nullptr;
    std::int64_t nextPts_ = 0;
};
