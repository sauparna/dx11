#pragma once

#include <string>
#include <type_traits>
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

    void initialize_shaders();

    template <typename T>
    void create_shader(std::wstring filename,
                       std::string entry_point,
                       std::string target,
                       UINT compile_options,
                       ID3DBlob** blob,
                       T **d3d11_shader);

    bool shader_compiler_succeeded(HRESULT hr, ID3DBlob *shader_compiler_error_blob);
    
    void create_input_layout();
    void build_geometry();
    void create_constant_buffers();
    void create_sampler_state();
    void create_texture();
    void create_rasterizer_state();
    void create_depth_stencil_state();

    void create_wic_resources();
    void discard_wic_resources();

    void render(KClock& clock);
    void resize();
    HRESULT create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                              ID3D11Device1 **d3d11_device,
                              ID3D11DeviceContext1 **d3d11_device_context);

    bool device_lost_{false};
    bool window_resized_{true};
    KWorldState world_state_{};
    KCamera camera_{};

protected:

    void enable_d3d_debugging(ID3D11Device1 **d3d11_device);

    ID3D11Device1 *d3d11_device_{};
    ID3D11DeviceContext1* d3d11_device_context_{};
    IDXGISwapChain1 *dxgi_swap_chain_{};
    ID3D11RenderTargetView *d3d11_frame_buffer_view_{};
    ID3D11DepthStencilView *d3d11_depth_buffer_view_{};
    ID3D11DepthStencilState *d3d11_depth_stencil_state_{};

    ID3D11VertexShader *lights_vertex_shader_{};
    ID3D11VertexShader *blinnphong_vertex_shader_{};
    ID3D11PixelShader *lights_pixel_shader_{};
    ID3D11PixelShader *blinnphong_pixel_shader_{};
    ID3D11InputLayout *lights_input_layout_{};
    ID3D11InputLayout *blinnphong_input_layout_{};

    ID3D11Buffer *vertex_buffer_{};
    ID3D11Buffer *index_buffer_{};
    ID3D11Buffer *lights_constbuf_{};
    ID3D11Buffer *blinnphong_constbuf_{};
    ID3D11Buffer *blinnphong_ps_constbuf_{};

    ID3D11RasterizerState *rasterizer_state_{};
    
    UINT stride_{};
    UINT offset_{};
    UINT nvertex_{};
    UINT nindex_{};

    float4x4 perspective_matrix_{};
    float4x4 view_matrix_{};
    float4x4 inverse_view_matrix_{};
    float4x4 model_matrix_{};
    float4x4 mvp_matrix_{};

    struct KLightsConstBufDataStruct
    {
        float4x4 mvp_matrix;
        float4 color;
    };

    struct KBlinnPhongConstBufDataStruct
    {
        float4x4 mvp_matrix;
        float4x4 mv_matrix;
        float3x3 normal_matrix;
    };

    struct KDirectionalLight
    {
        float4 eye_dir; // Towards the light.
        float4 color;
    };

    struct KPointLight
    {
        float4 eye_pos;
        float4 color;
    };

    struct KBlinnPhongPSConstBufDataStruct
    {
        KDirectionalLight dir_light;
        KPointLight point_light;
    };

    ID3D11SamplerState *sampler_state_{};
    ID3D11Texture2D *texture_{};
    ID3D11ShaderResourceView *texture_view_{};
    
    IWICImagingFactory2 *wic_factory_{};
    IWICFormatConverter *wic_converter_{};
    IWICBitmap *wic_bitmap_{};
    std::wstring img_filename_{L"texture.jpg"};
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
void KD3DSurface::create_shader(std::wstring filename,
                                std::string entry_point,
                                std::string target,
                                UINT compile_options,
                                ID3DBlob** blob,
                                T **d3d11_shader)
{
    ID3DBlob *compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(filename.c_str(),
                                    nullptr,
                                    nullptr,
                                    entry_point.c_str(),
                                    target.c_str(),
                                    compile_options,
                                    0,
                                    blob,
                                    &compiler_error_blob);
    assert(shader_compiler_succeeded(hr, compiler_error_blob));

    if constexpr (std::is_same<T, ID3D11VertexShader>::value)
    {
        hr = d3d11_device_->CreateVertexShader((*blob)->GetBufferPointer(),
                                               (*blob)->GetBufferSize(),
                                               nullptr,
                                               d3d11_shader);
        assert(SUCCEEDED(hr));
        return;
    }

    if constexpr (std::is_same<T, ID3D11PixelShader>::value)
    {
        hr = d3d11_device_->CreatePixelShader((*blob)->GetBufferPointer(),
                                              (*blob)->GetBufferSize(),
                                              nullptr,
                                              d3d11_shader);
        assert(SUCCEEDED(hr));
        return;
    }
}

template <typename T>
inline void SafeRelease(T** pointer_to_object)
{
    if (*pointer_to_object != nullptr)
    {
        (*pointer_to_object)->Release();
        (*pointer_to_object) = nullptr;
    }
}
