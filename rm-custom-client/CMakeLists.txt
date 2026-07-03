cmake_minimum_required(VERSION 3.16)

project(rm-custom-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)

find_library(PAHO_MQTT_CPP_LIBRARY paho-mqttpp3 REQUIRED)
find_library(PAHO_MQTT_C_LIBRARY paho-mqtt3as REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/custom_client.proto)

add_executable(rm-custom-client
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    src/custom_stream_handler.cpp
    src/hero_protocol.cpp
    src/main.cpp
    src/main_window.cpp
    src/mqtt_client.cpp
    src/official_hevc_normalizer.cpp
    src/official_udp_receiver.cpp
    src/video_decoder.cpp)

target_include_directories(rm-custom-client PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(rm-custom-client
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        protobuf::libprotobuf
        PkgConfig::FFMPEG
        ${PAHO_MQTT_CPP_LIBRARY}
        ${PAHO_MQTT_C_LIBRARY})
