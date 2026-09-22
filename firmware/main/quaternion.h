// From: https://github.com/rbv188/IMU-algorithm/tree/master
#ifdef __cplusplus
extern "C" {
#endif

#ifndef QUATERNION_INCLUDED
#define QUATERNION_INCLUDED

#include "vector_3d.h"

typedef struct Quaternion {

    float a;
    float b;
    float c;
    float d;

    // q = a + bi + cj + dk

} Quaternion;

Quaternion quaternion_initialize(float a, float b, float c, float d);
Quaternion quaternion_product(Quaternion q1, Quaternion q2);
Quaternion quaternion_conjugate(Quaternion q);
Quaternion quaternion_normalize(Quaternion q);
Quaternion quaternion_between_vectors(vector_ijk v1, vector_ijk v2);
vector_ijk quaternion_rotate_vector(vector_ijk v, Quaternion q);

#endif

#ifdef __cplusplus
}
#endif
