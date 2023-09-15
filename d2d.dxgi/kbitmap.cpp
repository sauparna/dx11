#include "kbitmap.h"

KBitmap::KBitmap(int width, int height) : width_{width}, height_{height}
{
    stride_ = width_ * bytes_per_pixel_;
    size_ = width_ * height_;
    mem_ = new uint32_t[size_];
    clear(0xffffffff);
}

KBitmap::~KBitmap()
{
    delete [] mem_;
}

void KBitmap::draw(KScene& scene)
{
    put_pixel(static_cast<int>(scene.x_), static_cast<int>(scene.y_), 0x00000000);    
}
