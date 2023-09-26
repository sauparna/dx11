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
    create_render_target_resources();
}

KD2DSurface::~KD2DSurface()
{
    discard_render_target_resources();
    discard_device_independent_resources();
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
    // Create the render target.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hr = d2d1_factory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                               D2D1::HwndRenderTargetProperties(hwnd_,
                                                                                size_),
                                               &hwndrt_);
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Create the rest of the resources related to the render target.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hr = hwndrt_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                        &d2d1_brush_);
    assert(SUCCEEDED(hr));
    hr = hwndrt_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                        &d2d1_text_brush_);
    assert(SUCCEEDED(hr));
    ///////////////////////////////////////////////////////////////////////////////////////////
}

void KD2DSurface::discard_render_target_resources()
{
    SafeRelease(&d2d1_text_brush_);
    SafeRelease(&d2d1_brush_);
    SafeRelease(&hwndrt_);
}

void KD2DSurface::render()
{    
    HRESULT hr = S_OK;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw Direct2D content.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hwndrt_->BeginDraw();
    draw();
    hr = hwndrt_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
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
    size_ = D2D1::SizeU(rect.right, rect.bottom);

    HRESULT hr = hwndrt_->Resize(size_);
    assert(SUCCEEDED(hr));

    geometry_.resize(size_);
    textOverlay.Resize(size_);
}

void KD2DSurface::draw()
{
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Initialize.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hwndrt_->SetTransform(D2D1::Matrix3x2F::Identity());
    hwndrt_->Clear(D2D1::ColorF(D2D1::ColorF::FloralWhite));
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw geometry.
    ///////////////////////////////////////////////////////////////////////////////////////////
	for (auto ellipseIter = geometry_.ellipse_list_.begin();
         ellipseIter != geometry_.ellipse_list_.end();
         ++ellipseIter)
	{
        d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
        hwndrt_->FillEllipse(*ellipseIter, d2d1_brush_);
        d2d1_brush_->SetColor(D2D1::ColorF{D2D1::ColorF::Black, 0.5f});
        hwndrt_->DrawEllipse(*ellipseIter, d2d1_brush_, 1.f);
        d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
	}
	if (geometry_.draw_bounding_box_)
	{
		d2d1_brush_->SetColor(D2D1::ColorF{D2D1::ColorF::Gray});
		hwndrt_->DrawRectangle(geometry_.bounding_box_, d2d1_brush_, 1.f, d2d1_stroke_style_);
		d2d1_brush_->SetColor(D2D1::ColorF(D2D1::ColorF::LightPink));
	}
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Draw text.
    ///////////////////////////////////////////////////////////////////////////////////////////
    hwndrt_->DrawText(textOverlay.text.c_str(),
                      static_cast<UINT32>(wcslen(textOverlay.text.c_str())),
                      dwrite_text_format_,
                      &textOverlay.rect,
                      d2d1_text_brush_);
    ///////////////////////////////////////////////////////////////////////////////////////////
}
