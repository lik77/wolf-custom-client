#include "mqtt_publisher.h"

#include <stdexcept>

#include "custom_control.pb.h"

MqttPublisher::MqttPublisher(std::string host, int port, std::string clientId, std::string topic)
    : uri_("tcp://" + host + ':' + std::to_string(port)),
      topic_(std::move(topic)),
      client_(std::make_unique<mqtt::async_client>(uri_, std::move(clientId))) {
    client_->set_callback(*this);
    connectOptions_.set_clean_session(true);
    connectOptions_.set_automatic_reconnect(true);
}

MqttPublisher::~MqttPublisher() {
    try {
        if (client_ && connected_) {
            client_->disconnect()->wait();
        }
    } catch (...) {
    }
}

void MqttPublisher::connect() {
    try {
        client_->connect(connectOptions_)->wait();
        connected_ = true;
    } catch (const mqtt::exception& exception) {
        throw std::runtime_error("MQTT connect failed: " + std::string(exception.what()));
    }
}

void MqttPublisher::publishCustomControl(const std::vector<std::uint8_t>& payload) {
    if (!connected_) {
        throw std::runtime_error("MQTT is not connected");
    }

    rmproto::CustomControl message;
    if (!payload.empty()) {
        message.set_data(payload.data(), payload.size());
    } else {
        message.set_data("", 0);
    }

    std::string bytes;
    message.SerializeToString(&bytes);

    try {
        auto mqttMessage = mqtt::make_message(topic_, bytes);
        mqttMessage->set_qos(1);
        client_->publish(mqttMessage);
    } catch (const mqtt::exception& exception) {
        connected_ = false;
        throw std::runtime_error("MQTT publish failed: " + std::string(exception.what()));
    }
}

bool MqttPublisher::isConnected() const {
    return connected_;
}

void MqttPublisher::connected(const std::string&) {
    connected_ = true;
}

void MqttPublisher::connection_lost(const std::string&) {
    connected_ = false;
}

void MqttPublisher::message_arrived(mqtt::const_message_ptr) {}

void MqttPublisher::delivery_complete(mqtt::delivery_token_ptr) {}
