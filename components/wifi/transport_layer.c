#include "transport_layer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "string.h"
#include <unistd.h>
#include "joystick_input.h"
#define TAG "Remote Transport"


uint32_t htonf(float* f) {
    uint32_t result = 0 ;
    unsigned char* p = (unsigned char*) f;
    result = *p<<24 | *(p+1)<<16 | *(p+2)<<8 | *(p+3);
    return result ;
}

float ntohf(uint32_t* u) {
    float* f = NULL;
    unsigned char* pu = (unsigned char*) u ;
    unsigned char* pf = (unsigned char*) f ;
    *pf = *(pu+3) ;
    *(pf+1) = *(pu+2) ;
    *(pf+2) = *(pu+1) ;
    *(pf+3) = *pu ;
    return *f ;
}



ssize_t recvn(int8_t sock, void *buffer, size_t count) {
    ssize_t num_read ; //# of bytes fetched by last read()
    size_t tot_read ; //total # of bytes read so far
    char *buf ;
    buf = buffer ;

    for(tot_read=0; tot_read<count;){
        num_read = read(sock, buf, count - tot_read);

        if(num_read == 0){ // EOF
            return tot_read ; //May be 0 if this is the first read
        }
        if(num_read == -1){
            if(errno == EINTR){
                continue ; //Interrupted --> restard read()
            }
            else{
                return -1 ; //Some other error
            }
        }
        tot_read ++;
        buf += num_read ;
    }
    return tot_read ;
}
ssize_t sendn(int8_t sock, void *buffer, size_t count) {
    ssize_t num_written ;
    size_t tot_written ;
    char *buf = buffer ;

    buf = buffer ;

    for(tot_written=0; tot_written<count;){
        num_written = write(sock, buf, count);
        
        if(num_written <= 0){
            if(num_written == -1 && errno == EINTR){
                continue ;//Interrupted --> restard write()
            }
            else{
                return -1 ;//Some other error
            }
        }
        
        tot_written ++ ;
        buf += num_written;
    }
    return tot_written ;
}


void transport_task(void *pvParameters) {
    
    (void)pvParameters;
    char *payload = "Hello Drone !";
    char recvbuf[BUF_SIZE];
    drone_net_ctx_t context ;
    udp_msg_t msg ;
    msg.msg_type = 1 ;
    //See later for heartbeat
    //I'll just try to send some data 
    msg.set_point.flight_profile = 1 ;
    msg.set_point.q_w_be = htonl(0x0102);
    msg.set_point.q_x_be = htonl(0x0304);
    msg.set_point.q_y_be = htonl(0x0506);
    msg.set_point.q_z_be = htonl(0x0708);

    // local_addr: "My local endpo int8_t" -> the local IP/port the tcp_socket is bound to.
    // In TCP server: used by bind() + listen() to choose the listening port.   
    // In UDP: used by bind() to choose the receive port.
    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET ;
    local_addr.sin_addr.s_addr = inet_addr(REMOTE_ADDR);
    local_addr.sin_port = htons(REMOTE_TCP_PORT) ;


    // peer_addr: "Remote endpoint8_t (destination)" -> the remote IP/port we want to reach.
    // In TCP client: passed to connect() to establish the connection.
    // In UDP: passed to sendto() as the destination.
    struct sockaddr_in peer_addr = {0}; //Not useful in the case of the server (Drone)

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(DRONE_ADDR);
    peer_addr.sin_port = htons(DRONE_TCP_PORT);
    // from_addr: "Remote end point8_t (who connected / who sent)" -> output filled by the stack.
    // In TCP server: accept() can fill it with the client's IP/port (who connected).
    // In UDP: recvfrom() fills it with the sender's IP/port (who sent this datagram).
    // Note: after TCP is connected, you usually don't need this struct for each recv()/send().
    struct sockaddr_in from_addr = {0};
    
    int error_code ;    
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    socklen_t local_addr_len = sizeof(local_addr)  ;
    socklen_t peer_addr_len = sizeof(peer_addr) ;

    ESP_LOGI(TAG, "Creating socket connection with the Drone.");
    error_code = connect(tcp_sock,(struct sockaddr *)&peer_addr, peer_addr_len) ;
    if(error_code < 0){
        ESP_LOGE(TAG,"connect() returned errno=%d", errno);
    }
    else{
        ESP_LOGI(TAG,"connect() function returned success.") ;
    }
    
    ESP_LOGI(TAG,"Sending Hello Drone.");
    error_code = sendn(tcp_sock, payload, strlen(payload));
    if(error_code == 13){
        ESP_LOGI(TAG, "Total number of bytes written : %d", error_code) ;
    }
    else{
        ESP_LOGE(TAG, "Sending Hello Drone failed. errno = %d", errno) ;
    }

    ESP_LOGI(TAG,"Waiting for Hello Remote.") ;

    error_code = recvn(tcp_sock, recvbuf, 14) ;

    if(error_code == 14 && strncmp(recvbuf, "Hello Remote !", 14)==0){
        ESP_LOGI(TAG, "Hello Remote received ! May now start controlling the Drone.") ;
    }
    else{
        ESP_LOGE(TAG, "Reception of Hello Remote failed. errorcode= %d, errno = %d", error_code, errno) ;
        for(uint8_t i=0; i<error_code; i++){
            ESP_LOGE(TAG,"recvbuf[%d]= %c", i, recvbuf[i]);
        }
    }
    //loop waiting for arm command
    ESP_LOGI(TAG,"Arming the ESCs...");
    
    fd_set readfds ; //File descriptor set used to poll the tcp_socket for possible input(if input is possible from the socket)
    struct timeval timeout = {0} ;
    FD_ZERO(&readfds) ;
    
    int num_ready ; //number of ready file descriptors for input

    for(;;){
        //read_joysticks() ;
        //calculate_setpoint() ;
        FD_SET(tcp_sock, &readfds); //Since the select syscall modifies the sets of file descriptors
        //it is necessary to reinitialize the sets to the file descriptors of interest.    
        num_ready=select(tcp_sock + 1, &readfds, NULL, NULL, &timeout) ;
        if(num_ready==-1){ 
            //ESP_LOGE(TAG,"Error during select() call on tcp socket. errno=%d", errno) ;
        }
        if(num_ready==1 && FD_ISSET(tcp_sock, &readfds)){
           // ESP_LOGI(TAG,"tcp socket ready for input operation. errno=%d", errno) ;
            //calculate the quaternion
            //send it via dgram socket
            //error_code = readn();
            //read the telemetry data and implement telemetry logic
        }
        else{
            //calculate the quaternion
            //send it via dgram socket
        }

        //!!Put a vtaskdelay at the end!!
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if(udp_sock > 0){
        close(udp_sock);
    }
    if(tcp_sock > 0){
        close(tcp_sock);
    }

    vTaskDelete(NULL);
}
