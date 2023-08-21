#pragma once

#include <windows.h>

class KClock
{
public:
    KClock();
    void reset();
    
    LONGLONG origin_{};
    LONGLONG frequency_{};
    double t{};
    float dt{};

private:
};
