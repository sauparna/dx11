#pragma once

#include <memory>
#include "kd2dsurface.h"

class KDrawingEngine : public KWindow
{
public:
    KDrawingEngine(int surface_width, int surface_height);
    LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void run();
    
private:
    std::unique_ptr<KD2DSurface> k_d2d_surface_{};
};
