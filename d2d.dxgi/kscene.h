#pragma once

#include <random>

class KScene
{
public:
    KScene(int width, int height);
    ~KScene();
    void update();
    float cosine_of_random_deflection_angle_delta();
    float sine_of_random_deflection_angle_delta();

    float x_{};
    float y_{};

private:
    int width_{100};
    int height_{100};
    float dx_{1.f};
    float dy_{1.f};
    float speed_{1.f};

    std::random_device rdev{};
    std::mt19937 rng;
    std::uniform_int_distribution<int> rdist;
 };
