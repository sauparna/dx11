#pragma once

#include <string>
#include <d2d1_2.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>

class KD2DSurface
{
public:
    KD2DSurface(HWND hwnd, int width, int height);
    ~KD2DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_device_dependent_resources();
    void discard_device_dependent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();

    void resize();
    void render();
    void update();
    HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriveType,
                              ID3D11Device1 **d3d11_device,
                              ID3D11DeviceContext1 **d3d11_device_context);
    bool device_lost_{false};
    bool window_resized_{true};

protected:

    void initialize_d3d11_debug_layer(ID3D11Device1 **d3d11_device);
    
    ID3D11Device1 *d3d11_device_{};
    ID3D11DeviceContext1* d3d11_device_context_{};
    ID2D1Factory2 *d2d1_factory_{};
    ID2D1DeviceContext *d2d1_device_context_{};
    ID2D1Bitmap1 *d2d1_dxgi_bitmap_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    
    const int kMemWidth{100};
    const int kMemHeight{100};
    const int kBytesPerPixel{4};
    const int kMemStride = kMemWidth * kBytesPerPixel;
    const D2D1_RECT_U kMemSrcRect = D2D1::RectU(0, 0, kMemWidth, kMemHeight);
    const int kMemSize{kMemWidth * kMemHeight};
    uint32_t *mem_{};
    int x_{kMemWidth / 2};
    int y_{kMemWidth / 2};
    int dx_{1};
    int dy_{1};
    
    // 32-bit color layout: 0xaarrggbb, where aa = alpha, rr = red, bb = green, gg = blue
    void put_pixel(int x, int y, uint32_t color);
    void clear_bitmap_mem(uint32_t color);

    UINT d3d11_runtime_layers_{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
    
    HWND hwnd_{};
    int surface_width_{1};
    int surface_height_{1};
};

template <typename T>
inline void SafeRelease(T** pointer_to_object)
{
    if (*pointer_to_object != nullptr)
    {
        (*pointer_to_object)->Release();
        (*pointer_to_object) = nullptr;
    }
}
