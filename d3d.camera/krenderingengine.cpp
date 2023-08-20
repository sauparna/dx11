#include "kwindow.h"
#include "krenderingengine.h"

using namespace std;

KRenderingEngine::KRenderingEngine(int surface_width, int surface_height) : KWindow{}
{
    initialize(surface_width, surface_height);
    k_d3d_surface_ = std::make_unique<KD3DSurface>(hwnd_, surface_width, surface_height);
}

LRESULT KRenderingEngine::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{ 
    LRESULT lr = 0;
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        bool keystate = (msg == WM_KEYDOWN);
        if (wparam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        else if (k_d3d_surface_)
        {
            // k_d3d_surface_->world_state_.keypress(wparam, keystate);
            k_d3d_surface_->camera_.keypress(wparam, keystate);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (k_d3d_surface_)
            k_d3d_surface_->window_resized_ = true;
        break;
    default:
        lr = DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return lr;
}

void KRenderingEngine::run()
{
    static bool running = true;
    clock.reset();
    while (running)
    {
        float dt{};
        {
            double prev_t = clock.t;
            LARGE_INTEGER ticks;
            QueryPerformanceCounter(&ticks);
            clock.t = static_cast<double>(ticks.QuadPart - clock.origin_) / static_cast<double>(clock.frequency_);
            dt = static_cast<float>(clock.t - prev_t);
            if (dt > (1.0f / 60.0f))
                dt = (1.0f / 60.0f);
        }
        clock.dt = dt;
    
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (k_d3d_surface_ == nullptr) continue;
        if (k_d3d_surface_->window_resized_)
        {
            k_d3d_surface_->resize();
            k_d3d_surface_->window_resized_ = false;
        }

        if (k_d3d_surface_ == nullptr) continue;
        if (k_d3d_surface_->device_lost_)
        {
            k_d3d_surface_->discard_device_dependent_resources();
            k_d3d_surface_->create_device_dependent_resources();
            k_d3d_surface_->create_render_target_resources();
            k_d3d_surface_->create_vertex_shader();
            k_d3d_surface_->create_pixel_shader();
            k_d3d_surface_->create_input_layout();
            k_d3d_surface_->create_vertex_buffer();
            k_d3d_surface_->create_constant_buffer();
            k_d3d_surface_->create_sampler_state();
            k_d3d_surface_->create_texture();
            k_d3d_surface_->create_rasterizer_state();
            k_d3d_surface_->device_lost_ = false;
        }

        if (k_d3d_surface_ == nullptr) continue;
        k_d3d_surface_->render(clock);
    }
}
