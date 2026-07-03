#pragma once

#include <deque>

#include <opencv2/core.hpp>

struct PreprocessorOptions {
    int outputWidth = 300;
    int outputHeight = 300;
    double centerKeepRatio = 0.33;
    bool enableTrail = true;
    int trailBrightnessThreshold = 160;
    double trailDisableMotionRatio = 0.10;
    int trailReenableFrames = 5;
    int trailHistoryFrames = 5;
};

class FramePreprocessor {
public:
    explicit FramePreprocessor(const PreprocessorOptions& options);

    cv::Mat process(const cv::Mat& sourceBgr);

private:
    PreprocessorOptions options_;
    cv::Mat previousGray_;
    std::deque<cv::Mat> trailHistory_;
    int lowMotionFrameCount_ = 0;
    bool trailTemporarilyDisabled_ = false;
};
