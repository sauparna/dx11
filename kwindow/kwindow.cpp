#include <cassert>
#include "kwindow.h"

KWindow::KWindow() { }

KWindow::~KWindow() {}

bool KWindow::initialize(int surface_width = 400, int surface_height = 300)
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = static_wndproc;
    wc.hInstance = HINST_THISCOMPONENT;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"KWindowClass";
    wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBoxA(0, "RegisterClassEx failed", "ERROR", MB_OK);
        return false;
    }

    RECT rect{0, 0, surface_width, surface_height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
    LONG window_width = rect.right - rect.left;
    LONG window_height = rect.bottom - rect.top;

    MDICREATESTRUCT mdic{};
    mdic.lParam = (LPARAM)this;

    hwnd_ = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
                           wc.lpszClassName,
                           L"KWindow",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           window_width,
                           window_height,
                           0,
                           0,
                           HINST_THISCOMPONENT,
                           &mdic);
    assert(hwnd_ != 0);

    if (hwnd_ == 0)
    {
        MessageBoxA(0, "CreateWindowEx failed", "ERROR", MB_OK);
        return false;
    }

    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);

    return true;
}

LRESULT KWindow::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lr = 0;
    switch (msg)
    {
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
            DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    default:
        lr = DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return lr;
}

LRESULT CALLBACK KWindow::static_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    KWindow *ptr_window;
    if (msg == WM_NCCREATE)
    {
        assert(!IsBadReadPtr((void*)lparam, sizeof(CREATESTRUCT)));
        MDICREATESTRUCT *ptr_mdic = (MDICREATESTRUCT*)((LPCREATESTRUCT)lparam)->lpCreateParams;
        ptr_window = (KWindow*)(ptr_mdic->lParam);
        assert(!IsBadReadPtr(ptr_window, sizeof(KWindow)));
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptr_window);
    }
    else
    {
        ptr_window = (KWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (ptr_window) return ptr_window->wndproc(hwnd, msg, wparam, lparam);
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void KWindow::run()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
