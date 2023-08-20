#include "kclock.h"

KClock::KClock()
{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    frequency_ = f.QuadPart;

    reset();
}

void KClock::reset()
{
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    origin_ = ticks.QuadPart;
}
