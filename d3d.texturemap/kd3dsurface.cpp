#include <string>
#include <cassert>
#include "kd3dsurface.h"

KD3DSurface::KD3DSurface(HWND hwnd, int width, int height)
    : hwnd_{hwnd}, surface_width_{width}, surface_height_{height}
{
    create_device_independent_resources();
    create_device_dependent_resources();
    create_render_target_resources();
}

KD3DSurface::~KD3DSurface()
{
    discard_device_dependent_resources();
    discard_device_independent_resources();
}

void KD3DSurface::create_device_independent_resources()
{
    create_wic_resources();
}

void KD3DSurface::discard_device_independent_resources()
{
    discard_wic_resources();
}

void KD3DSurface::create_device_dependent_resources()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D device and context.
    ///////////////////////////////////////////////////////////////////////////////////////////

    HRESULT hr = create_d3d_device(D3D_DRIVER_TYPE_HARDWARE, &d3d11_device_, &d3d11_device_context_);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = create_d3d_device(D3D_DRIVER_TYPE_WARP, &d3d11_device_, &d3d11_device_context_);
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the DXGI swap chain.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    IDXGIFactory2 *dxgi_factory = nullptr;
    {
        IDXGIDevice1 *dxgi_device = nullptr;
        hr = d3d11_device_->QueryInterface(__uuidof(IDXGIDevice1),
                                           reinterpret_cast<void**>(&dxgi_device));
        assert(SUCCEEDED(hr));

        IDXGIAdapter *dxgi_adapter = nullptr;
        hr = dxgi_device->GetAdapter(&dxgi_adapter);
        assert(SUCCEEDED(hr));
        dxgi_device->Release();

        DXGI_ADAPTER_DESC adapter_desc;
        dxgi_adapter->GetDesc(&adapter_desc);
        OutputDebugStringA("DXGI adapter: ");
        OutputDebugStringW(adapter_desc.Description);

        hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2),
                                     reinterpret_cast<void**>(&dxgi_factory));
        assert(SUCCEEDED(hr));
        dxgi_adapter->Release();
    }

    DXGI_SWAP_CHAIN_DESC1 dxgi_swap_chain_desc{};
    dxgi_swap_chain_desc.Width = 0;
    dxgi_swap_chain_desc.Height = 0;
    dxgi_swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    dxgi_swap_chain_desc.SampleDesc.Count = 1;
    dxgi_swap_chain_desc.SampleDesc.Quality = 0;
    dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_swap_chain_desc.BufferCount = 2;
    dxgi_swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    dxgi_swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    dxgi_swap_chain_desc.Flags = 0;

    hr = dxgi_factory->CreateSwapChainForHwnd(d3d11_device_,
                                              hwnd_,
                                              &dxgi_swap_chain_desc,
                                              0, 0,
                                              &dxgi_swap_chain_);
    assert(SUCCEEDED(hr));
    dxgi_factory->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create rest of the Direct3D device-dependent resources.
    ///////////////////////////////////////////////////////////////////////////////////////////

    create_vertex_shader();
    create_pixel_shader();
    create_input_layout();
    create_vertex_buffer();
    create_sampler_state();
    create_texture();
}

void KD3DSurface::discard_device_dependent_resources()
{
    SafeRelease(&ps_blob_);
    SafeRelease(&pixel_shader_);
    SafeRelease(&vs_blob_);
    SafeRelease(&vertex_shader_);
    SafeRelease(&input_layout_);
    SafeRelease(&vertex_buffer_);
    SafeRelease(&d3d11_frame_buffer_view_);
    SafeRelease(&dxgi_swap_chain_);
    SafeRelease(&d3d11_device_context_);
    SafeRelease(&d3d11_device_);
}

void KD3DSurface::create_render_target_resources()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D render targets.
    ///////////////////////////////////////////////////////////////////////////////////////////
    assert(d3d11_device_ != nullptr);
    assert(dxgi_swap_chain_ != nullptr);
    
    ID3D11Texture2D *d3d11_frame_buffer;
    HRESULT hr = dxgi_swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                             reinterpret_cast<void**>(&d3d11_frame_buffer));
    assert(SUCCEEDED(hr));
    hr = d3d11_device_->CreateRenderTargetView(d3d11_frame_buffer,
                                               nullptr,
                                               &d3d11_frame_buffer_view_);
    assert(SUCCEEDED(hr));
    d3d11_frame_buffer->Release();
}

void KD3DSurface::discard_render_target_resources()
{
    d3d11_device_context_->OMSetRenderTargets(0, 0, 0);
    SafeRelease(&d3d11_frame_buffer_view_);
}

void KD3DSurface::create_vertex_shader()
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr,
                                    "vs_main", "vs_5_0", 0, 0,
                                    &vs_blob_, &shader_compiler_error_blob);
    assert(shader_compiler_succeeded(hr, shader_compiler_error_blob));
    hr = d3d11_device_->CreateVertexShader(vs_blob_->GetBufferPointer(),
                                           vs_blob_->GetBufferSize(),
                                           nullptr, &vertex_shader_);
    assert(SUCCEEDED(hr));
}

void KD3DSurface::create_pixel_shader()
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr,
                                    "ps_main", "ps_5_0", 0, 0,
                                    &ps_blob_, &shader_compiler_error_blob);
    assert(shader_compiler_succeeded(hr, shader_compiler_error_blob));
    hr = d3d11_device_->CreatePixelShader(ps_blob_->GetBufferPointer(),
                                          ps_blob_->GetBufferSize(),
                                          nullptr, &pixel_shader_);
    assert(SUCCEEDED(hr));
}

bool KD3DSurface::shader_compiler_succeeded(HRESULT hr, ID3DBlob *shader_compiler_error_blob)
{
    if (FAILED(hr))
    {
        const char *error_string = nullptr;
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            error_string = "Could not compile shader; file not found.";
        }
        else if (shader_compiler_error_blob)
        {
            error_string = (const char*)shader_compiler_error_blob->GetBufferPointer();
            shader_compiler_error_blob->Release();
        }
        MessageBoxA(0, error_string, "Shader compiler error", MB_ICONERROR | MB_OK);
        return false;
    }
    return true;;
}
    
void KD3DSurface::create_input_layout()
{
    D3D11_INPUT_ELEMENT_DESC kInputElementDesc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
    HRESULT hr = d3d11_device_->CreateInputLayout(kInputElementDesc, ARRAYSIZE(kInputElementDesc),
                                                  vs_blob_->GetBufferPointer(), vs_blob_->GetBufferSize(),
                                                  &input_layout_);
    assert(SUCCEEDED(hr));
}

void KD3DSurface::create_vertex_buffer()
{
    float kVertexData[]{ // x, y, r, g, b, a
        0.0f,  0.5f, 0.f, 1.f, 0.f, 1.f,
        0.5f, -0.5f, 1.f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.f, 0.f, 1.f, 1.f
    };
    stride_ = 6 * sizeof(float);
    nvertex_ = sizeof(kVertexData) / stride_;
    offset_ = 0;

    D3D11_BUFFER_DESC d3d11_vertex_buffer_desc{};
    d3d11_vertex_buffer_desc.ByteWidth = sizeof(kVertexData);
    d3d11_vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    d3d11_vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA d3d11_vertex_subresource_data{kVertexData};

    HRESULT hr = d3d11_device_->CreateBuffer(&d3d11_vertex_buffer_desc,
                                     &d3d11_vertex_subresource_data,
                                     &vertex_buffer_);
    assert(SUCCEEDED(hr));
}

void KD3DSurface::create_sampler_state()
{
    D3D11_SAMPLER_DESC sd{};
    sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    sd.BorderColor[0] = 1.0f;
    sd.BorderColor[1] = 1.0f;
    sd.BorderColor[2] = 1.0f;
    sd.BorderColor[3] = 1.0f;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

    d3d11_device_->CreateSamplerState(&sd, &sampler_state_);
}

void KD3DSurface::create_texture()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a WIC bitmap, obtain a lock, and retrieve a pointer to
    // the bitmap memory.
    ///////////////////////////////////////////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    hr = wic_factory_->CreateBitmapFromSource(wic_converter_,
                                              WICBitmapCacheOnLoad,
                                              &wic_bitmap_);
    assert(SUCCEEDED(hr));
    WICRect lockedRect{0, 0,
                       static_cast<INT>(bitmap_width_),
                       static_cast<INT>(bitmap_height_)};
    IWICBitmapLock *lock{};
    hr = wic_bitmap_->Lock(&lockedRect, WICBitmapLockWrite, &lock);
    assert(SUCCEEDED(hr));

    UINT stride{};
    hr = lock->GetStride(&stride);
    assert(SUCCEEDED(hr));
    
    UINT sz{};
    BYTE *mem{};
    hr = lock->GetDataPointer(&sz, &mem);
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a D3D 2D texture and texture-view from the bitmap.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    D3D11_TEXTURE2D_DESC texd{};
    texd.Width              = bitmap_width_;
    texd.Height             = bitmap_height_;
    texd.MipLevels          = 1;
    texd.ArraySize          = 1;
    texd.Format             = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texd.SampleDesc.Count   = 1;
    texd.Usage              = D3D11_USAGE_IMMUTABLE;
    texd.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA texsd{};
    texsd.pSysMem = mem;
    texsd.SysMemPitch = stride;

    d3d11_device_->CreateTexture2D(&texd, &texsd, &texture_);
    d3d11_device_->CreateShaderResourceView(texture_, nullptr, &texture_view_);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Release the lock on the bitmap abstraction.
    ///////////////////////////////////////////////////////////////////////////////////////////

    lock->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////
}

void KD3DSurface::create_wic_resources()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create WIC resources to read/write image data.
    ///////////////////////////////////////////////////////////////////////////////////////////

    hr = CoCreateInstance(CLSID_WICImagingFactory2,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&wic_factory_));
    assert(SUCCEEDED(hr));

    IWICBitmapDecoder *decoder{};
    hr = wic_factory_->CreateDecoderFromFilename(img_filename_.c_str(),
                                                 nullptr,
                                                 GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand,
                                                 &decoder);
    assert(SUCCEEDED(hr));
    IWICBitmapFrameDecode *frame{};
    hr = decoder->GetFrame(0, &frame);
    assert(SUCCEEDED(hr));
    hr = wic_factory_->CreateFormatConverter(&wic_converter_);
    assert(SUCCEEDED(hr));
    hr = wic_converter_->Initialize(frame,
                                    GUID_WICPixelFormat32bppPBGRA,
                                    WICBitmapDitherTypeNone,
                                    nullptr,
                                    0.0f,
                                    WICBitmapPaletteTypeCustom);
    assert(SUCCEEDED(hr));
    wic_converter_->GetSize(&bitmap_width_, &bitmap_height_);
}

void KD3DSurface::discard_wic_resources()
{
    wic_converter_->Release();
    wic_factory_->Release();
}

void KD3DSurface::render()
{
    HRESULT hr = S_OK;    
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Render D3D content.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    FLOAT kBackgroundColor[4]{ 0.1f, 0.2f, 0.6f, 1.0f };
    d3d11_device_context_->ClearRenderTargetView(d3d11_frame_buffer_view_, kBackgroundColor);

    RECT window_rect{};
    GetClientRect(hwnd_, &window_rect);
    D3D11_VIEWPORT d3d11_viewport{
        0.0f, 0.0f,
        (FLOAT)(window_rect.right - window_rect.left),
        (FLOAT)(window_rect.bottom - window_rect.top),
        0.0f, 1.0f
    };

    d3d11_device_context_->RSSetViewports(1, &d3d11_viewport);

    d3d11_device_context_->OMSetRenderTargets(1, &d3d11_frame_buffer_view_, nullptr);

    d3d11_device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d11_device_context_->IASetInputLayout(input_layout_);

    d3d11_device_context_->VSSetShader(vertex_shader_, nullptr, 0);

    d3d11_device_context_->PSSetShader(pixel_shader_, nullptr, 0);
    d3d11_device_context_->PSSetShaderResources(0, 1, &texture_view_);
    d3d11_device_context_->PSSetSamplers(0, 1, &sampler_state_);

    d3d11_device_context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride_, &offset_);

    d3d11_device_context_->Draw(nvertex_, 0);

    hr = dxgi_swap_chain_->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        device_lost_ = true;
        return;
    }
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////}
}

void KD3DSurface::resize()
{
    discard_render_target_resources();
    HRESULT hr = dxgi_swap_chain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    assert(SUCCEEDED(hr));
    create_render_target_resources();
}

// REWRITE: Consider not passing in the device and context placeholders.
HRESULT KD3DSurface::create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                                       ID3D11Device1 **d3d11_device,
                                       ID3D11DeviceContext1 **d3d11_device_context)
{
    ID3D11Device *base_device = nullptr;
    ID3D11DeviceContext *base_device_context = nullptr;

    D3D_FEATURE_LEVEL kFeatureLevels[]{ D3D_FEATURE_LEVEL_11_0 };

    UINT kCreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG_BUILD)
    kCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(0, kD3DDriverType,
                                   0, kCreationFlags,
                                   kFeatureLevels, ARRAYSIZE(kFeatureLevels),
                                   D3D11_SDK_VERSION, &base_device,
                                   0, &base_device_context);
    assert(SUCCEEDED(hr));
    hr = base_device->QueryInterface(__uuidof(ID3D11Device1),
                                     reinterpret_cast<void**>(d3d11_device));
    assert(SUCCEEDED(hr));
    base_device->Release();
    hr = base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1),
                                             reinterpret_cast<void**>(d3d11_device_context));
    assert(SUCCEEDED(hr));
    base_device_context->Release();

#ifdef DEBUG_BUILD
    ID3D11Debug *d3d_debug = nullptr;
    (*d3d11_device)->QueryInterface(__uuidof(ID3D11Debug),
                                    reinterpret_cast<void**>(&d3d_debug));
    if (d3d_debug)
    {
        ID3D11InfoQueue *d3dInfoQueue = nullptr;
        if (SUCCEEDED(d3d_debug->QueryInterface(__uuidof(ID3D11InfoQueue),
                                                reinterpret_cast<void**>(&d3dInfoQueue))))
        {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3dInfoQueue->Release();
        }
        d3d_debug->Release();
    }
#endif

    return hr;
}
