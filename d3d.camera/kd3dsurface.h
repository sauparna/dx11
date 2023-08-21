#pragma once

#include <string>
#include <wincodec.h>
#include <d2d1_2.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>

#include "kclock.h"
#include "kworldstate.h"
#include "kcamera.h"

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
    void create_constant_buffer();
    void create_sampler_state();
    void create_texture();
    void create_rasterizer_state();

    void create_wic_resources();
    void discard_wic_resources();

    void render(KClock& clock);
    void resize();
    HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                              ID3D11Device1 **d3d11_device,
                              ID3D11DeviceContext1 **d3d11_device_context);

    bool device_lost_{false};
    bool window_resized_{true};
    // KWorldState world_state_{};
    KCamera camera_{};

protected:

    void enable_d3d_debugging(ID3D11Device1 **d3d11_device);

    ID3D11Device1 *d3d11_device_{};
    ID3D11DeviceContext1* d3d11_device_context_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    ID3D11RenderTargetView *d3d11_frame_buffer_view_{};
    ID3D11DepthStencilView *d3d11_depth_buffer_view_{};

    ID3DBlob *vs_blob_{};
    ID3D11VertexShader *vertex_shader_;
    ID3DBlob *ps_blob_{};
    ID3D11PixelShader *pixel_shader_;

    ID3D11InputLayout *input_layout_{};
    ID3D11Buffer *vertex_buffer_;
    ID3D11Buffer *constant_buffer_{};
    ID3D11RasterizerState *rasterizer_state_{};
    
    UINT stride_;
    UINT offset_;
    UINT nvertex_;

    float4x4 perspective_matrix_{};
    float4x4 view_matrix_{};
    float4x4 model_matrix_{};
    float4x4 mvp_matrix_{};

    struct KD3DConstantsDataStructure
    {
        // "color" must be 16-byte-algined; hence the padding.
        // float2 pos;
        // float2 padding;
        // float4 color;
        float4x4 model_view_projection_matrix;
    };

    ID3D11SamplerState *sampler_state_{};
    ID3D11Texture2D *texture_{};
    ID3D11ShaderResourceView *texture_view_{};
    
    IWICImagingFactory2 *wic_factory_{};
    IWICFormatConverter *wic_converter_{};
    IWICBitmap *wic_bitmap_{};
    // ID2D1Bitmap1 *d2d_bitmap_{};
    std::wstring img_filename_{L"tintin_on_train.jpg"};
    unsigned bitmap_width_{};
    unsigned bitmap_height_{};

    UINT d3d11_runtime_layers_{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
    UINT d3d11_shader_compile_options_{0};

    HWND hwnd_{};
    int surface_width_{1};
    int surface_height_{1};
    float surface_aspect_ratio_{1.0f};    
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
