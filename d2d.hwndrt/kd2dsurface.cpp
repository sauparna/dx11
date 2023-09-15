#include <cassert>
#include <cmath>
#include <random>
#include <algorithm>
#include "kd2dsurface.h"

KD2DSurface::KD2DSurface(HWND hwnd, int width, int height)
    : hwnd_{hwnd},
      surface_width_{width},
      surface_height_{height},
      kbitmap{std::min(width, height) / 2, std::min(width, height) / 2},
      scene{std::min(width, height) / 2, std::min(width, height) / 2}
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

void KD2DSurface::create_device_independent_resources()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1_factory_);
    assert(SUCCEEDED(hr));
    create_wic_resources();
    create_text_resources();
}

void KD2DSurface::discard_device_independent_resources()
{
    discard_text_resources();
    discard_wic_resources();
    SafeRelease(&d2d1_factory_);
}

void KD2DSurface::create_device_dependent_resources()
{
}

void KD2DSurface::discard_device_dependent_resources()
{   
    SafeRelease(&d2d1_bitmap_);
    SafeRelease(&d2d1_bitmap_from_wic_);
    SafeRelease(&d2d1_text_brush_);
    SafeRelease(&hwndrt_);
}

void KD2DSurface::create_render_target_resources()
{
    HRESULT hr = S_OK;

    hr = d2d1_factory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                               D2D1::HwndRenderTargetProperties(
                                                   hwnd_,
                                                   D2D1::SizeU(surface_width_, surface_height_)),
                                               &hwndrt_);
    assert(SUCCEEDED(hr));
    
    hr = hwndrt_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                        &d2d1_text_brush_);
    assert(SUCCEEDED(hr));
    
    hr = hwndrt_->CreateBitmapFromWicBitmap(wic_converter_,
                                            nullptr,
                                            &d2d1_bitmap_from_wic_);
    assert(SUCCEEDED(hr));

    D2D1_BITMAP_PROPERTIES bp = D2D1::BitmapProperties();
    bp.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                       D2D1_ALPHA_MODE_IGNORE);
    hr = hwndrt_->CreateBitmap(D2D1::SizeU(kbitmap.width(), kbitmap.height()),
                               kbitmap.data(),
                               kbitmap.stride(),
                               bp,
                               &d2d1_bitmap_);
    assert(SUCCEEDED(hr));
}

void KD2DSurface::discard_render_target_resources()
{
    SafeRelease(&d2d1_bitmap_);
    SafeRelease(&d2d1_bitmap_from_wic_);
    SafeRelease(&d2d1_text_brush_);
    SafeRelease(&hwndrt_);
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
    SafeRelease(&wic_converter_);
    SafeRelease(&wic_factory_);
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
                                      72.0f, L"en-us",
                                      &dwrite_text_format_);
    dwrite_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    dwrite_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void KD2DSurface::discard_text_resources()
{
    SafeRelease(&dwrite_text_format_);
    SafeRelease(&dwrite_factory_);
}
    
void KD2DSurface::render()
{
    HRESULT hr = S_OK;

    kbitmap.draw(scene);

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Copy the bitmap contents from memory to the Direct2D bitmap.
    ///////////////////////////////////////////////////////////////////////////////////////////
    D2D1_POINT_2U dest_point = D2D1::Point2U((surface_width_  - kbitmap.width()) / 2,
                                             (surface_height_ - kbitmap.height()) / 2);
    D2D1_RECT_U dest_rect = D2D1::RectU(dest_point.x,
                                        dest_point.y,
                                        dest_point.x + kbitmap.width(),
                                        dest_point.y + kbitmap.height());
    hr = d2d1_bitmap_->CopyFromMemory(&dest_rect, kbitmap.data(), kbitmap.stride());
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hwndrt_->BeginDraw();
    hwndrt_->Clear(D2D1::ColorF(D2D1::ColorF::FloralWhite));
    hwndrt_->DrawText(kText,
                      static_cast<UINT32>(wcslen(kText)),
                      dwrite_text_format_,
                      &kTextRect,
                      d2d1_text_brush_);
    hwndrt_->DrawBitmap(d2d1_bitmap_from_wic_,
                        D2D1::RectF(50.f,
                                    10.f,
                                    50.f + static_cast<float>(bitmap_width_),
                                    10.f + static_cast<float>(bitmap_height_)));
    hwndrt_->DrawBitmap(d2d1_bitmap_,
                        D2D1::RectF(50.f + 130.f,
                                    10.f,
                                    50.f + 130.f + 100.f,
                                    10.f + 100.f));
    hr = hwndrt_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        discard_device_dependent_resources();
        device_lost_ = true;
        return;
    }
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////
}

void KD2DSurface::resize()
{
    RECT rect{};
    GetClientRect(hwnd_, &rect);
    surface_width_ = rect.right;
    surface_height_ = rect.bottom;

    // REWRITE: Do you need to discard/recreate when using a HWND RT?
    // discard_render_target_resources();
    HRESULT hr = hwndrt_->Resize(D2D1::SizeU(surface_width_, surface_height_));
    assert(SUCCEEDED(hr));
    // create_render_target_resources();
}

void KD2DSurface::update()
{
    scene.update();
}

void KD2DSurface::write_wic_bitmap(std::wstring filename)
{
    HRESULT hr = S_OK;
    
    IWICBitmapEncoder *wic_encoder;
    hr = wic_factory_->CreateEncoder(GUID_ContainerFormatPng, NULL, &wic_encoder);
    assert(SUCCEEDED(hr));

    IWICStream *wic_stream;
    hr = wic_factory_->CreateStream(&wic_stream);
    assert(SUCCEEDED(hr));
    hr = wic_stream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE);
    assert(SUCCEEDED(hr));
    hr = wic_encoder->Initialize(wic_stream, WICBitmapEncoderNoCache);
    assert(SUCCEEDED(hr));

    IWICBitmapFrameEncode *wic_frame;
    hr = wic_encoder->CreateNewFrame(&wic_frame, NULL);
    assert(SUCCEEDED(hr));
    hr = wic_frame->Initialize(NULL);
    assert(SUCCEEDED(hr));
    hr = wic_frame->SetSize(100, 100);
    assert(SUCCEEDED(hr));

    WICPixelFormatGUID pixel_format = GUID_WICPixelFormatDontCare;
    hr = wic_frame->SetPixelFormat(&pixel_format);
    assert(SUCCEEDED(hr));
    hr = wic_frame->WriteSource(wic_bitmap_, NULL);
    assert(SUCCEEDED(hr));
    hr = wic_frame->Commit();
    assert(SUCCEEDED(hr));
    hr = wic_encoder->Commit();    
    assert(SUCCEEDED(hr));
}
