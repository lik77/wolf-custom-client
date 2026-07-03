cmake_minimum_required(VERSION 3.16)

project(hero-camera-agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)

set(MVS_SDK_ROOT "$ENV{MVS_SDK_ROOT}" CACHE PATH "Root path of Hikrobot MVS SDK")
find_path(MVS_INCLUDE_DIR MvCameraControl.h HINTS
    "${MVS_SDK_ROOT}/include"
    "/opt/MVS/include")
find_library(MVS_LIBRARY MvCameraControl HINTS
    "${MVS_SDK_ROOT}/lib/64"
    "/opt/MVS/lib/64")

add_executable(hero-camera-agent
    src/camera_source.cpp
    src/crc16.cpp
    src/frame_preprocessor.cpp
    src/main.cpp
    src/serial_port.cpp
    src/video_encoder.cpp)

target_include_directories(hero-camera-agent PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(hero-camera-agent PRIVATE ${OpenCV_LIBS} PkgConfig::FFMPEG)

if(MVS_INCLUDE_DIR AND MVS_LIBRARY)
    target_compile_definitions(hero-camera-agent PRIVATE WITH_MVS_SDK=1)
    target_include_directories(hero-camera-agent PRIVATE "${MVS_INCLUDE_DIR}")
    target_link_libraries(hero-camera-agent PRIVATE "${MVS_LIBRARY}")
else()
    message(WARNING "MVS SDK not found. Build will only support OpenCV/video-file sources.")
endif()
