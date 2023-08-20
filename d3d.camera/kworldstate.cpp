#include <cmath>
#include "kmath.h"
#include "kclock.h"
#include "kworldstate.h"

KWorldState::KWorldState()
    : change_frequency{2.0f * static_cast<float>(K_PI) / change_period_in_seconds}
{
}

void KWorldState::update(const KClock& clock)
{
    float displacement = obj_speed * clock.dt;
    if (input[static_cast<int>(Movement::kUp)])
        obj_pos.y += displacement;
    if (input[static_cast<int>(Movement::kDown)])
        obj_pos.y -= displacement;
    if (input[static_cast<int>(Movement::kLeft)])
        obj_pos.x -= displacement;
    if (input[static_cast<int>(Movement::kRight)])
        obj_pos.x += displacement;

    obj_color.x = 0.5f * (sinf(change_frequency * static_cast<float>(clock.t)) + 1.0f);
    obj_color.y = 1.0f - obj_color.x;
    obj_color.z = 0.0f;
    obj_color.w = 1.0f;
}

void KWorldState::keypress(WPARAM key, bool keystate)
{
    if (key == 'W')
        input[static_cast<int>(Movement::kUp)] = keystate;
    else if (key == 'A')
        input[static_cast<int>(Movement::kLeft)] = keystate;
    else if (key == 'S')
        input[static_cast<int>(Movement::kDown)] = keystate;
    else if (key == 'D')
        input[static_cast<int>(Movement::kRight)] = keystate;
    else if (key == VK_UP)
        input[static_cast<int>(Movement::kUp)] = keystate;
    else if (key == VK_LEFT)
        input[static_cast<int>(Movement::kLeft)] = keystate;
    else if (key == VK_DOWN)
        input[static_cast<int>(Movement::kDown)] = keystate;
    else if (key == VK_RIGHT)
        input[static_cast<int>(Movement::kRight)] = keystate;
}
