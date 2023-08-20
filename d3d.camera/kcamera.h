#pragma once

enum class CameraMovement
{
    kUp,
    kDown,
    kLeft,
    kRight,
    kForward,
    kBackward,
    kYawLeft,
    kYawRight,
    kPitchUp,
    kPitchDown,
    kCameraMovementDirectionCount
};

struct KCamera
{
    KCamera();
    void update(const KClock& clock);
    void keypress(WPARAM key, bool keystate);
    
    float3 pos{0, 0, 2};
    float3 fwd{0, 0, -1};
    float pitch{0.0f};
    float yaw{0.0f};
    float4x4 perspective_matrix{};
    const float translation_speed{5.0f};
    const float rotation_speed{K_PI};
    bool camera_input[static_cast<int>(CameraMovement::kCameraMovementDirectionCount)]{};
};
