#pragma once

#define UNICODE
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // Use the STL min/max

#include <windows.h>
#include <cassert>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class KWindow
{
public:
    KWindow();
    virtual ~KWindow();
    virtual bool initialize(int surface_width, int surface_height);
    virtual void run();
    HWND hwnd_{0};

protected:
    virtual LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK static_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};
