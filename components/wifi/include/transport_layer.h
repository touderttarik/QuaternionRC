#pragma once

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "string.h"
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "freertos/semphr.h"


#define BUF_SIZE 128
#define PACKETS_TO_SEND 1
#define DRONE_ADDR "192.168.4.1"
#define REMOTE_ADDR "192.168.4.2"
#define REMOTE_TCP_PORT 50000
#define REMOTE_UDP_PORT 50001
#define DRONE_TCP_PORT 50000
#define DRONE_UDP_PORT 50001





extern SemaphoreHandle_t sem ;
extern SemaphoreHandle_t arm_seq_sem ;

void transport_task(void *pvParameters);

/*

ssize_t recvn(int8_t sock, void *buffer, size_t count) ;//I will use this function to make sure that I receive the desired number of bytes on my stream sockets
ssize_t sendn(int8_t sock, void *buffer, size_t count) ;//Same
//The recvn and sendn function will be useful in case of a failed send or rec system call and they will handle the return error
//and implement a restart mechanism.

uint32_t htonf(float* f) ;
float ntohf(uint32_t* u) ;

struct joystick_values_t ;
struct udp_set_point_v1_t ;

void htonsp(joystick_values_t* hsp, udp_set_point_v1_t* nsp); //host to network setpoint
void ntohsp(); //network to host setpoint

*/