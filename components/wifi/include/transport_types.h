#ifndef TRANSPORT_TYPES_H
#define TRANSPORT_TYPES_H

#include "types.h"

#pragma pack(push, 1)
typedef struct {
    uint8_t magic0;
    uint8_t magic1;
    uint8_t version;
    uint8_t msg_type;
    uint16_t seq_be;
    uint16_t payload_len_be;
    uint16_t session_id_be;
    uint32_t tx_time_ms_be;
    uint32_t crc32_be;
} dr_udp_hdr_v1_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint32_t q_w_be;
    uint32_t q_x_be;
    uint32_t q_y_be;
    uint32_t q_z_be;
    uint32_t throttle_be;
    uint8_t flight_profile;
} udp_set_point_v1_t;
#pragma pack(pop)

typedef struct {
    bool link_alive;
    bool tcp_session_valid;
    bool armed;
    bool failsafe_active;

    uint32_t current_session_id;
    uint32_t last_udp_rx_ms;
    uint16_t last_udp_seq;

    float last_roll_cmd;
    float last_pitch_cmd;
    float last_yaw_rate_cmd;
    float last_throttle_cmd;
} drone_net_ctx_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t msg_type;
    udp_set_point_v1_t set_point;
} udp_msg_t;
#pragma pack(pop)

#endif
