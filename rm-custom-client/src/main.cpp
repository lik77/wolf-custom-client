#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QThread>
#include <QtWidgets/QApplication>

#include <google/protobuf/stubs/common.h>

extern "C" {
#include <libavutil/log.h>
}

#include "control_state.h"
#include "custom_stream_handler.h"
#include "main_window.h"
#include "mqtt_client.h"
#include "official_hevc_normalizer.h"
#include "official_udp_receiver.h"
#include "video_decoder.h"

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    qRegisterMetaType<ControlState>("ControlState");

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("rm-custom-client"));
    av_log_set_level(AV_LOG_ERROR);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("mqtt-host"), QStringLiteral("MQTT host"), QStringLiteral("host"), QStringLiteral("192.168.12.1")});
    parser.addOption({QStringLiteral("mqtt-port"), QStringLiteral("MQTT port"), QStringLiteral("port"), QStringLiteral("3333")});
    parser.addOption({QStringLiteral("udp-port"), QStringLiteral("Official video UDP port"), QStringLiteral("port"), QStringLiteral("3334")});
    parser.addOption({QStringLiteral("robot-id"), QStringLiteral("Robot/client ID"), QStringLiteral("id"), QStringLiteral("1")});
    parser.process(app);

    MainWindow window;
    MqttClient mqttClient;
    OfficialUdpReceiver udpReceiver;
    CustomStreamHandler customStreamHandler;
    QThread officialVideoThread;
    auto* officialNormalizer = new OfficialHevcNormalizer;
    auto* officialHevcDecoder = new VideoDecoder(AV_CODEC_ID_HEVC);
    VideoDecoder heroDecoder(AV_CODEC_ID_H264);

    officialNormalizer->moveToThread(&officialVideoThread);
    officialHevcDecoder->moveToThread(&officialVideoThread);
    QObject::connect(&officialVideoThread, &QThread::finished, officialNormalizer, &QObject::deleteLater);
    QObject::connect(&officialVideoThread, &QThread::finished, officialHevcDecoder, &QObject::deleteLater);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&officialVideoThread]() {
        officialVideoThread.quit();
        officialVideoThread.wait();
    });
    officialVideoThread.start();

    QObject::connect(&udpReceiver, &OfficialUdpReceiver::hevcFrameReady, officialNormalizer, &OfficialHevcNormalizer::handleFrame, Qt::QueuedConnection);
    QObject::connect(officialNormalizer, &OfficialHevcNormalizer::normalizedHevcFrameReady, officialHevcDecoder, &VideoDecoder::feedBytes, Qt::QueuedConnection);
    QObject::connect(officialHevcDecoder, &VideoDecoder::frameDecoded, &window, &MainWindow::updateOfficialFrame, Qt::QueuedConnection);
    QObject::connect(officialHevcDecoder, &VideoDecoder::logMessage, &window, &MainWindow::appendLog, Qt::QueuedConnection);
    QObject::connect(&udpReceiver, &OfficialUdpReceiver::logMessage, &window, &MainWindow::appendLog);
    QObject::connect(officialNormalizer, &OfficialHevcNormalizer::logMessage, &window, &MainWindow::appendLog, Qt::QueuedConnection);

    QObject::connect(&mqttClient, &MqttClient::customByteBlockReceived, &customStreamHandler, &CustomStreamHandler::handleCustomByteBlock);
    QObject::connect(&customStreamHandler, &CustomStreamHandler::heroChunkReady, &heroDecoder, &VideoDecoder::feedBytes);
    QObject::connect(&customStreamHandler, &CustomStreamHandler::heroResetRequired, &heroDecoder, &VideoDecoder::reset);
    QObject::connect(&customStreamHandler, &CustomStreamHandler::heroConfigSummary, &window, &MainWindow::updateHeroConfigStatus);
    QObject::connect(&customStreamHandler, &CustomStreamHandler::logMessage, &window, &MainWindow::appendLog);
    QObject::connect(&heroDecoder, &VideoDecoder::frameDecoded, &window, &MainWindow::updateHeroFrame);
    QObject::connect(&heroDecoder, &VideoDecoder::logMessage, &window, &MainWindow::appendLog);

    QObject::connect(&mqttClient, &MqttClient::gameStatusUpdated, &window, &MainWindow::updateGameStatus);
    QObject::connect(&mqttClient, &MqttClient::dynamicStatusUpdated, &window, &MainWindow::updateDynamicStatus);
    QObject::connect(&mqttClient, &MqttClient::moduleStatusUpdated, &window, &MainWindow::updateModuleStatus);
    QObject::connect(&mqttClient, &MqttClient::positionStatusUpdated, &window, &MainWindow::updatePositionStatus);
    QObject::connect(&mqttClient, &MqttClient::connectionStateChanged, &window, &MainWindow::updateConnectionStatus);
    QObject::connect(&mqttClient, &MqttClient::logMessage, &window, &MainWindow::appendLog);

    QObject::connect(&window, &MainWindow::controlFrameReady, &mqttClient, &MqttClient::publishControlFrame);
    QObject::connect(&window, &MainWindow::customControlRequested, &mqttClient, &MqttClient::publishCustomControl);

    const quint16 udpPort = parser.value(QStringLiteral("udp-port")).toUShort();
    udpReceiver.start(udpPort);
    mqttClient.connectToBroker(
        parser.value(QStringLiteral("mqtt-host")),
        parser.value(QStringLiteral("mqtt-port")).toInt(),
        parser.value(QStringLiteral("robot-id")));

    window.show();
    const int result = app.exec();
    google::protobuf::ShutdownProtobufLibrary();
    return result;
}
