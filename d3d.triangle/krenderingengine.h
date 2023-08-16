#pragma once

#include <memory>
#include "kd3dsurface.h"

class KRenderingEngine : public KWindow
{
public:
    KRenderingEngine(int surface_width, int surface_height);
    LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void run();
    
private:
    std::unique_ptr<KD3DSurface> k_d3d_surface_{};
};

 
