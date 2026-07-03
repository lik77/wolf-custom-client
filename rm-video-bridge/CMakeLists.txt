cmake_minimum_required(VERSION 3.16)

project(rm-video-bridge C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(rm-video-bridge STATIC
    Src/bridge.c
    Src/crc16.c
    Src/bridge_platform_stub.c)

target_include_directories(rm-video-bridge PUBLIC Inc)

add_executable(rm-video-bridge-demo Src/main_template.c)
target_link_libraries(rm-video-bridge-demo PRIVATE rm-video-bridge)
