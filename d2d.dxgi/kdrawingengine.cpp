#include "kwindow.h"
#include "kdrawingengine.h"

using namespace std;

KDrawingEngine::KDrawingEngine(int surface_width, int surface_height) : KWindow{}
{
    initialize(surface_width, surface_height);
    k_d2d_surface_ = std::make_unique<KD2DSurface>(hwnd_, surface_width, surface_height);
}

LRESULT KDrawingEngine::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lr = 0;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (k_d2d_surface_)
            k_d2d_surface_->window_resized_ = true;
        break;
    default:
        lr = DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return lr;
}

void KDrawingEngine::run()
{
    static bool running = true;
    while (running)
    {
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (k_d2d_surface_ == nullptr) continue;
        if (k_d2d_surface_->window_resized_)
        {
            k_d2d_surface_->resize();
            k_d2d_surface_->window_resized_ = false;
        }

        if (k_d2d_surface_ == nullptr) continue;
        if (k_d2d_surface_->device_lost_)
        {
            k_d2d_surface_->discard_device_dependent_resources();
            k_d2d_surface_->create_device_dependent_resources();
            k_d2d_surface_->create_render_target_resources();
            k_d2d_surface_->device_lost_ = false;
        }

        if (k_d2d_surface_ == nullptr) continue;
        k_d2d_surface_->render();
        k_d2d_surface_->update();
    }
}
