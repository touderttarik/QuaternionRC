#ifndef TRANSPORT_MATH_H
#define TRANSPORT_MATH_H

#include "joystick_input.h"
#include "transport_types.h"
#include "transport_tools.h"
#include "quaternions.h"
#include "matrix.h"
#include "vectors.h"



void calculate_setpoint(udp_set_point_v1_t *sp, joystick_values_t *joystick_input) ;



#endif