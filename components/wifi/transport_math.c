#include "transport_math.h"


void calculate_setpoint(udp_set_point_v1_t *sp, joystick_values_t *joystick_input) {
    matrix_t rotmat ;
    quat_t *q_sp = NULL;
    q_sp = malloc(sizeof(quat_t)) ;
    euler_to_rotmat(&joystick_input->angles, &rotmat) ;
    rotmat_to_quat(rotmat, q_sp);
    htonsp(sp, q_sp);
}