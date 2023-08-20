#include "kmath.h"
#include "kclock.h"
#include "kcamera.h"

KCamera::KCamera() {}

void KCamera::update(const KClock& clock)
{
    float3 roll_axis = normalize(float3{fwd.x, 0, fwd.z});
    float3 pitch_axis = cross(roll_axis, {0, 1, 0});
    
    float translation_delta = translation_speed * clock.dt;
    float3 z_displacement = roll_axis * translation_delta;
    float3 x_displacement = pitch_axis * translation_delta;

    if (camera_input[static_cast<int>(CameraMovement::kForward)])
        pos += z_displacement;
    if (camera_input[static_cast<int>(CameraMovement::kBackward)])
        pos -= z_displacement;
    if (camera_input[static_cast<int>(CameraMovement::kLeft)])
        pos -= x_displacement;
    if (camera_input[static_cast<int>(CameraMovement::kRight)])
        pos += x_displacement;
    if (camera_input[static_cast<int>(CameraMovement::kUp)])
        pos.y += translation_delta;
    if (camera_input[static_cast<int>(CameraMovement::kDown)])
        pos.y -= translation_delta;

    const float rotation_delta = rotation_speed * clock.dt;

    if (camera_input[static_cast<int>(CameraMovement::kYawLeft)])
        yaw += rotation_delta;
    if (camera_input[static_cast<int>(CameraMovement::kYawRight)])
        yaw -= rotation_delta;
    if (camera_input[static_cast<int>(CameraMovement::kPitchUp)])
        pitch += rotation_delta;
    if (camera_input[static_cast<int>(CameraMovement::kPitchDown)])
        pitch -= rotation_delta;

    // Wrap yaw to avoid floating point errors when the rotation-delta
    // is high.
    float two_pi = 2.0f * K_PI;
    while (yaw >= two_pi)
        yaw -= two_pi;
    while (yaw <= -two_pi)
        yaw += two_pi;

    // Clamp pitch to prevent the camera from flipping upside-down.
    float threshold = degrees_to_radians(85);
    if (pitch > threshold)
        pitch = threshold;
    if (pitch < -threshold)
        pitch = -threshold;
}

void KCamera::keypress(WPARAM key, bool keystate)
{
    if (key == 'W')
        camera_input[static_cast<int>(CameraMovement::kForward)] = keystate;
    else if (key == 'A')
        camera_input[static_cast<int>(CameraMovement::kLeft)] = keystate;
    else if (key == 'S')
        camera_input[static_cast<int>(CameraMovement::kBackward)] = keystate;
    else if (key == 'D')
        camera_input[static_cast<int>(CameraMovement::kRight)] = keystate;
    else if (key == 'E')
        camera_input[static_cast<int>(CameraMovement::kUp)] = keystate;
    else if (key == 'Q')
        camera_input[static_cast<int>(CameraMovement::kDown)] = keystate;
    else if (key == VK_UP)
        camera_input[static_cast<int>(CameraMovement::kPitchUp)] = keystate;
    else if (key == VK_LEFT)
        camera_input[static_cast<int>(CameraMovement::kYawLeft)] = keystate;
    else if (key == VK_DOWN)
        camera_input[static_cast<int>(CameraMovement::kPitchDown)] = keystate;
    else if (key == VK_RIGHT)
        camera_input[static_cast<int>(CameraMovement::kYawRight)] = keystate;
}
