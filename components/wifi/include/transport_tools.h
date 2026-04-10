#ifndef TRANSPORT_NET_TOOLS_H
#define TRANSPORT_NET_TOOLS_H

#include <sys/types.h>
#include "transport_types.h"
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

ssize_t recvn(int8_t sock, void *buffer, size_t count) ;//I will use this function to make sure that I receive the desired number of bytes on my stream sockets
ssize_t sendn(int8_t sock, void *buffer, size_t count) ;//Same
//The recvn and sendn function will be useful in case of a failed send or recv system call and they will handle the return error
//and implement a restart mechanism.

uint32_t htonf(float* f) ;
float ntohf(uint32_t* u) ;
void htonsp(udp_set_point_v1_t* sp, quat_t *q_sp); //host to network setpoint
void ntohsp(udp_set_point_v1_t* sp, quat_t *q_sp); //network to host setpoint



#endif
