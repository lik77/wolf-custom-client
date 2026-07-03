#include "camera_source.h"
#include "crc16.h"
#include "frame_preprocessor.h"
#include "hero_protocol.h"
#include "serial_port.h"
#include "video_encoder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <deque>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>

namespace {

std::atomic<bool> gRunning = true;

void logInfo(const std::string& message) {
    std::cerr << "[信息] " << message << std::endl;
}

void logWarn(const std::string& message) {
    std::cerr << "[警告] " << message << std::endl;
}

void logError(const std::string& message) {
    std::cerr << "[错误] " << message << std::endl;
}

void onSignal(int) {
    gRunning = false;
}

struct AppConfig {
    std::string configPath;
    std::string serialDevice = "/dev/ttyUSB0";
    int baudRate = 1500000;
    int maxStreamPacketsPerSec = 44;
    CameraOptions camera;
    PreprocessorOptions preprocessor;
    EncoderOptions encoder;
};

struct Stats {
    std::uint32_t capturedFrames = 0;
    std::uint32_t encodedFrames = 0;
    std::uint32_t sentPackets = 0;
    std::uint32_t droppedPackets = 0;
    std::uint32_t readFailures = 0;
    std::uint32_t keyframePackets = 0;
    std::uint32_t backlogResetCount = 0;
};

struct PendingStreamPacket {
    std::vector<std::uint8_t> payload;
    bool keyframeHint = false;
};

bool parseIntArgument(const std::string& value, int& output) {
    try {
        output = std::stoi(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool readIntNode(const cv::FileNode& node, const char* key, int& output) {
    const cv::FileNode child = node[key];
    if (child.empty()) {
        return false;
    }
    child >> output;
    return true;
}

bool readDoubleNode(const cv::FileNode& node, const char* key, double& output) {
    const cv::FileNode child = node[key];
    if (child.empty()) {
        return false;
    }
    child >> output;
    return true;
}

bool readBoolNode(const cv::FileNode& node, const char* key, bool& output) {
    const cv::FileNode child = node[key];
    if (child.empty()) {
        return false;
    }
    int boolValue = output ? 1 : 0;
    child >> boolValue;
    output = boolValue != 0;
    return true;
}

bool readStringNode(const cv::FileNode& node, const char* key, std::string& output) {
    const cv::FileNode child = node[key];
    if (child.empty()) {
        return false;
    }
    child >> output;
    return true;
}

void syncDerivedConfig(AppConfig& config) {
    config.encoder.width = config.preprocessor.outputWidth;
    config.encoder.height = config.preprocessor.outputHeight;
    config.encoder.fps = config.camera.fps;
}

void applyConfigFile(const std::string& path, AppConfig& config) {
    cv::FileStorage storage(path, cv::FileStorage::READ);
    if (!storage.isOpened()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    config.configPath = path;
    readStringNode(storage.root(), "serial_device", config.serialDevice);
    readIntNode(storage.root(), "baud_rate", config.baudRate);
    readIntNode(storage.root(), "max_stream_packets_per_sec", config.maxStreamPacketsPerSec);

    const cv::FileNode cameraNode = storage["camera"];
    if (!cameraNode.empty()) {
        readStringNode(cameraNode, "source", config.camera.source);
        readIntNode(cameraNode, "device_index", config.camera.deviceIndex);
        readStringNode(cameraNode, "device_serial", config.camera.deviceSerial);
        readStringNode(cameraNode, "video_path", config.camera.videoPath);
        readIntNode(cameraNode, "exposure_us", config.camera.exposureUs);
        readIntNode(cameraNode, "fps", config.camera.fps);
        readDoubleNode(cameraNode, "gain", config.camera.gain);
        if (!cameraNode["video_path"].empty() && cameraNode["source"].empty() && !config.camera.videoPath.empty()) {
            config.camera.source = "file";
        }
    }

    const cv::FileNode preprocessorNode = storage["preprocessor"];
    if (!preprocessorNode.empty()) {
        readIntNode(preprocessorNode, "output_width", config.preprocessor.outputWidth);
        readIntNode(preprocessorNode, "output_height", config.preprocessor.outputHeight);
        readDoubleNode(preprocessorNode, "center_keep_ratio", config.preprocessor.centerKeepRatio);
        readBoolNode(preprocessorNode, "enable_trail", config.preprocessor.enableTrail);
        readIntNode(preprocessorNode, "trail_brightness_threshold", config.preprocessor.trailBrightnessThreshold);
        readDoubleNode(preprocessorNode, "trail_disable_motion_ratio", config.preprocessor.trailDisableMotionRatio);
        readIntNode(preprocessorNode, "trail_reenable_frames", config.preprocessor.trailReenableFrames);
        readIntNode(preprocessorNode, "trail_history_frames", config.preprocessor.trailHistoryFrames);
    }

    const cv::FileNode encoderNode = storage["encoder"];
    if (!encoderNode.empty()) {
        readIntNode(encoderNode, "target_bytes_per_sec", config.encoder.targetBytesPerSec);
        readIntNode(encoderNode, "gop", config.encoder.gop);
        readStringNode(encoderNode, "codec_name", config.encoder.codecName);
    }

    syncDerivedConfig(config);
}

std::string findConfigPathArgument(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--config") {
            if (index + 1 >= argc) {
                throw std::runtime_error("Missing value for --config");
            }
            return argv[index + 1];
        }
    }
    return {};
}

AppConfig parseArgs(int argc, char** argv) {
    AppConfig config;
    const std::string configPath = findConfigPathArgument(argc, argv);
    if (!configPath.empty()) {
        applyConfigFile(configPath, config);
    }

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto readValue = [&](const char* name) -> std::string {
            if (index + 1 >= argc) {
                throw std::runtime_error(std::string("Missing value for ") + name);
            }
            return argv[++index];
        };

        if (argument == "--serial") {
            config.serialDevice = readValue("--serial");
        } else if (argument == "--config") {
            static_cast<void>(readValue("--config"));
        } else if (argument == "--baud") {
            if (!parseIntArgument(readValue("--baud"), config.baudRate)) {
                throw std::runtime_error("Invalid --baud value");
            }
        } else if (argument == "--source") {
            config.camera.source = readValue("--source");
        } else if (argument == "--device-index") {
            if (!parseIntArgument(readValue("--device-index"), config.camera.deviceIndex)) {
                throw std::runtime_error("Invalid --device-index value");
            }
        } else if (argument == "--device-serial") {
            config.camera.deviceSerial = readValue("--device-serial");
        } else if (argument == "--video") {
            config.camera.videoPath = readValue("--video");
            config.camera.source = "file";
        } else if (argument == "--fps") {
            if (!parseIntArgument(readValue("--fps"), config.camera.fps)) {
                throw std::runtime_error("Invalid --fps value");
            }
        } else if (argument == "--gain") {
            try {
                config.camera.gain = std::stod(readValue("--gain"));
            } catch (...) {
                throw std::runtime_error("Invalid --gain value");
            }
        } else if (argument == "--width") {
            if (!parseIntArgument(readValue("--width"), config.preprocessor.outputWidth)) {
                throw std::runtime_error("Invalid --width value");
            }
        } else if (argument == "--height") {
            if (!parseIntArgument(readValue("--height"), config.preprocessor.outputHeight)) {
                throw std::runtime_error("Invalid --height value");
            }
        } else if (argument == "--target-bytes") {
            if (!parseIntArgument(readValue("--target-bytes"), config.encoder.targetBytesPerSec)) {
                throw std::runtime_error("Invalid --target-bytes value");
            }
        } else if (argument == "--exposure-us") {
            if (!parseIntArgument(readValue("--exposure-us"), config.camera.exposureUs)) {
                throw std::runtime_error("Invalid --exposure-us value");
            }
        } else if (argument == "--max-stream-pps") {
            if (!parseIntArgument(readValue("--max-stream-pps"), config.maxStreamPacketsPerSec)) {
                throw std::runtime_error("Invalid --max-stream-pps value");
            }
        } else if (argument == "--disable-trail") {
            config.preprocessor.enableTrail = false;
        } else {
            throw std::runtime_error("Unknown argument: " + argument);
        }
    }

    syncDerivedConfig(config);
    return config;
}

std::vector<std::uint8_t> makePacket(
    hero::PacketType packetType,
    hero::CodecId codec,
    std::uint8_t flags,
    std::uint32_t seq,
    const std::uint8_t* payload,
    std::size_t payloadSize) {
    if (payloadSize > hero::kMaxPayloadSize) {
        throw std::runtime_error("Payload too large for hero packet");
    }

    hero::VideoPacketHeaderV1 header {};
    header.magic = hero::kMagic;
    header.version = hero::kVersion;
    header.packetType = static_cast<std::uint8_t>(packetType);
    header.streamId = 1;
    header.codec = static_cast<std::uint8_t>(codec);
    header.flags = flags;
    header.reserved0 = 0;
    header.seq = seq;
    header.payloadLen = static_cast<std::uint16_t>(payloadSize);
    header.packetCrc16 = 0;

    std::vector<std::uint8_t> bytes(hero::kHeaderSize + payloadSize);
    std::memcpy(bytes.data(), &header, hero::kHeaderSize);
    if (payloadSize > 0) {
        std::memcpy(bytes.data() + hero::kHeaderSize, payload, payloadSize);
    }

    std::vector<std::uint8_t> crcInput;
    crcInput.reserve(hero::kHeaderSize - sizeof(header.packetCrc16) + payloadSize);
    crcInput.insert(
        crcInput.end(),
        bytes.begin(),
        bytes.begin() + static_cast<std::ptrdiff_t>(offsetof(hero::VideoPacketHeaderV1, packetCrc16)));
    crcInput.insert(crcInput.end(), bytes.begin() + static_cast<std::ptrdiff_t>(hero::kHeaderSize), bytes.end());

    const std::uint16_t crc = crc16CcittFalse(crcInput.data(), crcInput.size());
    std::memcpy(bytes.data() + offsetof(hero::VideoPacketHeaderV1, packetCrc16), &crc, sizeof(crc));
    return bytes;
}

void sendPacket(SerialPort& port, const std::vector<std::uint8_t>& packet, Stats& stats) {
    port.writeAll(packet.data(), packet.size());
    ++stats.sentPackets;
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    try {
        const AppConfig config = parseArgs(argc, argv);

        logInfo("hero-camera-agent 启动中");
        if (!config.configPath.empty()) {
            logInfo("配置文件: " + config.configPath);
        }
        logInfo("串口设备: " + config.serialDevice + "，波特率: " + std::to_string(config.baudRate));
        std::string cameraLog = "视频源: " + config.camera.source + "，设备序号: " + std::to_string(config.camera.deviceIndex);
        if (!config.camera.deviceSerial.empty()) {
            cameraLog += "，固定序列号: " + config.camera.deviceSerial;
        }
        cameraLog += "，曝光(us): " + std::to_string(config.camera.exposureUs);
        cameraLog += "，增益: " + std::to_string(config.camera.gain);
        logInfo(cameraLog);
        logInfo("输出分辨率: " + std::to_string(config.encoder.width) + "x" + std::to_string(config.encoder.height) +
                "，帧率: " + std::to_string(config.encoder.fps) +
                "，目标码率: " + std::to_string(config.encoder.targetBytesPerSec) + " Byte/s");
        logInfo("流分片发送速率上限: " + std::to_string(config.maxStreamPacketsPerSec) + " 包/秒");
        logInfo(std::string("拖影增强: ") + (config.preprocessor.enableTrail ? "开启" : "关闭") +
                "，高亮阈值: " + std::to_string(config.preprocessor.trailBrightnessThreshold) +
                "，大运动禁用阈值: " + std::to_string(config.preprocessor.trailDisableMotionRatio) +
                "，恢复帧数: " + std::to_string(config.preprocessor.trailReenableFrames) +
                "，托影长度: " + std::to_string(config.preprocessor.trailHistoryFrames) + " 帧");

        auto camera = createCameraSource(config.camera);
        logInfo("相机源初始化完成");
        SerialPort serial(config.serialDevice, config.baudRate);
        logInfo("串口打开成功，准备向 MCU 发送数据");
        FramePreprocessor preprocessor(config.preprocessor);
        VideoEncoder encoder(config.encoder);
        logInfo("图像预处理器和编码器初始化完成");
        Stats stats;
        std::uint32_t nextSeq = 1;
        bool firstFrameLogged = false;
        bool firstEncodedLogged = false;
        std::deque<PendingStreamPacket> pendingStreamPackets;
        const std::size_t maxBufferedBytes = std::max<std::size_t>(16384, static_cast<std::size_t>(config.encoder.targetBytesPerSec) * 2U);
        const auto streamSendInterval = std::chrono::microseconds(1000000 / std::max(1, config.maxStreamPacketsPerSec));
        auto nextStreamSendAt = std::chrono::steady_clock::now();
        auto lastBacklogLogAt = nextStreamSendAt;
        auto lastConfigAt = nextStreamSendAt;
        auto lastHeartbeatAt = nextStreamSendAt;
        auto lastLogAt = nextStreamSendAt;

        const auto sendReset = [&](std::uint8_t flags) {
            const auto packet = makePacket(hero::PacketType::kReset, hero::CodecId::kH264AnnexB, flags, nextSeq++, nullptr, 0);
            sendPacket(serial, packet, stats);
            logWarn("已发送重置包，通知下游清空缓存并等待新关键帧");
        };

        const auto sendConfig = [&](bool repeat) {
            hero::VideoConfigV1 payload {};
            payload.width = static_cast<std::uint16_t>(config.encoder.width);
            payload.height = static_cast<std::uint16_t>(config.encoder.height);
            payload.fps = static_cast<std::uint16_t>(config.encoder.fps);
            payload.targetBytesPerSec = static_cast<std::uint16_t>(config.encoder.targetBytesPerSec);
            std::uint8_t flags = repeat ? hero::kFlagConfigRepeat : 0;
            const auto packet = makePacket(
                hero::PacketType::kConfig,
                hero::CodecId::kH264AnnexB,
                flags,
                nextSeq++,
                reinterpret_cast<const std::uint8_t*>(&payload),
                sizeof(payload));
            sendPacket(serial, packet, stats);
            logInfo(std::string(repeat ? "已重发配置包" : "已发送首个配置包") +
                    "，内容: " + std::to_string(payload.width) + "x" + std::to_string(payload.height) +
                    " @ " + std::to_string(payload.fps) + "fps，目标 " + std::to_string(payload.targetBytesPerSec) + " Byte/s");
        };

        const auto sendHeartbeat = [&]() {
            hero::VideoHeartbeatV1 payload {};
            payload.capturedFrames = stats.capturedFrames;
            payload.encodedFrames = stats.encodedFrames;
            payload.sentPackets = stats.sentPackets;
            payload.droppedPackets = stats.droppedPackets;
            const auto packet = makePacket(
                hero::PacketType::kHeartbeat,
                hero::CodecId::kH264AnnexB,
                0,
                nextSeq++,
                reinterpret_cast<const std::uint8_t*>(&payload),
                sizeof(payload));
            sendPacket(serial, packet, stats);
            logInfo("已发送心跳包，供 MCU 侧观察链路状态");
        };

        const auto pendingByteCount = [&]() -> std::size_t {
            std::size_t total = 0;
            for (const auto& packet : pendingStreamPackets) {
                total += packet.payload.size();
            }
            return total;
        };

        const auto resetBacklogIfNeeded = [&]() {
            if (pendingByteCount() <= maxBufferedBytes) {
                return;
            }
            pendingStreamPackets.clear();
            ++stats.backlogResetCount;
            ++stats.droppedPackets;
            logWarn("待发送码流积压过多，已清空缓冲并请求下游重新等待关键帧，当前累计重置次数: " +
                    std::to_string(stats.backlogResetCount));
            sendReset(hero::kFlagDiscontinuity);
            sendConfig(true);
        };

        const auto flushOneStreamPacket = [&]() {
            if (pendingStreamPackets.empty()) {
                return;
            }
            const auto now = std::chrono::steady_clock::now();
            if (now < nextStreamSendAt) {
                return;
            }

            std::uint8_t flags = 0;
            const auto& pending = pendingStreamPackets.front();
            if (pending.keyframeHint) {
                flags |= hero::kFlagKeyframeHint;
            }

            const auto packet = makePacket(
                hero::PacketType::kStreamChunk,
                hero::CodecId::kH264AnnexB,
                flags,
                nextSeq++,
                pending.payload.data(),
                pending.payload.size());
            sendPacket(serial, packet, stats);

            pendingStreamPackets.pop_front();
            nextStreamSendAt = now + streamSendInterval;
        };

        const auto serviceOutput = [&]() {
            const auto now = std::chrono::steady_clock::now();
            if (now - lastConfigAt >= std::chrono::seconds(1)) {
                sendConfig(true);
                lastConfigAt = now;
            }
            if (now - lastHeartbeatAt >= std::chrono::seconds(1)) {
                sendHeartbeat();
                lastHeartbeatAt = now;
            }
            resetBacklogIfNeeded();
            flushOneStreamPacket();
            if (pendingByteCount() > 0 && now - lastBacklogLogAt >= std::chrono::seconds(1)) {
                logInfo("当前待发送码流缓冲: " + std::to_string(pendingByteCount()) + " 字节");
                lastBacklogLogAt = now;
            }
        };

        sendReset(hero::kFlagDiscontinuity);
        sendConfig(false);

        while (gRunning.load()) {
            cv::Mat frame;
            std::int64_t timestampUs = 0;
            if (!camera->read(frame, timestampUs) || frame.empty()) {
                ++stats.readFailures;
                if (stats.readFailures <= 3 || stats.readFailures % 200 == 0) {
                    logWarn("读取相机画面失败，累计失败次数: " + std::to_string(stats.readFailures));
                }
                serviceOutput();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }

            ++stats.capturedFrames;
            if (!firstFrameLogged) {
                logInfo("已成功读取第一帧原始画面，尺寸: " + std::to_string(frame.cols) + "x" + std::to_string(frame.rows) +
                        "，时间戳(us): " + std::to_string(timestampUs));
                firstFrameLogged = true;
            }
            cv::Mat processed = preprocessor.process(frame);

            encoder.encode(processed, [&](const std::uint8_t* encoded, std::size_t size, bool keyframe) {
                ++stats.encodedFrames;
                if (!firstEncodedLogged) {
                    logInfo("已编码出第一段视频码流，长度: " + std::to_string(size) + " 字节");
                    firstEncodedLogged = true;
                }
                if (keyframe) {
                    ++stats.keyframePackets;
                    if (stats.keyframePackets <= 3 || stats.keyframePackets % 30 == 0) {
                        logInfo("检测到关键帧，编码输出长度: " + std::to_string(size) + " 字节");
                    }
                }

                for (std::size_t offset = 0; offset < size; offset += hero::kMaxPayloadSize) {
                    const std::size_t chunkSize = std::min<std::size_t>(hero::kMaxPayloadSize, size - offset);
                    PendingStreamPacket packet;
                    packet.keyframeHint = keyframe;
                    packet.payload.insert(
                        packet.payload.end(),
                        encoded + static_cast<std::ptrdiff_t>(offset),
                        encoded + static_cast<std::ptrdiff_t>(offset + chunkSize));
                    pendingStreamPackets.push_back(std::move(packet));
                }
            });

            serviceOutput();

            const auto now = std::chrono::steady_clock::now();
            if (now - lastLogAt >= std::chrono::seconds(2)) {
                logInfo("运行统计: 已采集 " + std::to_string(stats.capturedFrames) +
                        " 帧，已编码 " + std::to_string(stats.encodedFrames) +
                        " 段，已发送 " + std::to_string(stats.sentPackets) +
                        " 包，读帧失败 " + std::to_string(stats.readFailures) +
                        " 次，缓冲重置 " + std::to_string(stats.backlogResetCount) +
                        " 次");
                lastLogAt = now;
            }
        }

        encoder.flush([&](const std::uint8_t* encoded, std::size_t size, bool keyframe) {
            for (std::size_t offset = 0; offset < size; offset += hero::kMaxPayloadSize) {
                const std::size_t chunkSize = std::min<std::size_t>(hero::kMaxPayloadSize, size - offset);
                PendingStreamPacket packet;
                packet.keyframeHint = keyframe;
                packet.payload.insert(
                    packet.payload.end(),
                    encoded + static_cast<std::ptrdiff_t>(offset),
                    encoded + static_cast<std::ptrdiff_t>(offset + chunkSize));
                pendingStreamPackets.push_back(std::move(packet));
            }
        });

        while (!pendingStreamPackets.empty()) {
            serviceOutput();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        sendReset(hero::kFlagDiscontinuity);
        logInfo("hero-camera-agent 正常退出");
        return 0;
    } catch (const std::exception& exception) {
        logError(std::string("hero-camera-agent 发生异常: ") + exception.what());
        return 1;
    }
}
