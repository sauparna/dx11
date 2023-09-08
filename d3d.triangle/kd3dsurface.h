#pragma once

#include <wincodec.h>
#include <d2d1_2.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>

class KD3DSurface
{
public:
    KD3DSurface(HWND hwnd, int width, int height);
    ~KD3DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_device_dependent_resources();
    void discard_device_dependent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();

    void create_vertex_shader();
    void create_pixel_shader();
    bool shader_compiler_succeeded(HRESULT hr, ID3DBlob *shader_compiler_error_blob);
    
    void create_input_layout();
    void create_vertex_buffer();

    void render();
    void resize();
    HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                              ID3D11Device1 **d3d11_device,
                              ID3D11DeviceContext1 **d3d11_device_context);
    bool device_lost_{false};
    bool window_resized_{true};

protected:
    
    void enable_d3d_debugging(ID3D11Device1 **d3d11_device);
    
    ID3D11Device1 *d3d11_device_{};
    ID3D11DeviceContext1* d3d11_device_context_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    ID3D11RenderTargetView *d3d11_frame_buffer_view_{};

    ID3DBlob *vs_blob_{};
    ID3D11VertexShader *vertex_shader_;
    ID3DBlob *ps_blob_{};
    ID3D11PixelShader *pixel_shader_;

    ID3D11InputLayout *input_layout_{};
    ID3D11Buffer *vertex_buffer_;
    UINT stride_;
    UINT offset_;
    UINT nvertex_;
    
    // IWICImagingFactory2 *wic_factory_{};
    // IWICFormatConverter *wic_converter_{};
    // IWICBitmap *wic_bitmap_{};
    // ID2D1Bitmap1 *d2d_bitmap_{};
    // std::wstring img_filename_{L"tintin_on_train.jpg"};
    // unsigned bitmap_width_{};
    // unsigned bitmap_height_{};

    UINT d3d11_runtime_layers_{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
    UINT d3d11_shader_compile_options_{0};
    
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
