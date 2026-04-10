#ifndef ORIENTATIONAPI_MATRIX_H
#define ORIENTATIONAPI_MATRIX_H

#include "types.h"

matrix_t get_skew_mat(vec3_t v);
vec3_t mat_vec_prod(matrix_t mat, vec3_t v) ;
void euler_to_rotmat(angles_t *angles, matrix_t* rotmat);


#endif