#pragma once

#include "kmath.h"

enum class Movement
{
    kUp,
    kDown,
    kLeft,
    kRight,
    kMovementDirectionCount
};

class KWorldState
{
public:
    KWorldState();
    void update(const KClock& clock,
                const float4x4 &view_matrix,
                const float4x4 &inverse_view_matrix);
    void keypress(WPARAM key, bool keystate);
    
    float3 obj_pos{0, 0, 0};
    float4 obj_color{1, 1, 1};
    float4x4 obj_mv_matrix{};
    float3x3 obj_normal_matrix{};

    float4 light_pos{1, 0.5f, 0, 1};
    float4 light_color{0.9f, 0.9f, 0.9f, 1.f};
    float4 light_pos_eye{};
    float4x4 light_mv_matrix{};

private:
    bool input[static_cast<int>(Movement::kMovementDirectionCount)]{};
    const float obj_speed{1.5f};
    const float change_period_in_seconds{5.0f};
    float change_frequency;
};
