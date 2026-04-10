#ifndef TYPES_H
#define TYPES_H

#include "stdbool.h"
#include "stdint.h"

typedef struct {float w, x, y, z ;} quat_t ;
typedef struct {float x, y, z; } vec3_t ;
typedef struct {vec3_t axis; float angle ;} axis_angle_t ;
typedef struct {float roll, pitch, yaw ;} angles_t ;
typedef struct {float R[3][3] ;} matrix_t ;


typedef struct {
    angles_t angles ;
    float throttle_stick;  // [-1.0, +1.0] (corrected orientation)
    float throttle;        // [0.0, +1.0]

    bool right_switch_pressed; // GPIO25, active low
    bool left_switch_pressed;  // GPIO26, active low

    uint64_t timestamp_us;
} joystick_values_t;


#endif
