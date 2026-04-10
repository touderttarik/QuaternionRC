#include "matrix.h"
#include <math.h>
#include "stdint.h" // pour avoir le type uint8_t


matrix_t get_skew_mat(vec3_t v){

    matrix_t skew ;
    
    for(uint8_t i=0 ; i<3 ; i++){
        skew.R[i][i] = 0 ;
    }
    skew.R[0][1] = -v.z ;
    skew.R[0][2] =  v.y ;
    skew.R[1][0] =  v.z ;
    skew.R[1][2] = -v.x ;
    skew.R[2][0] = -v.y ;
    skew.R[2][1] =  v.x ;

    return skew ;
}

vec3_t mat_vec_prod (matrix_t mat, vec3_t v){

    float v_array_for_looping[3] = {v.x, v.y, v.z};
    float prod_for_looping[3]    = {.0f ,.0f ,.0f};
    vec3_t prod ;
    for(uint8_t i=0;i<3;i++){
        for(uint8_t j=0;j<3;j++){
            prod_for_looping[i] += mat.R[i][j]*v_array_for_looping[j] ;
        }
    }

    prod.x = prod_for_looping[0] ;
    prod.y = prod_for_looping[1] ;
    prod.z = prod_for_looping[2] ;

    return prod ;
}

void euler_to_rotmat(angles_t *angles, matrix_t *rotmat){
    rotmat->R[0][0] = cosf(angles->yaw)*cosf(angles->pitch);
    rotmat->R[0][1] = cosf(angles->yaw)*sinf(angles->pitch)*sinf(angles->roll) - sinf(angles->yaw)*cosf(angles->roll) ;
    rotmat->R[0][2] = cosf(angles->yaw)*sinf(angles->pitch)*cosf(angles->roll) + sinf(angles->yaw)*sinf(angles->roll) ;
    rotmat->R[1][0] = sinf(angles->yaw)*sinf(angles->pitch) ;
    rotmat->R[1][1] = sinf(angles->yaw)*sinf(angles->pitch)*sinf(angles->roll) + cosf(angles->yaw)*cosf(angles->roll) ;
    rotmat->R[1][2] = sinf(angles->yaw)*sinf(angles->pitch)*cosf(angles->roll) - cosf(angles->yaw)*sinf(angles->roll) ;
    rotmat->R[2][0] = -sinf(angles->pitch) ;
    rotmat->R[2][1] = cosf(angles->pitch)*sinf(angles->roll) ;
    rotmat->R[2][2] = cosf(angles->pitch)*cosf(angles->roll) ;

}
