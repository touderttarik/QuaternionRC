#include "transport_tools.h"
#include "transport_layer.h"
#include "transport_math.h"
#include "joystick_input.h"

#define TAG "Remote Transport"

void transport_task(void *pvParameters) {
    
    (void)pvParameters;
    char *payload = "Hello Drone !";
    char recvbuf[BUF_SIZE];
    udp_msg_t *msg = NULL;
    joystick_values_t *v = NULL;
    quat_t *q_sp = NULL ;
    msg = malloc(sizeof(udp_msg_t));
    v = malloc(sizeof(joystick_values_t)) ;
    q_sp = malloc(sizeof(quat_t)) ;
    msg->msg_type = 0 ;
    msg->set_point.q_w_be = 0;
    msg->set_point.q_x_be = 0;
    msg->set_point.q_y_be = 0;
    msg->set_point.q_z_be = 0;
    q_sp -> w=0.0f;
    q_sp -> x=0.0f;
    q_sp -> y=0.0f;
    q_sp -> z=0.0f;

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
        ESP_LOGE(TAG,"STREAM_SOCK connect() returned errno=%d", errno);
    }
    else{
        ESP_LOGI(TAG,"STREAM_SOCK connect() function returned success.") ;
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
    ESP_LOGI(TAG,"Waiting for input to arm the ESCs...");
    //Send arming input via STREAM_SOCK

    //DGRAM Socket :
    error_code = connect(udp_sock,(struct sockaddr *)&peer_addr, peer_addr_len) ;
    if(error_code < 0){
        ESP_LOGE(TAG,"DGRAM_SOCK connect() returned errno=%d", errno);
    }
    else{
        ESP_LOGI(TAG,"DGRAM_SOCK connect() function returned success.") ;
    }

    
    fd_set readfds ; //File descriptor set used to poll the tcp_socket for possible input(if input is possible from the socket)
    struct timeval timeout = {0} ;
    FD_ZERO(&readfds) ;
    
    int num_ready ; //number of ready file descriptors for input
    if(joystick_input_init()==ESP_OK){
        ESP_LOGI(TAG,"Joystick Init success");
    }else{
        ESP_LOGI(TAG,"Joystick init fail");
    }
    for(;;){
        joystick_input_read(v) ;
        ESP_LOGI(TAG, "angles : yaw= %f pitch= %f roll= %f", v->angles.yaw, v->angles.pitch, v->angles.roll);
        calculate_setpoint(&msg->set_point, v) ;

        FD_SET(tcp_sock, &readfds); //Since the select syscall modifies the sets of file descriptors
        //it is necessary to reinitialize the sets to the file descriptors of interest.    
        num_ready=select(tcp_sock + 1, &readfds, NULL, NULL, &timeout) ;
        if(num_ready==-1){ 
            ESP_LOGE(TAG,"Error during select() call on tcp socket. errno=%d", errno) ;
        }
        if(num_ready==1 && FD_ISSET(tcp_sock, &readfds)){
            ESP_LOGI(TAG,"tcp socket ready for input operation. errno=%d", errno) ;
            sendn(udp_sock, msg, 22) ;
            
            //error_code = readn();
            //read the telemetry data and implement telemetry logic
        }
        else{
            sendn(udp_sock, msg, 22) ;
        }

        //!!Put a vtaskdelay at the end!!
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    if(udp_sock > 0){
        close(udp_sock);
    }
    if(tcp_sock > 0){
        close(tcp_sock);
    }

    vTaskDelete(NULL);
}
