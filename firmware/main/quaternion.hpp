// From: https://github.com/rbv188/IMU-algorithm/tree/master
#ifndef QUATERNION_INCLUDED
#define QUATERNION_INCLUDED

#include "Vector3.hpp"

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
Quaternion quaternion_between_vectors(Vector3 v1, Vector3 v2);
Vector3 quaternion_rotate_vector(Vector3 v, Quaternion q);

#endif
