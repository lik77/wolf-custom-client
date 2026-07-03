#ifndef HERO_PROTOCOL_H
#define HERO_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HERO_MAGIC 0x4856u
#define HERO_VERSION 1u
#define HERO_HEADER_SIZE 16u
#define HERO_MAX_PACKET_SIZE 300u
#define HERO_MAX_PAYLOAD_SIZE (HERO_MAX_PACKET_SIZE - HERO_HEADER_SIZE)

typedef enum {
    HERO_PACKET_CONFIG = 1,
    HERO_PACKET_STREAM_CHUNK = 2,
    HERO_PACKET_HEARTBEAT = 3,
    HERO_PACKET_RESET = 4,
} hero_packet_type_t;

typedef enum {
    HERO_CODEC_H264_ANNEXB = 1,
    HERO_CODEC_HEVC_ANNEXB = 2,
    HERO_CODEC_MJPEG = 3,
} hero_codec_id_t;

enum {
    HERO_FLAG_KEYFRAME_HINT = 1u << 0,
    HERO_FLAG_CONFIG_REPEAT = 1u << 1,
    HERO_FLAG_DISCONTINUITY = 1u << 2,
};

#pragma pack(push, 1)
typedef struct {
    uint16_t magic;
    uint8_t version;
    uint8_t packet_type;
    uint8_t stream_id;
    uint8_t codec;
    uint8_t flags;
    uint8_t reserved0;
    uint32_t seq;
    uint16_t payload_len;
    uint16_t packet_crc16;
} hero_video_packet_header_v1_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t fps;
    uint16_t target_bytes_per_sec;
} hero_video_config_v1_t;

typedef struct {
    uint32_t captured_frames;
    uint32_t encoded_frames;
    uint32_t sent_packets;
    uint32_t dropped_packets;
} hero_video_heartbeat_v1_t;
#pragma pack(pop)

static inline bool hero_header_looks_valid(const hero_video_packet_header_v1_t* header) {
    return header->magic == HERO_MAGIC &&
           header->version == HERO_VERSION &&
           header->payload_len <= HERO_MAX_PAYLOAD_SIZE;
}

#endif
