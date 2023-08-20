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
    void update(const KClock& clock);
    void keypress(WPARAM key, bool keystate);
    
    float2 obj_pos{};
    float4 obj_color{};

private:
    bool input[static_cast<int>(Movement::kMovementDirectionCount)]{};
    const float obj_speed{1.5f};
    const float change_period_in_seconds{5.0f};
    float change_frequency;
};
