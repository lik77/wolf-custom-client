#include "camera_source.h"

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#ifdef WITH_MVS_SDK
#include <MvCameraControl.h>
#endif

namespace {

std::int64_t nowUs() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

#ifdef WITH_MVS_SDK
std::string readSdkString(const unsigned char* value, std::size_t capacity) {
    std::size_t length = 0;
    while (length < capacity && value[length] != '\0') {
        ++length;
    }
    return std::string(reinterpret_cast<const char*>(value), length);
}

std::string mvsDeviceSerial(const MV_CC_DEVICE_INFO* deviceInfo) {
    if (deviceInfo == nullptr) {
        return {};
    }
    if (deviceInfo->nTLayerType == MV_GIGE_DEVICE) {
        return readSdkString(
            deviceInfo->SpecialInfo.stGigEInfo.chSerialNumber,
            sizeof(deviceInfo->SpecialInfo.stGigEInfo.chSerialNumber));
    }
    if (deviceInfo->nTLayerType == MV_USB_DEVICE) {
        return readSdkString(
            deviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber,
            sizeof(deviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber));
    }
    return {};
}

std::string mvsDeviceModel(const MV_CC_DEVICE_INFO* deviceInfo) {
    if (deviceInfo == nullptr) {
        return {};
    }
    if (deviceInfo->nTLayerType == MV_GIGE_DEVICE) {
        return readSdkString(
            deviceInfo->SpecialInfo.stGigEInfo.chModelName,
            sizeof(deviceInfo->SpecialInfo.stGigEInfo.chModelName));
    }
    if (deviceInfo->nTLayerType == MV_USB_DEVICE) {
        return readSdkString(
            deviceInfo->SpecialInfo.stUsb3VInfo.chModelName,
            sizeof(deviceInfo->SpecialInfo.stUsb3VInfo.chModelName));
    }
    return {};
}

std::string describeMvsDevices(const MV_CC_DEVICE_INFO_LIST& deviceList) {
    std::string description;
    for (unsigned int index = 0; index < deviceList.nDeviceNum; ++index) {
        if (const MV_CC_DEVICE_INFO* deviceInfo = deviceList.pDeviceInfo[index]; deviceInfo != nullptr) {
            if (!description.empty()) {
                description += "; ";
            }
            const std::string serial = mvsDeviceSerial(deviceInfo);
            const std::string model = mvsDeviceModel(deviceInfo);
            description += "#" + std::to_string(index) + " serial=" + (serial.empty() ? "<empty>" : serial);
            if (!model.empty()) {
                description += " model=" + model;
            }
        }
    }
    return description.empty() ? "<none>" : description;
}

int resolveMvsDeviceIndex(const CameraOptions& options, const MV_CC_DEVICE_INFO_LIST& deviceList) {
    const std::string& fixedSerial = options.deviceSerial;
    if (!fixedSerial.empty()) {
        for (unsigned int index = 0; index < deviceList.nDeviceNum; ++index) {
            if (fixedSerial == mvsDeviceSerial(deviceList.pDeviceInfo[index])) {
                return static_cast<int>(index);
            }
        }
        throw std::runtime_error(
            "Configured MVS serial not found: " + fixedSerial + ". Available devices: " + describeMvsDevices(deviceList));
    }

    if (options.deviceIndex < 0 || options.deviceIndex >= static_cast<int>(deviceList.nDeviceNum)) {
        throw std::runtime_error(
            "MVS device index out of range. Available devices: " + describeMvsDevices(deviceList));
    }
    return options.deviceIndex;
}
#endif

class OpenCvCameraSource final : public CameraSource {
public:
    explicit OpenCvCameraSource(const CameraOptions& options) {
        if (!options.videoPath.empty()) {
            capture_.open(options.videoPath, cv::CAP_ANY);
        } else {
            capture_.open(options.deviceIndex, cv::CAP_ANY);
        }
        if (!capture_.isOpened()) {
            throw std::runtime_error("Failed to open OpenCV capture source");
        }
        capture_.set(cv::CAP_PROP_FPS, options.fps);
        capture_.set(cv::CAP_PROP_GAIN, options.gain);
    }

    bool read(cv::Mat& frame, std::int64_t& timestampUs) override {
        if (!capture_.read(frame) || frame.empty()) {
            return false;
        }
        timestampUs = nowUs();
        return true;
    }

    cv::Size frameSize() const override {
        return {
            static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
            static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT))
        };
    }

private:
    cv::VideoCapture capture_;
};

#ifdef WITH_MVS_SDK
class MvsCameraSource final : public CameraSource {
public:
    explicit MvsCameraSource(const CameraOptions& options) {
        MV_CC_DEVICE_INFO_LIST deviceList {};
        const int enumerateRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList);
        if (enumerateRet != MV_OK || deviceList.nDeviceNum == 0) {
            throw std::runtime_error("No Hikrobot MVS camera found");
        }
        const int deviceIndex = resolveMvsDeviceIndex(options, deviceList);

        const int createRet = MV_CC_CreateHandle(&handle_, deviceList.pDeviceInfo[deviceIndex]);
        if (createRet != MV_OK) {
            throw std::runtime_error("MV_CC_CreateHandle failed");
        }

        if (MV_CC_OpenDevice(handle_) != MV_OK) {
            throw std::runtime_error("MV_CC_OpenDevice failed");
        }

        MV_CC_SetEnumValue(handle_, "TriggerMode", 0);
        MV_CC_SetEnumValue(handle_, "ExposureAuto", 0);
        MV_CC_SetEnumValue(handle_, "GainAuto", 0);
        MV_CC_SetFloatValue(handle_, "AcquisitionFrameRate", static_cast<float>(options.fps));
        MV_CC_SetFloatValue(handle_, "ExposureTime", static_cast<float>(options.exposureUs));
        MV_CC_SetFloatValue(handle_, "Gain", static_cast<float>(options.gain));

        if (MV_CC_StartGrabbing(handle_) != MV_OK) {
            throw std::runtime_error("MV_CC_StartGrabbing failed");
        }
    }

    ~MvsCameraSource() override {
        if (handle_ != nullptr) {
            MV_CC_StopGrabbing(handle_);
            MV_CC_CloseDevice(handle_);
            MV_CC_DestroyHandle(handle_);
            handle_ = nullptr;
        }
    }

    bool read(cv::Mat& frame, std::int64_t& timestampUs) override {
        MV_FRAME_OUT outFrame {};
        const int ret = MV_CC_GetImageBuffer(handle_, &outFrame, 1000);
        if (ret != MV_OK) {
            return false;
        }

        const int width = static_cast<int>(outFrame.stFrameInfo.nWidth);
        const int height = static_cast<int>(outFrame.stFrameInfo.nHeight);
        convertedBuffer_.resize(static_cast<std::size_t>(width * height * 3));

        MV_CC_PIXEL_CONVERT_PARAM convertParam {};
        convertParam.nWidth = outFrame.stFrameInfo.nWidth;
        convertParam.nHeight = outFrame.stFrameInfo.nHeight;
        convertParam.pSrcData = outFrame.pBufAddr;
        convertParam.nSrcDataLen = outFrame.stFrameInfo.nFrameLen;
        convertParam.enSrcPixelType = outFrame.stFrameInfo.enPixelType;
        convertParam.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
        convertParam.pDstBuffer = convertedBuffer_.data();
        convertParam.nDstBufferSize = static_cast<unsigned int>(convertedBuffer_.size());

        const int convertRet = MV_CC_ConvertPixelType(handle_, &convertParam);
        MV_CC_FreeImageBuffer(handle_, &outFrame);

        if (convertRet != MV_OK) {
            return false;
        }

        frame = cv::Mat(height, width, CV_8UC3, convertedBuffer_.data()).clone();
        size_ = frame.size();
        timestampUs = nowUs();
        return true;
    }

    cv::Size frameSize() const override {
        return size_;
    }

private:
    void* handle_ = nullptr;
    std::vector<unsigned char> convertedBuffer_;
    cv::Size size_;
};
#endif

}  // namespace

std::unique_ptr<CameraSource> createCameraSource(const CameraOptions& options) {
    if (options.source == "file") {
        return std::make_unique<OpenCvCameraSource>(options);
    }

    if (options.source == "opencv") {
        return std::make_unique<OpenCvCameraSource>(options);
    }

    if (options.source == "mvs") {
#ifdef WITH_MVS_SDK
        return std::make_unique<MvsCameraSource>(options);
#else
        throw std::runtime_error("MVS SDK support was not compiled in");
#endif
    }

    throw std::runtime_error("Unknown camera source: " + options.source);
}
