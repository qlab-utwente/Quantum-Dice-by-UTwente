// From: https://github.com/rbv188/IMU-algorithm/tree/master
#include "quaternion.hpp"

Quaternion quaternion_initialize(float a, float b, float c, float d)
{
    Quaternion q;
    q.a = a;
    q.b = b;
    q.c = c;
    q.d = d;
    return q;
}

Quaternion quaternion_product(Quaternion q1, Quaternion q2)
{
    //q = q1*q2
    Quaternion q;
    q.a = (q1.a*q2.a) - (q1.b*q2.b) - (q1.c*q2.c) - (q1.d*q2.d);
    q.b = (q1.a*q2.b) + (q1.b*q2.a) + (q1.c*q2.d) - (q1.d*q2.c);
    q.c = (q1.a*q2.c) - (q1.b*q2.d) + (q1.c*q2.a) + (q1.d*q2.b);
    q.d = (q1.a*q2.d) + (q1.b*q2.c) - (q1.c*q2.b) + (q1.d*q2.a);
    return q;
}

Quaternion quaternion_conjugate(Quaternion q1)
{
    Quaternion q2;
    q2.a = q1.a;
    q2.b = -q1.b;
    q2.c = -q1.c;
    q2.d = -q1.d;
    return q2;
}

Quaternion quaternion_normalize(Quaternion q1)
{
    Quaternion q2;
    float one_by_sqrt;
    one_by_sqrt = 1.0F / sqrtf(q1.a*q1.a + q1.b*q1.b + q1.c*q1.c + q1.d*q1.d);
    q2.a = q1.a*one_by_sqrt;
    q2.b = q1.b*one_by_sqrt;
    q2.c = q1.c*one_by_sqrt;
    q2.d = q1.d*one_by_sqrt;
    return q2;
}

Quaternion quaternion_between_vectors(Vector3 v1, Vector3 v2)
{
    // rotates from v1 to v2
    Vector3 v1_norm = v1.normalize();
    Vector3 v2_norm = v2.normalize();
    Vector3 half_way_vector = (v1_norm + v2_norm).normalize();
    float angle = v1_norm.dotProduct(half_way_vector);
    Vector3 axis = v1_norm.crossProduct(half_way_vector);
    Quaternion result = quaternion_initialize(angle, axis.x, axis.y,axis.z);
    return result;
}

Vector3 quaternion_rotate_vector(Vector3 v, Quaternion q)
{
    Quaternion quaternion_vector = quaternion_initialize(0.0, v.x, v.y, v.z);
    Quaternion q_inverse = quaternion_conjugate(q);
    Quaternion quaternion_rotated_vector = quaternion_product(quaternion_product(q, quaternion_vector),q_inverse);
    return Vector3(quaternion_rotated_vector.b, quaternion_rotated_vector.c, quaternion_rotated_vector.d);
}
