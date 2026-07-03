#include "frame_preprocessor.h"

#include <algorithm>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace {

cv::Rect centerSquare(const cv::Mat& source) {
    const int side = std::min(source.cols, source.rows);
    return {(source.cols - side) / 2, (source.rows - side) / 2, side, side};
}

}  // namespace

FramePreprocessor::FramePreprocessor(const PreprocessorOptions& options) : options_(options) {
    options_.trailBrightnessThreshold = std::clamp(options_.trailBrightnessThreshold, 0, 255);
    options_.trailDisableMotionRatio = std::clamp(options_.trailDisableMotionRatio, 0.0, 1.0);
    options_.trailReenableFrames = std::max(1, options_.trailReenableFrames);
    options_.trailHistoryFrames = std::max(1, options_.trailHistoryFrames);
}

cv::Mat FramePreprocessor::process(const cv::Mat& sourceBgr) {
    cv::Mat roi = sourceBgr(centerSquare(sourceBgr)).clone();
    cv::resize(roi, roi, {options_.outputWidth, options_.outputHeight}, 0.0, 0.0, cv::INTER_AREA);

    cv::Mat gray;
    cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);

    const bool hasPreviousFrame = !previousGray_.empty();
    cv::Mat motionMask(gray.size(), CV_8UC1, cv::Scalar(255));
    double motionPixelRatio = 0.0;
    if (hasPreviousFrame) {
        cv::Mat diff;
        cv::absdiff(gray, previousGray_, diff);
        cv::threshold(diff, motionMask, 18, 255, cv::THRESH_BINARY);
        cv::erode(motionMask, motionMask, cv::Mat(), {-1, -1}, 1);
        cv::dilate(motionMask, motionMask, cv::Mat(), {-1, -1}, 3);
        motionPixelRatio = static_cast<double>(cv::countNonZero(motionMask)) / static_cast<double>(motionMask.total());
    }
    previousGray_ = gray;

    cv::Mat blurred;
    cv::GaussianBlur(roi, blurred, {11, 11}, 0.0);
    cv::Mat blurredGray;
    cv::cvtColor(blurred, blurredGray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(blurredGray, blurred, cv::COLOR_GRAY2BGR);

    cv::Mat output = blurred.clone();
    roi.copyTo(output, motionMask);

    const int centerWidth = static_cast<int>(output.cols * options_.centerKeepRatio);
    const int centerHeight = static_cast<int>(output.rows * options_.centerKeepRatio);
    const cv::Rect centerRect((output.cols - centerWidth) / 2, (output.rows - centerHeight) / 2, centerWidth, centerHeight);
    roi(centerRect).copyTo(output(centerRect));

    if (options_.enableTrail) {
        if (!hasPreviousFrame) {
            trailHistory_.clear();
            lowMotionFrameCount_ = 0;
            trailTemporarilyDisabled_ = false;
        } else {
            if (motionPixelRatio > options_.trailDisableMotionRatio) {
                trailTemporarilyDisabled_ = true;
                lowMotionFrameCount_ = 0;
                trailHistory_.clear();
            } else if (trailTemporarilyDisabled_) {
                ++lowMotionFrameCount_;
                if (lowMotionFrameCount_ >= options_.trailReenableFrames) {
                    trailTemporarilyDisabled_ = false;
                    lowMotionFrameCount_ = 0;
                }
            }

            cv::Mat motionHighlight(gray.size(), CV_8UC1, cv::Scalar(0));
            if (!trailTemporarilyDisabled_) {
                cv::Mat brightMask;
                cv::threshold(gray, brightMask, options_.trailBrightnessThreshold, 255, cv::THRESH_BINARY);
                cv::Mat brightMotionMask;
                cv::bitwise_and(brightMask, motionMask, brightMotionMask);
                cv::bitwise_and(gray, brightMotionMask, motionHighlight);
            }

            trailHistory_.push_back(motionHighlight);
            while (static_cast<int>(trailHistory_.size()) > options_.trailHistoryFrames) {
                trailHistory_.pop_front();
            }

            cv::Mat trailHighlight(gray.size(), CV_8UC1, cv::Scalar(0));
            for (const cv::Mat& historyFrame : trailHistory_) {
                cv::max(trailHighlight, historyFrame, trailHighlight);
            }

            std::vector<cv::Mat> channels;
            cv::split(output, channels);
            channels[2] = cv::max(channels[2], trailHighlight);
            channels[1] = cv::max(channels[1], trailHighlight / 2);
            cv::merge(channels, output);
        }
    }

    return output;
}
