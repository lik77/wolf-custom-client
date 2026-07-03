cmake_minimum_required(VERSION 3.16)

project(custom-control-usb-bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Protobuf REQUIRED)

find_library(PAHO_MQTT_CPP_LIBRARY paho-mqttpp3 REQUIRED)
find_library(PAHO_MQTT_C_LIBRARY paho-mqtt3as REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/custom_control.proto)

add_executable(custom-control-usb-bridge
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    src/crc.cpp
    src/frame_parser.cpp
    src/main.cpp
    src/mqtt_publisher.cpp
    src/serial_port.cpp)

target_include_directories(custom-control-usb-bridge PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(custom-control-usb-bridge
    PRIVATE
        protobuf::libprotobuf
        ${PAHO_MQTT_CPP_LIBRARY}
        ${PAHO_MQTT_C_LIBRARY})
