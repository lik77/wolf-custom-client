#include "mqtt_client.h"

#include <QtCore/QMetaObject>
#include <QtCore/QTimer>

#include <cstdint>

#include "custom_client.pb.h"

namespace {

QString stageName(std::uint32_t stage) {
    switch (stage) {
        case 0:
            return QStringLiteral("未开始");
        case 1:
            return QStringLiteral("准备阶段");
        case 2:
            return QStringLiteral("自检阶段");
        case 3:
            return QStringLiteral("倒计时");
        case 4:
            return QStringLiteral("比赛中");
        case 5:
            return QStringLiteral("结算中");
        default:
            return QStringLiteral("未知");
    }
}

std::string toStdBinary(const QByteArray& bytes) {
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

}  // namespace

MqttClient::MqttClient(QObject* parent) : QObject(parent) {}

MqttClient::~MqttClient() {
    try {
        if (client_ && connected_) {
            client_->disconnect()->wait();
        }
    } catch (...) {
    }
}

void MqttClient::connectToBroker(const QString& host, int port, const QString& clientId) {
    host_ = host;
    port_ = port;
    clientId_ = clientId;

    const std::string uri = QStringLiteral("tcp://%1:%2").arg(host_).arg(port_).toStdString();
    client_ = std::make_unique<mqtt::async_client>(uri, clientId_.toStdString());
    client_->set_callback(*this);

    connectOptions_.set_clean_session(true);
    connectOptions_.set_automatic_reconnect(true);

    emit connectionStateChanged(QStringLiteral("MQTT 正在连接: %1").arg(QString::fromStdString(uri)));
    emit logMessage(QStringLiteral("[信息] 正在连接 MQTT 服务器，clientID=%1").arg(clientId_));
    try {
        client_->connect(connectOptions_)->wait();
    } catch (const mqtt::exception& exception) {
        emit logMessage(QStringLiteral("[错误] MQTT 连接失败: %1").arg(QString::fromStdString(exception.what())));
    }
}

void MqttClient::publishControlFrame(const ControlState& state) {
    if (!connected_ || !client_) {
        static int skippedCount = 0;
        ++skippedCount;
        if (skippedCount <= 3 || skippedCount % 100 == 0) {
            emit logMessage(QStringLiteral("[警告] MQTT 未连接，键鼠控制帧未发送"));
        }
        return;
    }

    rmproto::KeyboardMouseControl message;
    message.set_mouse_x(state.mouseX);
    message.set_mouse_y(state.mouseY);
    message.set_mouse_z(state.mouseZ);
    message.set_left_button_down(state.leftButtonDown);
    message.set_right_button_down(state.rightButtonDown);
    message.set_mid_button_down(state.midButtonDown);
    message.set_keyboard_value(state.keyboardMask);

    std::string payload;
    message.SerializeToString(&payload);
    publishBinary("KeyboardMouseControl", payload);
}

void MqttClient::publishCustomControl(const QByteArray& payload) {
    if (!connected_ || !client_) {
        emit logMessage(QStringLiteral("[警告] MQTT 未连接，自定义控制数据未发送"));
        return;
    }

    QByteArray trimmed = payload.left(30);
    rmproto::CustomControl message;
    message.set_data(toStdBinary(trimmed));
    std::string bytes;
    message.SerializeToString(&bytes);
    publishBinary("CustomControl", bytes);
    emit logMessage(QStringLiteral("[信息] 已发送 CustomControl，自定义数据长度 %1 字节").arg(trimmed.size()));
}

void MqttClient::connected(const std::string& cause) {
    QMetaObject::invokeMethod(this, [this, cause]() {
        connected_ = true;
        emit connectionStateChanged(QStringLiteral("MQTT 已连接: %1").arg(QString::fromStdString(cause)));
        emit logMessage(QStringLiteral("[信息] MQTT 连接成功，开始订阅官方 Topic"));
        subscribeAll();
    }, Qt::QueuedConnection);
}

void MqttClient::connection_lost(const std::string& cause) {
    QMetaObject::invokeMethod(this, [this, cause]() {
        connected_ = false;
        emit connectionStateChanged(QStringLiteral("MQTT 已断开: %1").arg(QString::fromStdString(cause)));
        emit logMessage(QStringLiteral("[错误] MQTT 连接丢失: %1").arg(QString::fromStdString(cause)));
    }, Qt::QueuedConnection);
}

void MqttClient::message_arrived(mqtt::const_message_ptr msg) {
    const std::string topic = msg->get_topic();
    const auto payloadRef = msg->get_payload_ref();
    const QByteArray payload(payloadRef.data(), static_cast<int>(payloadRef.size()));

    QMetaObject::invokeMethod(this, [this, topic, payload]() {
        if (topic == "CustomByteBlock") {
            rmproto::CustomByteBlock message;
            if (message.ParseFromArray(payload.constData(), payload.size())) {
                static int customBlockCount = 0;
                ++customBlockCount;
                if (customBlockCount == 1 || customBlockCount % 100 == 0) {
                    emit logMessage(QStringLiteral("[信息] 已收到 CustomByteBlock，累计 %1 次，负载长度 %2 字节")
                                        .arg(customBlockCount)
                                        .arg(message.data().size()));
                }
                emit customByteBlockReceived(QByteArray::fromStdString(message.data()));
            } else {
                emit logMessage(QStringLiteral("[错误] CustomByteBlock Protobuf 反序列化失败"));
            }
            return;
        }

        if (topic == "GameStatus") {
            rmproto::GameStatus message;
            if (message.ParseFromArray(payload.constData(), payload.size())) {
                emit gameStatusUpdated(
                    QStringLiteral("Round %1/%2  Score %3:%4  Stage %5  T-%6s%7")
                        .arg(message.current_round())
                        .arg(message.total_rounds())
                        .arg(message.red_score())
                        .arg(message.blue_score())
                        .arg(stageName(message.current_stage()))
                        .arg(message.stage_countdown_sec())
                        .arg(message.is_paused() ? QStringLiteral("  [暂停]") : QString()));
            }
            return;
        }

        if (topic == "RobotDynamicStatus") {
            rmproto::RobotDynamicStatus message;
            if (message.ParseFromArray(payload.constData(), payload.size())) {
                emit dynamicStatusUpdated(
                    QStringLiteral("HP %1  Heat %2  Ammo %3  Chassis %4  OOC %5")
                        .arg(message.current_health())
                        .arg(message.current_heat(), 0, 'f', 1)
                        .arg(message.remaining_ammo())
                        .arg(message.current_chassis_energy())
                        .arg(message.is_out_of_combat() ? QStringLiteral("Y") : QStringLiteral("N")));
            }
            return;
        }

        if (topic == "RobotModuleStatus") {
            rmproto::RobotModuleStatus message;
            if (message.ParseFromArray(payload.constData(), payload.size())) {
                emit moduleStatusUpdated(
                    QStringLiteral("Video %1  Main %2  UWB %3  Laser %4")
                        .arg(message.video_transmission())
                        .arg(message.main_controller())
                        .arg(message.uwb())
                        .arg(message.laser_detection_module()));
            }
            return;
        }

        if (topic == "RobotPosition") {
            rmproto::RobotPosition message;
            if (message.ParseFromArray(payload.constData(), payload.size())) {
                emit positionStatusUpdated(
                    QStringLiteral("Pos (%1, %2, %3)  yaw %4  robot %5")
                        .arg(message.x(), 0, 'f', 2)
                        .arg(message.y(), 0, 'f', 2)
                        .arg(message.z(), 0, 'f', 2)
                        .arg(message.yaw(), 0, 'f', 1)
                        .arg(message.robot_id()));
            }
        }
    }, Qt::QueuedConnection);
}

void MqttClient::delivery_complete(mqtt::delivery_token_ptr) {}

void MqttClient::subscribeAll() {
    if (!client_) {
        return;
    }
    try {
        client_->subscribe("CustomByteBlock", 1)->wait();
        client_->subscribe("GameStatus", 1)->wait();
        client_->subscribe("RobotDynamicStatus", 1)->wait();
        client_->subscribe("RobotModuleStatus", 1)->wait();
        client_->subscribe("RobotPosition", 1)->wait();
        emit logMessage(QStringLiteral("[信息] MQTT Topic 订阅完成：CustomByteBlock / GameStatus / RobotDynamicStatus / RobotModuleStatus / RobotPosition"));
    } catch (const mqtt::exception& exception) {
        emit logMessage(QStringLiteral("[错误] MQTT 订阅失败: %1").arg(QString::fromStdString(exception.what())));
    }
}

void MqttClient::publishBinary(const std::string& topic, const std::string& payload) {
    try {
        auto message = mqtt::make_message(topic, payload);
        message->set_qos(1);
        client_->publish(message);
    } catch (const mqtt::exception& exception) {
        emit logMessage(QStringLiteral("[错误] MQTT 发布失败，Topic=%1，原因=%2")
                            .arg(QString::fromStdString(topic))
                            .arg(QString::fromStdString(exception.what())));
    }
}
