#pragma once

#include <string>
#include <d2d1_2.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>
#include <dwrite.h>

#include "kscene.h"

class KD2DSurface
{
public:
    KD2DSurface(HWND hwnd, uint32_t width, uint32_t height, KScene &scene);
    ~KD2DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_device_dependent_resources();
    void discard_device_dependent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();

    void create_text_resources();
    void discard_text_resources();

    void resize();
    void render();
    void draw();
    HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriveType,
                              ID3D11Device1 **d3d11_device,
                              ID3D11DeviceContext1 **d3d11_device_context);
    bool device_lost_{false};
    bool window_resized_{true};

protected:
    KScene &scene_;

    void initialize_d3d11_debug_layer(ID3D11Device1 **d3d11_device);
    
    ID3D11Device1 *d3d11_device_{};
    ID3D11DeviceContext1* d3d11_device_context_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    ID2D1Factory2 *d2d1_factory_{};
    ID2D1RenderTarget *d2d1_dxgi_surface_rt_{};
    ID2D1SolidColorBrush *d2d1_brush_{};
    ID2D1StrokeStyle *d2d1_stroke_style_{};
    
    UINT d3d11_runtime_layers_{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Text components.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IDWriteFactory *dwrite_factory_{};
    IDWriteTextFormat *dwrite_text_format_{};
    ID2D1SolidColorBrush *d2d1_text_brush_{};
    ///////////////////////////////////////////////////////////////////////////////////////////

    HWND hwnd_{};
    D2D1_SIZE_U size_{1, 1};
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
