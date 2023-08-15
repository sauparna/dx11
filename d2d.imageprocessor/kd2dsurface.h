#pragma once

#include "kgraphics.h"

class KD2DSurface
{
public:
    KD2DSurface(HWND hwnd, D2D1_SIZE_U sz);
    ~KD2DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_device_dependent_resources();
    void discard_device_dependent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();
    void create_wic_resources();
    void discard_wic_resources();
    void create_bitmap_resources();
    void discard_bitmap_resources();
    void resize();
    void render();
    HRESULT d3d_create_device(D3D_DRIVER_TYPE const driver_type, ID3D11Device *&d3d_device);
    D2D1_SIZE_U surface_size() const;
    bool device_lost_{false};
    bool window_resized_{false};

protected:
    ID3D11Device *d3d_device_{};
    ID2D1Factory2 *d2d_factory_{};
    ID2D1DeviceContext *d2d_device_context_{};
    ID2D1Bitmap1 *d2d_dxgi_bitmap_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    
    IWICImagingFactory2 *wic_factory_{};
    IWICFormatConverter *wic_converter_{};
    IWICBitmap *wic_bitmap_{};
    ID2D1Bitmap1 *d2d_bitmap_{};
    std::wstring img_filename_{L"tintin_on_train.jpg"};
    unsigned bitmap_width_{};
    unsigned bitmap_height_{};

    HWND hwnd_{};
    D2D1_SIZE_U surface_size_{};
};
