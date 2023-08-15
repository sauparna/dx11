#include "kgraphics.h"
#include "kd2dsurface.h"

using namespace std;
using namespace kdx;

KD2DSurface::KD2DSurface(HWND hwnd, D2D1_SIZE_U sz)
    : hwnd_{hwnd}, surface_size_{sz}
{
    create_device_independent_resources();
    create_device_dependent_resources();
    create_render_target_resources();
}

KD2DSurface::~KD2DSurface()
{
    discard_device_dependent_resources();
    discard_device_independent_resources();
}

HRESULT KD2DSurface::d3d_create_device(D3D_DRIVER_TYPE const driver_type, ID3D11Device *&d3d_device)
{
    UINT kD3DCreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    kD3DCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL kD3DFeatureLevels[7] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
    ID3D11DeviceContext *d3d_context{};
    HRESULT hr = D3D11CreateDevice(nullptr,
                                   driver_type,
                                   0,
                                   kD3DCreationFlags,
                                   kD3DFeatureLevels,
                                   ARRAYSIZE(kD3DFeatureLevels),
                                   D3D11_SDK_VERSION,
                                   &d3d_device,
                                   NULL,
                                   &d3d_context);
    d3d_context->Release();
    return hr;
}

void KD2DSurface::create_device_independent_resources()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory_);
    assert(SUCCEEDED(hr));
    create_wic_resources();
}

void KD2DSurface::discard_device_independent_resources()
{
    discard_wic_resources();
    d2d_factory_->Release();
}

void KD2DSurface::create_device_dependent_resources()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the Direct3D device.
    ///////////////////////////////////////////////////////////////////////////////////////////

    hr = d3d_create_device(D3D_DRIVER_TYPE_HARDWARE, d3d_device_);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        hr = d3d_create_device(D3D_DRIVER_TYPE_WARP, d3d_device_);
    }
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the swap chain.
    ///////////////////////////////////////////////////////////////////////////////////////////

    IDXGIDevice1 *dxgi_device{};
    hr = d3d_device_->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgi_device));
    assert(SUCCEEDED(hr));

    IDXGIAdapter *dxgi_adapter{};
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    assert(SUCCEEDED(hr));

    IDXGIFactory2 *dxgi_factory{};
    hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));
    dxgi_adapter->Release();
    assert(SUCCEEDED(hr));

    DXGI_SWAP_CHAIN_DESC1 dxgi_swap_chain_desc{};
    dxgi_swap_chain_desc.Width = surface_size_.width;
    dxgi_swap_chain_desc.Height = surface_size_.height;
    dxgi_swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    dxgi_swap_chain_desc.Stereo = false;
    dxgi_swap_chain_desc.SampleDesc.Count = 1;
    dxgi_swap_chain_desc.SampleDesc.Quality = 0;
    dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_swap_chain_desc.BufferCount = 2;
    dxgi_swap_chain_desc.Scaling = DXGI_SCALING_NONE;
    dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    dxgi_swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    dxgi_swap_chain_desc.Flags = 0;
    hr = dxgi_factory->CreateSwapChainForHwnd(d3d_device_,
                                              hwnd_,
                                              &dxgi_swap_chain_desc,
                                              nullptr,
                                              nullptr,
                                              &dxgi_swap_chain_);
    dxgi_factory->Release();
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the Direct2D device-context.
    ///////////////////////////////////////////////////////////////////////////////////////////

    ID2D1Device1 *d2d_device{};
    hr = d2d_factory_->CreateDevice(dxgi_device, &d2d_device);
    dxgi_device->Release();
    assert(SUCCEEDED(hr));

    hr =  d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                          &d2d_device_context_);
    d2d_device->Release();
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the rest of the Direct2D device-dependent resources.
    ///////////////////////////////////////////////////////////////////////////////////////////

    create_bitmap_resources();
}

void KD2DSurface::discard_device_dependent_resources()
{
    discard_bitmap_resources();
    d2d_dxgi_bitmap_->Release();
    d2d_device_context_->Release();
    dxgi_swap_chain_->Release();
    d3d_device_->Release();
}

void KD2DSurface::create_render_target_resources()
{
    HRESULT hr = S_OK;

    IDXGISurface *dxgi_surface;
    hr = dxgi_swap_chain_->GetBuffer(0, IID_PPV_ARGS(&dxgi_surface));
    assert(SUCCEEDED(hr));
    D2D1_BITMAP_PROPERTIES1 d2d_dxgi_bitmap_prop = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f);
    hr = d2d_device_context_->CreateBitmapFromDxgiSurface(dxgi_surface,
                                                          &d2d_dxgi_bitmap_prop,
                                                          &d2d_dxgi_bitmap_);
    dxgi_surface->Release();
    assert(SUCCEEDED(hr));
    d2d_device_context_->SetTarget(d2d_dxgi_bitmap_);
}

void KD2DSurface::discard_render_target_resources()
{
    d2d_device_context_->SetTarget(nullptr);
    d2d_dxgi_bitmap_->Release();
}

void KD2DSurface::resize()
{
    discard_render_target_resources();
    HRESULT hr = dxgi_swap_chain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    assert(SUCCEEDED(hr));
    create_render_target_resources();
    RECT rect;
    GetClientRect(hwnd_, &rect);
    surface_size_.width = rect.right;
    surface_size_.height = rect.bottom;
}

void KD2DSurface::render()
{
    HRESULT hr = S_OK;
    if (device_lost_)
    {
        create_device_dependent_resources();
        create_render_target_resources();
        device_lost_ = false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Copy the bitmap contents to the target-bitmap.
    ///////////////////////////////////////////////////////////////////////////////////////////
    D2D1_POINT_2U dst_point = D2D1::Point2U(0, 0);
    D2D1_RECT_U src_rect = D2D1::RectU(0, 0, bitmap_width_, bitmap_height_);
    hr = d2d_dxgi_bitmap_->CopyFromBitmap(&dst_point, d2d_bitmap_, &src_rect);
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Use Direct2D to draw the bitmap on the render target.
    ///////////////////////////////////////////////////////////////////////////////////////////
    // d2d_device_context_->BeginDraw();
    // d2d_device_context_->DrawBitmap(d2d_bitmap_,
    //                                 D2D1::RectF(10.0,
    //                                             10.0,
    //                                             10.0 + (FLOAT)bitmap_width_,
    //                                             10.0 + (FLOAT)bitmap_height_));
    // hr = d2d_device_context_->EndDraw();
    // if (hr == D2DERR_RECREATE_TARGET)
    // {
    //     discard_device_dependent_resources();
    //     device_lost_ = true;
    //     return;
    // }
    ///////////////////////////////////////////////////////////////////////////////////////////

    hr = dxgi_swap_chain_->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        discard_device_dependent_resources();
        device_lost_ = true;
        return;
    }
    assert(SUCCEEDED(hr));
}

void KD2DSurface::create_wic_resources()
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
    // hr = wic_converter_->GetResolution(&dpix, &dpiy);
    // assert(SUCCEEDED(hr));
}

void KD2DSurface::discard_wic_resources()
{
    wic_converter_->Release();
    wic_factory_->Release();
}

void KD2DSurface::create_bitmap_resources()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a WIC bitmap and write to bitmap memory.
    ///////////////////////////////////////////////////////////////////////////////////////////

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

    ZeroMemory(mem, sz/4);
    lock->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create D2D bitmap from WIC bitmap and write to that bitmap's memory.
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    D2D1_BITMAP_PROPERTIES1 d2d_bitmap_prop = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f, 96.0f);

    hr = d2d_device_context_->CreateBitmapFromWicBitmap(wic_bitmap_,
                                                        &d2d_bitmap_prop,
                                                        &d2d_bitmap_);
    assert(SUCCEEDED(hr));

    D2D1_MAPPED_RECT d2d1_mapped_rect{};
    hr = d2d_bitmap_->Map(D2D1_MAP_OPTIONS_READ, &d2d1_mapped_rect);
    assert(SUCCEEDED(hr));

    ZeroMemory(d2d1_mapped_rect.bits, d2d1_mapped_rect.pitch * (bitmap_height_ - 64));

    hr = d2d_bitmap_->Unmap();
    assert(SUCCEEDED(hr));

    ///////////////////////////////////////////////////////////////////////////////////////////
    // REWRITE: Another bitmap is probably needed to write the image to a file on disk.
    ///////////////////////////////////////////////////////////////////////////////////////////
    // hr = wic_factory_->CreateBitmap(100, 100,
    //                                      GUID_WICPixelFormat32bppBGR,
    //                                      WICBitmapCacheOnLoad,
    //                                      &wic_bitmap_);
    // assert(SUCCEEDED(hr));    
}

void KD2DSurface::discard_bitmap_resources()
{
    wic_bitmap_->Release();
    d2d_bitmap_->Release();
}

D2D1_SIZE_U KD2DSurface::surface_size() const
{
    return surface_size_;
}
