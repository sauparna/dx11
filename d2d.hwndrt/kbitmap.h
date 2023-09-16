#pragma once

#include "kscene.h"

class KBitmap
{
public:
    KBitmap(int width, int height);
    ~KBitmap();

    // 32-bit color byte layout: 0xaarrggbb.
    // Where aa = alpha, rr = red, bb = green, gg = blue.
    void put_pixel(int x, int y, uint32_t color);
    void clear(uint32_t color);
    int width();
    int height();
    int stride();
    int size();
    uint32_t* data();
    void draw(KScene& scene);
    
private:
    int width_{128};
    int height_{128};
    int bytes_per_pixel_{4};
    int stride_{};
    int size_{};
    uint32_t *mem_{};
};

inline int KBitmap::width() { return width_; }

inline int KBitmap::height() { return height_; }

inline int KBitmap::stride() { return stride_; }

inline int KBitmap::size() { return size_; }

inline uint32_t* KBitmap::data() { return mem_; }

inline void KBitmap::put_pixel(int x, int y, uint32_t color)
{
    mem_[y * width_ + x] = color;
}

inline void KBitmap::clear(uint32_t color)
{
    for (int y = 0; y < height_; y++)
    {
        for (int x = 0; x < width_; x++)
        {
            mem_[y * width_ + x] = color;
        }
    }
}
