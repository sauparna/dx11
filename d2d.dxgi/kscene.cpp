#include "kscene.h"

KScene::KScene(int width, int height) :
    width_{width},
    height_{height},
    x_{static_cast<float>(width_) / 2.f},
    y_{static_cast<float>(height_) / 2.f},
    rng{rdev()},
    rdist{std::uniform_int_distribution<int>(1, 9)}
{
    
}

KScene::~KScene()
{
}

void KScene::update()
{
    x_ += dx_;
    y_ += dy_;

    if (x_ >= static_cast<float>(width_) || x_ <= 0.f)
    {
        x_ = std::clamp<float>(x_, 0.f, static_cast<float>(width_) -1.f);
        float switch_direction = signbit(dx_) ? 1.f : -1.f;
        dx_ = switch_direction * cosine_of_random_deflection_angle_delta() * speed_;
    }

    if (y_ >= static_cast<float>(height_) || y_ <= 0.f)
    {
        y_ = std::clamp<float>(y_, 0.f, static_cast<float>(height_) - 1.f);
        float switch_direction = signbit(dy_) ? 1.f : -1.f;
        dy_ = switch_direction * sine_of_random_deflection_angle_delta() * speed_;
    }
}

// cos(k * 1/10 * pi/2), where k in [1, 9].
// k != 10 to ensure we don't return a zero.
inline float KScene::cosine_of_random_deflection_angle_delta()
{
    static const float half_pi = 3.141592f / 2;
    static const float delta = half_pi / 10.0f;
    int k = rdist(rng);
    return cosf(static_cast<float>(k) * delta);
}

// sin(k * 1/10 * pi/2), where k in [1, 9].
// k != 10 to ensure we don't return a zero.
inline float KScene::sine_of_random_deflection_angle_delta()
{
    static const float half_pi = 3.141592f / 2;
    static const float delta = half_pi / 10.0f;
    int k = rdist(rng);
    return sinf(static_cast<float>(k) * delta);
}
