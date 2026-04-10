idf_component_register(
    SRCS "matrix.c" "quaternions.c" "vectors.c"
    INCLUDE_DIRS "include" 
    REQUIRES commontypes
)
