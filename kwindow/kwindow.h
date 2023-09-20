#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class KWindow
{
public:
    KWindow();
    virtual ~KWindow();
    virtual bool Initialize(int surface_width, int surface_height);
    virtual void Run();
    HWND hwnd_{0};

protected:
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};
