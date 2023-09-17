#include "kclock.h"

KClock::KClock()
{
    reset();
}

void KClock::reset()
{
    QueryPerformanceCounter(&o_);
    prev_ = t_ = o_;
    QueryPerformanceFrequency(&f_);
}

double KClock::time(double *dt, LONGLONG resolution)
{
    prev_ = t_;

    QueryPerformanceCounter(&t_);

    dt_.QuadPart = (t_.QuadPart - prev_.QuadPart);
    auto t = dt_.QuadPart * resolution;
    *dt =  static_cast<double>(t / f_.QuadPart);

    elapsed_.QuadPart = (t_.QuadPart - o_.QuadPart);
    t = elapsed_.QuadPart * resolution;
    return static_cast<double>(t / f_.QuadPart);
}
