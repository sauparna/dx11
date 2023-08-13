#pragma once

#include <memory>
#include "kwindow.h"
#include "kd2dsurface.h"

class KImagingEngine : public KWindow
{
public:
    KImagingEngine(D2D1_SIZE_U surface_sz);
    LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void run();
    
private:
    std::unique_ptr<KD2DSurface> k_d2d_surface_{};
};
