#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <opencv2/core.hpp>

struct CameraOptions {
    std::string source = "mvs";
    int deviceIndex = 0;
    std::string deviceSerial;
    std::string videoPath;
    int exposureUs = 12000;
    int fps = 60;
    double gain = 0.0;
};

class CameraSource {
public:
    virtual ~CameraSource() = default;

    virtual bool read(cv::Mat& frame, std::int64_t& timestampUs) = 0;
    virtual cv::Size frameSize() const = 0;
};

std::unique_ptr<CameraSource> createCameraSource(const CameraOptions& options);
