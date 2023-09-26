#include <string>
#include <random>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "kd2dsurface.h"

KD2DSurface::KD2DSurface(HWND hwnd,
                         uint32_t width,
                         uint32_t height,
                         KGeometry &geometry,
                         KTextOverlay &textOverlay)
    : hwnd_{hwnd},
      size_{width, height},
      geometry_{geometry},
      textOverlay{textOverlay}
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

void KD2DSurface::initialize_d3d11_debug_layer(ID3D11Device1 **d3d11_device)
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
            SafeRelease(&d3dInfoQueue);
        }
        SafeRelease(&d3d_debug);
    }
}

void KD2DSurface::create_device_independent_resources()
{
    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1_factory_);
    assert(SUCCEEDED(hr));

    D2D1_STROKE_STYLE_PROPERTIES strokeStyleProperties = D2D1::StrokeStyleProperties(
		D2D1_CAP_STYLE_FLAT,		// The start cap
		D2D1_CAP_STYLE_FLAT,		// The end cap
		D2D1_CAP_STYLE_FLAT,		// The dash cap
		D2D1_LINE_JOIN_MITER,		// The line join
		10.0f,						// The miter limit
		D2D1_DASH_STYLE_CUSTOM,		// The dash style
		0.0f						// The dash offset
        );

	// A 'dash blank ... ' pattern where dash is a line of length 5
	// blank is a space of length 3
	float dashes[] = {5.0f, 3.0f};    

    hr = d2d1_factory_->CreateStrokeStyle(
		strokeStyleProperties,
		dashes,
		ARRAYSIZE(dashes),
		&d2d1_stroke_style_);
    assert(SUCCEEDED(hr));

    create_text_resources();
}

void KD2DSurface::discard_device_independent_resources()
{
    discard_text_resources();
    SafeRelease(&d2d1_stroke_style_);
    SafeRelease(&d2d1_factory_);
}

void KD2DSurface::create_device_dependent_resources()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the D3D device.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hr = create_d3d_device(D3D_DRIVER_TYPE_HARDWARE, &d3d11_device_, &d3d11_device_context_);
    if (hr == DXGI_ERROR_UNSUPPORTED)
    {
        hr = create_d3d_device(D3D_DRIVER_TYPE_WARP, &d3d11_device_, &d3d11_device_context_);
    }
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the DXGI swap chain.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IDXGIFactory2 *dxgi_factory{};
    IDXGIDevice1 *dxgi_device{};
    {
        hr = d3d11_device_->QueryInterface(__uuidof(IDXGIDevice1),
                                           reinterpret_cast<void**>(&dxgi_device));
        assert(SUCCEEDED(hr));

        IDXGIAdapter *dxgi_adapter{};
        hr = dxgi_device->GetAdapter(&dxgi_adapter);
        assert(SUCCEEDED(hr));

        DXGI_ADAPTER_DESC adapter_desc;
        dxgi_adapter->GetDesc(&adapter_desc);
        OutputDebugStringA("DXGI adapter: ");
        OutputDebugStringW(adapter_desc.Description);

        hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2),
                                     reinterpret_cast<void**>(&dxgi_factory));
        assert(SUCCEEDED(hr));
        SafeRelease(&dxgi_adapter);
    }

    DXGI_SWAP_CHAIN_DESC1 dxgi_swap_chain_desc{};
    dxgi_swap_chain_desc.Width = 0;
    dxgi_swap_chain_desc.Height = 0;
    dxgi_swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    dxgi_swap_chain_desc.SampleDesc.Count = 1;
    dxgi_swap_chain_desc.SampleDesc.Quality = 0;
    dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_swap_chain_desc.BufferCount = 2;
    dxgi_swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    dxgi_swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    dxgi_swap_chain_desc.Flags = 0;

    hr = dxgi_factory->CreateSwapChainForHwnd(d3d11_device_,
                                              hwnd_,
                                              &dxgi_swap_chain_desc,
                                              nullptr,
                                              nullptr,
                                              &dxgi_swap_chain_);
    assert(SUCCEEDED(hr));
    SafeRelease(&dxgi_factory);
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the Direct2D device-context.
    ///////////////////////////////////////////////////////////////////////////////////////////
    ID2D1Device1 *d2d1_device{};
    hr = d2d1_factory_->CreateDevice(dxgi_device, &d2d1_device);
    assert(SUCCEEDED(hr));
    SafeRelease(&dxgi_device);

    hr =  d2d1_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                           &d2d1_device_context_);
    assert(SUCCEEDED(hr));
    SafeRelease(&d2d1_device);
    ///////////////////////////////////////////////////////////////////////////////////////////
}

void KD2DSurface::discard_device_dependent_resources()
{
    SafeRelease(&d2d1_bitmap_);
    SafeRelease(&d2d1_device_context_);
    SafeRelease(&dxgi_swap_chain_);
    SafeRelease(&d3d11_device_);
}

void KD2DSurface::create_text_resources()
{
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                        __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&dwrite_factory_));
    dwrite_factory_->CreateTextFormat(L"Verdana",
                                      NULL,
                                      DWRITE_FONT_WEIGHT_NORMAL,
                                      DWRITE_FONT_STYLE_NORMAL,
                                      DWRITE_FONT_STRETCH_NORMAL,
                                      10.0f, L"en-us",
                                      &dwrite_text_format_);
    dwrite_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    dwrite_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
}

void KD2DSurface::discard_text_resources()
{
    SafeRelease(&dwrite_text_format_);
    SafeRelease(&dwrite_factory_);
}

void KD2DSurface::create_render_target_resources()
{
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create a render target.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IDXGISurface *dxgi_surface{};
    hr = dxgi_swap_chain_->GetBuffer(0, IID_PPV_ARGS(&dxgi_surface));
    assert(SUCCEEDED(hr));

    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        96.0f,
        96.0f);
    hr = d2d1_device_context_->CreateBitmapFromDxgiSurface(dxgi_surface,
                                                           &bp,
                                                           &d2d1_bitmap_);
    assert(SUCCEEDED(hr));
    SafeRelease(&dxgi_surface);
    d2d1_device_context_->SetTarget(d2d1_bitmap_);
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the rest of the resources related to the render target.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hr = d2d1_device_context_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                                     &d2d1_brush_);
    assert(SUCCEEDED(hr));
    hr = d2d1_device_context_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                                     &d2d1_text_brush_);
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////
}

void KD2DSurface::discard_render_target_resources()
{
    SafeRelease(&d2d1_text_brush_);
    SafeRelease(&d2d1_brush_);
    SafeRelease(&d2d1_bitmap_);
    d2d1_device_context_->SetTarget(nullptr);
}

void KD2DSurface::render()
{    
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw Direct2D content.
    ///////////////////////////////////////////////////////////////////////////////////////////
    d2d1_device_context_->BeginDraw();
    draw();
    hr = d2d1_device_context_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        device_lost_ = true;
        return;
    }
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////

    hr = dxgi_swap_chain_->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        device_lost_ = true;
        return;
    }
    assert(SUCCEEDED(hr));
}

void KD2DSurface::resize()
{
    discard_render_target_resources();
    HRESULT hr = dxgi_swap_chain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    assert(SUCCEEDED(hr));
    create_render_target_resources();

    DXGI_SWAP_CHAIN_DESC1 scd{};
    hr = dxgi_swap_chain_->GetDesc1(&scd);
    assert(SUCCEEDED(hr));
    size_ = D2D1::SizeU(scd.Width, scd.Height);
    geometry_.resize(size_);
    textOverlay.Resize(size_);
}

void KD2DSurface::draw()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Initialize.
    ///////////////////////////////////////////////////////////////////////////////////////////
    d2d1_device_context_->SetTransform(D2D1::Matrix3x2F::Identity());
    d2d1_device_context_->Clear(D2D1::ColorF(D2D1::ColorF::FloralWhite));
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw geometry.
    ///////////////////////////////////////////////////////////////////////////////////////////
	for (auto ellipseIter = geometry_.ellipse_list_.begin();
         ellipseIter != geometry_.ellipse_list_.end();
         ++ellipseIter)
	{
        d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
        d2d1_device_context_->FillEllipse(*ellipseIter, d2d1_brush_);
        d2d1_brush_->SetColor(D2D1::ColorF{D2D1::ColorF::Black, 0.5f});
        d2d1_device_context_->DrawEllipse(*ellipseIter, d2d1_brush_, 1.f);
        d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
	}
	if (geometry_.draw_bounding_box_)
	{
		d2d1_brush_->SetColor(D2D1::ColorF{D2D1::ColorF::Gray});
		d2d1_device_context_->DrawRectangle(geometry_.bounding_box_, d2d1_brush_, 1.f, d2d1_stroke_style_);
		d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightPink));
	}
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw text.
    ///////////////////////////////////////////////////////////////////////////////////////////
    d2d1_device_context_->DrawText(textOverlay.text.c_str(),
                                    static_cast<UINT32>(wcslen(textOverlay.text.c_str())),
                                    dwrite_text_format_,
                                    &textOverlay.rect,
                                    d2d1_text_brush_);
    ///////////////////////////////////////////////////////////////////////////////////////////
}

HRESULT KD2DSurface::create_d3d_device(D3D_DRIVER_TYPE const kD3DDriverType,
                                       ID3D11Device1 **d3d11_device,
                                       ID3D11DeviceContext1 **d3d11_device_context)
{
    ID3D11Device *base_device{};
    ID3D11DeviceContext *base_device_context{};
    
    D3D_FEATURE_LEVEL kFeatureLevels[]{D3D_FEATURE_LEVEL_11_0};
    
#if defined(DEBUG_BUILD)
    d3d11_runtime_layers_ |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(0,
                                   kD3DDriverType,
                                   0,
                                   d3d11_runtime_layers_,
                                   kFeatureLevels,
                                   ARRAYSIZE(kFeatureLevels),
                                   D3D11_SDK_VERSION,
                                   &base_device,
                                   0,
                                   &base_device_context);
    assert(SUCCEEDED(hr));
    hr = base_device->QueryInterface(__uuidof(ID3D11Device1),
                                     reinterpret_cast<void**>(d3d11_device));
    assert(SUCCEEDED(hr));
    SafeRelease(&base_device);
    hr = base_device_context->QueryInterface(__uuidof(ID3D11DeviceContext1),
                                             reinterpret_cast<void**>(d3d11_device_context));
    assert(SUCCEEDED(hr));
    SafeRelease(&base_device_context);

#if defined(DEBUG_BUILD)
    initialize_d3d11_debug_layer(&d3d11_device_);
#endif
    
    return hr;
}
