#pragma once

#include <string>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include "kbitmap.h"
#include "kscene.h"
#include "kclock.h"

class KD2DSurface
{
public:
    KD2DSurface(HWND hwnd, int width, int height);
    ~KD2DSurface();
    void create_device_independent_resources();
    void discard_device_independent_resources();
    void create_device_dependent_resources();
    void discard_device_dependent_resources();
    void create_render_target_resources();
    void discard_render_target_resources();

    void create_wic_resources();
    void discard_wic_resources();
    void write_wic_bitmap(std::wstring filename);

    void create_text_resources();
    void discard_text_resources();

    void resize();
    void render();
    void update();
    bool device_lost_{false};
    bool window_resized_{true};

protected:

    KBitmap kbitmap_;
    KScene scene_;
    KClock clock_{};

    ID2D1Factory *d2d1_factory_;
    ID2D1HwndRenderTarget *hwndrt_;
    ID2D1Bitmap *d2d1_bitmap_;

    ///////////////////////////////////////////////////////////////////////////////////////////
    // WIC components.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IWICImagingFactory2 *wic_factory_{};
    IWICFormatConverter *wic_converter_{};
    IWICBitmap *wic_bitmap_{};
    ID2D1Bitmap *d2d1_bitmap_from_wic_{};
    std::wstring img_filename_{L"tintin_on_train.jpg"};
    unsigned bitmap_width_{};
    unsigned bitmap_height_{};
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Text components.
    ///////////////////////////////////////////////////////////////////////////////////////////
    IDWriteFactory *dwrite_factory_{};
    IDWriteTextFormat *dwrite_text_format_{};
    ID2D1SolidColorBrush *d2d1_text_brush_{};
    ///////////////////////////////////////////////////////////////////////////////////////////

    HWND hwnd_{};
    D2D1_SIZE_U surface_size_{1, 1};
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
