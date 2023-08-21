#include <string>
#include <cassert>
#include "kmath.h"
#include "kd3dsurface.h"
#include "kobjloader.h"

KD3DSurface::KD3DSurface(HWND hwnd, int width, int height)
    : hwnd_{hwnd}, surface_width_{width}, surface_height_{height}
{
    create_device_independent_resources();
    create_device_dependent_resources();
    create_render_target_resources();
#if defined(DEBUG_BUILD)
    enable_d3d_debugging(&d3d11_device_);
#endif
}

KD3DSurface::~KD3DSurface()
{
    discard_device_dependent_resources();
    discard_device_independent_resources();
}

void KD3DSurface::enable_d3d_debugging(ID3D11Device1 **d3d11_device)
{
    if (*d3d11_device == nullptr) return;
    
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

    d3d11_runtime_layers_ |= D3D11_CREATE_DEVICE_DEBUG;
    d3d11_shader_compile_options_ |= D3DCOMPILE_DEBUG;    // Enable shader-debugging in Visual Studio.
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

    build_geometry();
    create_vertex_shader();
    create_pixel_shader();
    create_input_layout();
    create_constant_buffer();
    create_sampler_state();
    create_texture();
    create_rasterizer_state();
    create_depth_stencil_state();
}

void KD3DSurface::discard_device_dependent_resources()
{
    SafeRelease(&d3d11_depth_stencil_state_);
    SafeRelease(&rasterizer_state_);
    SafeRelease(&constant_buffer_);
    SafeRelease(&index_buffer_);
    SafeRelease(&vertex_buffer_);
    SafeRelease(&input_layout_);
    SafeRelease(&pixel_shader_);
    SafeRelease(&ps_blob_);
    SafeRelease(&vertex_shader_);
    SafeRelease(&vs_blob_);
    SafeRelease(&d3d11_frame_buffer_view_);
    SafeRelease(&d3d11_depth_buffer_view_);
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
    
    ID3D11Texture2D *d3d11_frame_buffer{};
    HRESULT hr = dxgi_swap_chain_->GetBuffer(0,
                                             __uuidof(ID3D11Texture2D),
                                             reinterpret_cast<void**>(&d3d11_frame_buffer));
    assert(SUCCEEDED(hr));

    hr = d3d11_device_->CreateRenderTargetView(d3d11_frame_buffer,
                                               nullptr,
                                               &d3d11_frame_buffer_view_);
    assert(SUCCEEDED(hr));

    D3D11_TEXTURE2D_DESC td{};
    d3d11_frame_buffer->GetDesc(&td);

    d3d11_frame_buffer->Release();

    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D *depth_buffer{};

    d3d11_device_->CreateTexture2D(&td, nullptr, &depth_buffer);
    d3d11_device_->CreateDepthStencilView(depth_buffer, nullptr, &d3d11_depth_buffer_view_);

    depth_buffer->Release();
}

void KD3DSurface::discard_render_target_resources()
{
    d3d11_device_context_->OMSetRenderTargets(0, 0, 0);
    SafeRelease(&d3d11_depth_buffer_view_);
    SafeRelease(&d3d11_frame_buffer_view_);
}

void KD3DSurface::create_vertex_shader()
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl",
                                    nullptr,
                                    nullptr,
                                    "vs_main",
                                    "vs_5_0",
                                    d3d11_shader_compile_options_,
                                    0,
                                    &vs_blob_,
                                    &shader_compiler_error_blob);
    assert(shader_compiler_succeeded(hr, shader_compiler_error_blob));
    hr = d3d11_device_->CreateVertexShader(vs_blob_->GetBufferPointer(),
                                           vs_blob_->GetBufferSize(),
                                           nullptr,
                                           &vertex_shader_);
    assert(SUCCEEDED(hr));
}

void KD3DSurface::create_pixel_shader()
{
    ID3DBlob *shader_compiler_error_blob = nullptr;
    HRESULT hr = D3DCompileFromFile(L"shaders.hlsl",
                                    nullptr,
                                    nullptr,
                                    "ps_main",
                                    "ps_5_0",
                                    d3d11_shader_compile_options_,
                                    0,
                                    &ps_blob_,
                                    &shader_compiler_error_blob);
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
    D3D11_INPUT_ELEMENT_DESC kInputElementDesc[] = {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    HRESULT hr = d3d11_device_->CreateInputLayout(kInputElementDesc,
                                                  ARRAYSIZE(kInputElementDesc),
                                                  vs_blob_->GetBufferPointer(),
                                                  vs_blob_->GetBufferSize(),
                                                  &input_layout_);
    assert(SUCCEEDED(hr));
}

void KD3DSurface::build_geometry()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // triangle: x, y, u, v
    ///////////////////////////////////////////////////////////////////////////////////////////
    // float kVertexData[]{  0.0f,  0.5f, 0.0f, 0.0f,
    //                       0.5f, -0.5f, 1.0f, 1.0f,
    //                      -0.5f, -0.5f, 0.0f, 1.0f
    // };
    // stride_ = 4 * sizeof(float);
    // nvertex_ = sizeof(kVertexData) / stride_;
    // offset_ = 0;
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // cube: x, y, z
    ///////////////////////////////////////////////////////////////////////////////////////////
    // float kVertexData[]{ -0.5f, -0.5f, -0.5f,
    //                      -0.5f, -0.5f,  0.5f,
    //                      -0.5f,  0.5f, -0.5f,
    //                      -0.5f,  0.5f,  0.5f,
    //                       0.5f, -0.5f, -0.5f,
    //                       0.5f, -0.5f,  0.5f,
    //                       0.5f,  0.5f, -0.5f,
    //                       0.5f,  0.5f,  0.5f};
    // stride_ = 3 * sizeof(float);
    // nvertex_ = sizeof(kVertexData) / stride_;
    // offset_ = 0;
    ///////////////////////////////////////////////////////////////////////////////////////////

    // D3D11_BUFFER_DESC d3d11_vertex_buffer_desc{};
    // d3d11_vertex_buffer_desc.ByteWidth = sizeof(kVertexData);
    // d3d11_vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    // d3d11_vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    // D3D11_SUBRESOURCE_DATA d3d11_vertex_subresource_data{kVertexData};
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Read in OBJ data.
    ///////////////////////////////////////////////////////////////////////////////////////////
    KOBJBlob objb = load_obj("3dmodel.obj");
    stride_ = sizeof(VertexData);
    // nvertex_ = objb.numVertices;
    offset_ = 0;
    nindex_ = objb.numIndices;
    ///////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC d3d11_vertex_buffer_desc{};
    d3d11_vertex_buffer_desc.ByteWidth = objb.numVertices * sizeof(VertexData);
    d3d11_vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    d3d11_vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA d3d11_vertex_subresource_data{objb.vertexBuffer};

    HRESULT hr = d3d11_device_->CreateBuffer(&d3d11_vertex_buffer_desc,
                                             &d3d11_vertex_subresource_data,
                                             &vertex_buffer_);
    assert(SUCCEEDED(hr));

    // uint16_t kIndexData[]{0, 6, 4,
    //                       0, 2, 6, 
    //                       0, 3, 2,
    //                       0, 1, 3,
    //                       2, 7, 6,
    //                       2, 3, 7,
    //                       4, 6, 7,
    //                       4, 7, 5,
    //                       0, 4, 5,
    //                       0, 5, 1,
    //                       1, 5, 7, 
    //                       1, 7, 3};
    // nindex_ = sizeof(kIndexData) / sizeof(kIndexData[0]);
    
    // D3D11_BUFFER_DESC d3d11_index_buffer_desc{};
    // d3d11_index_buffer_desc.ByteWidth = sizeof(kIndexData);
    // d3d11_index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    // d3d11_index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    // D3D11_SUBRESOURCE_DATA d3d11_index_subresource_data{kIndexData};

    D3D11_BUFFER_DESC d3d11_index_buffer_desc{};
    d3d11_index_buffer_desc.ByteWidth = objb.numIndices * sizeof(uint16_t);
    d3d11_index_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    d3d11_index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA d3d11_index_subresource_data{objb.indexBuffer};
    
    hr = d3d11_device_->CreateBuffer(&d3d11_index_buffer_desc,
                                     &d3d11_index_subresource_data,
                                     &index_buffer_);
    assert(SUCCEEDED(hr));
    
    free_obj(objb);
}

void KD3DSurface::create_constant_buffer()
{
    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(KD3DConstantsDataStructure) + 0xf & 0xfffffff0; // Enforce multiple-of-16.
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = d3d11_device_->CreateBuffer(&cbd, nullptr, &constant_buffer_);
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

void KD3DSurface::create_rasterizer_state()
{
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = TRUE;
    d3d11_device_->CreateRasterizerState(&rd, &rasterizer_state_);
}

void KD3DSurface::create_depth_stencil_state()
{
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    d3d11_device_->CreateDepthStencilState(&dsd, &d3d11_depth_stencil_state_);
}

void KD3DSurface::render(KClock& clock)
{
    HRESULT hr = S_OK;
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Update the camera and calculate the model-view-projection-matrix.
    ///////////////////////////////////////////////////////////////////////////////////////////

    camera_.update(clock);
    view_matrix_ = translation_matrix(-camera_.pos) * rotation_y_matrix(-camera_.yaw) * rotation_x_matrix(-camera_.pitch);
    camera_.fwd = {-view_matrix_.m[2][0], -view_matrix_.m[2][1], -view_matrix_.m[2][2]};
    model_matrix_ = rotation_x_matrix(-0.2f * static_cast<float>(K_PI * clock.t)) *
                    rotation_y_matrix( 0.1f * static_cast<float>(K_PI * clock.t));
    mvp_matrix_ = model_matrix_ * view_matrix_ * perspective_matrix_;
        
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Update world state.
    ///////////////////////////////////////////////////////////////////////////////////////////

    // world_state_.update(clock);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Write the changes to the constant-buffer.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    d3d11_device_context_->Map(constant_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
    KD3DConstantsDataStructure *kd3d11_constants = reinterpret_cast<KD3DConstantsDataStructure*>(mapped_subresource.pData);
    // kd3d11_constants->pos = world_state_.obj_pos;
    // kd3d11_constants->color = world_state_.obj_color;
    kd3d11_constants->model_view_projection_matrix = mvp_matrix_;
    d3d11_device_context_->Unmap(constant_buffer_, 0);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // RS
    ///////////////////////////////////////////////////////////////////////////////////////////

    // REWRITE: surface dimensions are already updated by resize()
    // which is guaranteed to be called first, before the first call
    // to render().
    //
    // RECT window_rect{};
    // GetClientRect(hwnd_, &window_rect);
    // D3D11_VIEWPORT d3d11_viewport{0.0f, 0.0f,
    //                               (FLOAT)(window_rect.right - window_rect.left),
    //                               (FLOAT)(window_rect.bottom - window_rect.top),
    //                               0.0f, 1.0f
    // };
    D3D11_VIEWPORT d3d11_viewport{0.0f, 0.0f,
                                  static_cast<FLOAT>(surface_width_),
                                  static_cast<FLOAT>(surface_height_),
                                  0.0f, 1.0f
    };
    d3d11_device_context_->RSSetViewports(1, &d3d11_viewport);
    d3d11_device_context_->RSSetState(rasterizer_state_);

    ///////////////////////////////////////////////////////////////////////////////////////////

    FLOAT kBackgroundColor[4]{ 0.2f, 0.4f, 0.8f, 1.0f };
    d3d11_device_context_->ClearRenderTargetView(d3d11_frame_buffer_view_, kBackgroundColor);
    d3d11_device_context_->ClearDepthStencilView(d3d11_depth_buffer_view_, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // OM
    ///////////////////////////////////////////////////////////////////////////////////////////
    d3d11_device_context_->OMSetDepthStencilState(d3d11_depth_stencil_state_, 0);
    d3d11_device_context_->OMSetRenderTargets(1, &d3d11_frame_buffer_view_, d3d11_depth_buffer_view_);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // IA
    ///////////////////////////////////////////////////////////////////////////////////////////

    d3d11_device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d11_device_context_->IASetInputLayout(input_layout_);
    d3d11_device_context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride_, &offset_);
    d3d11_device_context_->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R16_UINT, 0);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // VS & PS; set shaders and constant buffers.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    d3d11_device_context_->VSSetShader(vertex_shader_, nullptr, 0);
    d3d11_device_context_->VSSetConstantBuffers(0, 1, &constant_buffer_);
    d3d11_device_context_->PSSetShader(pixel_shader_, nullptr, 0);
    // d3d11_device_context_->PSSetShaderResources(0, 1, &texture_view_);
    // d3d11_device_context_->PSSetSamplers(0, 1, &sampler_state_);
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw() and then Present().
    ///////////////////////////////////////////////////////////////////////////////////////////

    // d3d11_device_context_->Draw(nvertex_, 0);
    d3d11_device_context_->DrawIndexed(nindex_, 0, 0);
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

    DXGI_SWAP_CHAIN_DESC1 scd{};
    hr = dxgi_swap_chain_->GetDesc1(&scd);
    assert(SUCCEEDED(hr));
    surface_width_ = scd.Width;
    surface_height_ = scd.Height;
    if (surface_height_ != 0)
        surface_aspect_ratio_ = static_cast<float>(surface_width_) / static_cast<float>(surface_height_);

    perspective_matrix_ = make_perspective_matrix(surface_aspect_ratio_, degrees_to_radians(84), 0.1f, 1000.f);
}

// REWRITE: Consider not passing in the device and context placeholders.
HRESULT KD3DSurface::create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                                       ID3D11Device1 **d3d11_device,
                                       ID3D11DeviceContext1 **d3d11_device_context)
{
    ID3D11Device *base_device = nullptr;
    ID3D11DeviceContext *base_device_context = nullptr;

    D3D_FEATURE_LEVEL kFeatureLevels[]{ D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDevice(0, kD3DDriverType,
                                   0, d3d11_runtime_layers_,
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

    return hr;
}
