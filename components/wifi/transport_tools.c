#include "transport_tools.h"
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

void htonsp(udp_set_point_v1_t* sp, quat_t *q_sp){
    float q_sp_w, q_sp_x, q_sp_y, q_sp_z ;
    q_sp_w = q_sp->w ;
    q_sp_x = q_sp->x ;
    q_sp_y = q_sp->y ;
    q_sp_z = q_sp->z ;
    sp->q_w_be = htonf(&q_sp_w);
    sp->q_x_be = htonf(&q_sp_x);
    sp->q_y_be = htonf(&q_sp_y);
    sp->q_z_be = htonf(&q_sp_z);


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
        tot_read += (size_t)num_read;
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
        num_written = write(sock, buf, count - tot_written);
        
        if(num_written <= 0){
            if(num_written == -1 && errno == EINTR){
                continue ;//Interrupted --> restard write()
            }
            else{
                return -1 ;//Some other error
            }
        }
        
        tot_written += (size_t)num_written;
        buf += num_written;
    }
    return tot_written ;
}
