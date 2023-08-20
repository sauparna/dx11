#pragma once

#include <cmath>
const double K_PI = 3.14159265358979323846;

#pragma warning(push)
#pragma warning(disable:4201) // anonymous struct warning

struct float2
{
    float x, y;
};

struct float3
{
    float x, y, z;
};

union float4
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float3 xyz;
    };
};

#pragma warning(pop)

// Internally use 3x4 to ensure HLSL-friendly alignment.
struct float3x3
{
    float m[3][4];
};

union float4x4
{
    float m[4][4];
    float4 cols[4];
    inline float4 row(int i)
    {
        return {m[0][i], m[1][i], m[2][i], m[3][i]};
    }
};

inline float degrees_to_radians(float d)
{
    return d * static_cast<float>(K_PI) / 180.0f;
}

inline float length(float3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline float length(float4 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z +v.w * v.w);
}

inline float dot(float4 a, float4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float3 operator*(float3 v, float f)
{
    return {v.x * f, v.y * f, v.z * f};
}

inline float4 operator*(float4 v, float f)
{
    return {v.x * f, v.y * f, v.z * f, v.w * f};
}

inline float3 normalize(float3 v)
{
    return v * (1.0f / length(v));
}

inline float4 normalize(float4 v)
{
    return v * (1.0f / length(v));
}

inline float3 cross(float3 a, float3 b)
{
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

inline float3 operator+=(float3 &lhs, float3 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;    
}

inline float3 operator-=(float3 &lhs, float3 rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;    
}

inline float3 operator-(float3 v)
{
    return {-v.x, -v.y, -v.z};    
}

inline float4x4 scale_matrix(float s)
{
   return {s, 0, 0, 0,
           0, s, 0, 0,
           0, 0, s, 0,
           0, 0, 0, 1};
}

inline float4x4 rotation_x_matrix(float rad)
{
    float sin_theta = sinf(rad);
    float cos_theta = cosf(rad);
    return {1, 0,          0,         0,
            0, cos_theta, -sin_theta, 0,
            0, sin_theta,  cos_theta, 0,
            0, 0,          0,         1};
}

inline float4x4 rotation_y_matrix(float rad)
{
    float sin_theta = sinf(rad);
    float cos_theta = cosf(rad);
    return { cos_theta, 0, sin_theta, 0,
             0,         1, 0,         0,
            -sin_theta, 0, cos_theta, 0,
             0,         0, 0,         1};
}

inline float4x4 translation_matrix(float3 trans)
{
   return {1, 0, 0, trans.x,
           0, 1, 0, trans.y,
           0, 0, 1, trans.z,
           0, 0, 0, 1};
}

inline float4x4 make_perspective_matrix(float aspect_ratio, float fov_y_radians, float z_near, float z_far)
{
    float y_scale = static_cast<float>(tanf((0.5f * static_cast<float>(K_PI)) - (0.5f * fov_y_radians)));
    float x_scale = y_scale / aspect_ratio;
    float z_range_inverse = 1.0f / (z_near - z_far); // REWRITE: check for a divide-by-zero error.
    float z_scale = z_far * z_range_inverse;
    float z_translation = z_far * z_near * z_range_inverse;

    float4x4 result = {x_scale, 0,        0,       0,
                       0,       y_scale,  0,       0,
                       0,       0,        z_scale, z_translation,
                       0,       0,       -1,       0};
    return result;
}

inline float4x4 operator*(float4x4 a, float4x4 b)
{
    return {dot(a.row(0), b.cols[0]),
            dot(a.row(1), b.cols[0]),
            dot(a.row(2), b.cols[0]),
            dot(a.row(3), b.cols[0]),
            dot(a.row(0), b.cols[1]),
            dot(a.row(1), b.cols[1]),
            dot(a.row(2), b.cols[1]),
            dot(a.row(3), b.cols[1]),
            dot(a.row(0), b.cols[2]),
            dot(a.row(1), b.cols[2]),
            dot(a.row(2), b.cols[2]),
            dot(a.row(3), b.cols[2]),
            dot(a.row(0), b.cols[3]),
            dot(a.row(1), b.cols[3]),
            dot(a.row(2), b.cols[3]),
            dot(a.row(3), b.cols[3])};
}

inline float4 operator*(float4 v, float4x4 m)
{
    return {dot(v, m.cols[0]),
            dot(v, m.cols[1]),
            dot(v, m.cols[2]),
            dot(v, m.cols[3])};
}

inline float4x4 transpose(float4x4 m)
{
    return float4x4 {m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0], 
                     m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1], 
                     m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2], 
                     m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]};
}

inline float3x3 float4x4_to_float3x3(float4x4 m)
{
    float3x3 result = {m.m[0][0], m.m[0][1], m.m[0][2], 0.0, 
                       m.m[1][0], m.m[1][1], m.m[1][2], 0.0,
                       m.m[2][0], m.m[2][1], m.m[2][2], 0.0};
    return result;
}
