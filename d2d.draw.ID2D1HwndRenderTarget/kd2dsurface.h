#pragma once

#include <string>
#include <d2d1_2.h>
#include <dwrite.h>

#include "kgeometry.h"
#include "ktextoverlay.h"

class KD2DSurface
{
public:
    KD2DSurface(HWND hwnd,
                uint32_t width,
                uint32_t height,
                KGeometry &geometry,
                KTextOverlay &textOverlay);
    ~KD2DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();

    void create_text_resources();
    void discard_text_resources();

    void resize();
    void render();
    void draw();
    bool device_lost_{false};
    bool window_resized_{true};

protected:
    KGeometry &geometry_;
    KTextOverlay &textOverlay;

    ID2D1Factory2 *d2d1_factory_{};
    ID2D1HwndRenderTarget *hwndrt_;

    ID2D1SolidColorBrush *d2d1_brush_{};
    ID2D1StrokeStyle *d2d1_stroke_style_{};
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Text components.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IDWriteFactory *dwrite_factory_{};
    IDWriteTextFormat *dwrite_text_format_{};
    ID2D1SolidColorBrush *d2d1_text_brush_{};
    ///////////////////////////////////////////////////////////////////////////////////////////

    HWND hwnd_{};
    D2D1_SIZE_U size_{1, 1};
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
