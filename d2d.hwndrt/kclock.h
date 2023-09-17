#pragma once

// KClock::time() returns the program's elapsed time in microseconds,
// and the time-delta between two successive calls to itself in the
// pointer variable dt passed in to it.

#include <windows.h>

class KClock
{
public:
    KClock();
    void reset();
    double time(double *dt = nullptr, LONGLONG resolution = 1E6);

private:
    LARGE_INTEGER o_{};
    LARGE_INTEGER f_{};
    LARGE_INTEGER t_{};
    LARGE_INTEGER prev_{};
    LARGE_INTEGER dt_{};
    LARGE_INTEGER elapsed_{};
};
