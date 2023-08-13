#pragma once

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
