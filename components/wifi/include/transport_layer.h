#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define BUF_SIZE 128
#define PACKETS_TO_SEND 1
#define DRONE_ADDR "192.168.4.1"
#define REMOTE_ADDR "192.168.4.2"
#define REMOTE_TCP_PORT 50000
#define REMOTE_UDP_PORT 50001
#define DRONE_TCP_PORT 50000
#define DRONE_UDP_PORT 50001


#pragma pack(push, 1) 

typedef struct {
    uint8_t magic0 ;
    uint8_t magic1 ;
    uint8_t version ;
    uint8_t msg_type; //DR_UDP_SETPOINT or DR_UDP_HEARTBEAT
    uint16_t seq_be; //sequence number of a packet, network order
    uint16_t payload_len_be ;
    uint16_t session_id_be ;
    uint32_t tx_time_ms_be ;
    uint32_t crc32_be ; //used to detect accidental corrupt of data
} dr_udp_hdr_v1_t ;
#pragma pack(pop)


#pragma pack(push, 1)

typedef struct {
    uint32_t q_w_be ; // float32 IEEE-754 representend as an uint32_t (method from Beej's network prog)
    uint32_t q_x_be ;
    uint32_t q_y_be ;
    uint32_t q_z_be ;
    uint32_t throttle_be ;
    uint8_t flight_profile ;
}udp_set_point_v1_t ;

#pragma pack(pop)


typedef struct {
    bool link_alive ;
    bool tcp_session_valid ;
    bool armed ;
    bool failsafe_active ;

    uint32_t current_session_id ;
    uint32_t last_udp_rx_ms ;
    uint16_t last_udp_seq ;

    float last_roll_cmd ;
    float last_pitch_cmd ;
    float last_yaw_rate_cmd ;
    float last_throttle_cmd ;
} drone_net_ctx_t ; //runtime context

#pragma pack(push,1)
typedef struct{
    uint8_t msg_type ; //1 for setpoint, 2 for heartbeat 
    udp_set_point_v1_t set_point ;
} udp_msg_t ;
#pragma pack(pop)



extern SemaphoreHandle_t sem ;
extern SemaphoreHandle_t arm_seq_sem ;

void transport_task(void *pvParameters);
ssize_t recvn(int8_t sock, void *buffer, size_t count) ;//I will use this function to make sure that I receive the desired number of bytes on my stream sockets
ssize_t sendn(int8_t sock, void *buffer, size_t count) ;//Same
//The recvn and sendn function will be useful in case of a failed send or rec system call and they will handle the return error
//and implement a restart mechanism.
