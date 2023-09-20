#include "kwindow.h"
#include <windowsx.h>
#include "kdrawingengine.h"
#include "resource.h"

using namespace std;

KDrawingEngine::KDrawingEngine(uint32_t surface_width, uint32_t surface_height)
    : KWindow{},
      scene_{surface_width, surface_height}
{
    Initialize(surface_width, surface_height);
    surface_ = std::make_unique<KD2DSurface>(hwnd_, surface_width, surface_height);
}

LRESULT KDrawingEngine::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT lr = 0;
    switch (msg)
    {
	case WM_CREATE:
		h_accel_tab_ = LoadAccelerators(((LPCREATESTRUCT)lparam)->hInstance, MAKEINTRESOURCE(IDR_ACCEL1));
		h_cursor_ = LoadCursor(NULL, IDC_CROSS);
		break;
    case WM_KEYDOWN:
        onKeyDown(wparam, lparam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (surface_)
            surface_->window_resized_ = true;
        break;
	case WM_LBUTTONDOWN:
		onLButtonDown(D2D1_POINT_2L{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
		break;
	case WM_LBUTTONUP:
		onLButtonUp();
		break;
	case WM_MOUSEMOVE:
		trackMouse();
		onMouseMove(D2D1_POINT_2L{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}, wparam);
		break;
	case WM_MOUSELEAVE:
		onMouseLeave();
		break;
	case WM_MOUSEWHEEL:
		onMouseWheelMove(GET_WHEEL_DELTA_WPARAM(wparam));
		break;
	case WM_COMMAND:
		onCommand(LOWORD(wparam));
		break;
	case WM_SETCURSOR:
		if (LOWORD(lparam) == HTCLIENT)
		{
			switch (mode_)
			{
			case Mode::DRAW:
				h_cursor_ = LoadCursor(NULL, IDC_CROSS);
				break;
			case Mode::SELECT:
				h_cursor_ = LoadCursor(NULL, IDC_HAND);
				break;
			}
			SetCursor(h_cursor_);
			return TRUE;
		}
		break;
    default:
        lr = DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return lr;
}

void KDrawingEngine::Run()
{
    static bool running = true;
    while (running)
    {
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (surface_ == nullptr) continue;
        if (surface_->window_resized_)
        {
            surface_->resize();
            surface_->window_resized_ = false;
        }

        if (surface_ == nullptr) continue;
        if (surface_->device_lost_)
        {
            surface_->discard_device_dependent_resources();
            surface_->create_device_dependent_resources();
            surface_->create_render_target_resources();
            surface_->device_lost_ = false;
        }

        if (surface_ == nullptr) continue;
        surface_->update(scene_);
        surface_->render(scene_);
    }
}

void KDrawingEngine::onKeyDown(WPARAM wparam, LPARAM lparam)
{
    if (wparam == VK_ESCAPE)
        DestroyWindow(hwnd_);
}

void KDrawingEngine::onLButtonDown(D2D1_POINT_2L point)
{
    D2D1_POINT_2F fpoint = left_click_ = toDIP(point);

    if (mode_ == Mode::DRAW)
    {
		if (DragDetect(hwnd_, point))
		{
			SetCapture(hwnd_);
			scene_.ellipse_iter_ = scene_.ellipse_list_.insert(scene_.ellipse_list_.end(),
                                                               D2D1_ELLIPSE{fpoint, 0.f, 0.f});
			scene_.bounding_box_ = D2D1_RECT_F{fpoint.x, fpoint.y, fpoint.x, fpoint.y};
			scene_.draw_bounding_box_ = true;
		}
    }
    else if (mode_ == Mode::SELECT)
    {
		scene_.ellipse_iter_ = scene_.ellipse_list_.end();
		if (scene_.selectShape(fpoint))
		{
			// Move selection to end of list, meaning, place it on top
			// of other ellipses in the scene, and consecuently it
			// gets drawn last.
			scene_.ellipse_list_.splice(scene_.ellipse_list_.end(), scene_.ellipse_list_, scene_.ellipse_iter_);
			prev_point_ = left_click_;
		}
    }
    // REWRITE: Necessary?
    InvalidateRect(hwnd_, NULL, FALSE);
}

void KDrawingEngine::onLButtonUp()
{
	if (mode_ == Mode::DRAW && (scene_.ellipse_iter_ != scene_.ellipse_list_.end()))
	{
		scene_.ellipse_iter_ = scene_.ellipse_list_.end();
	}

	scene_.draw_bounding_box_ = false;

	ReleaseCapture();
	InvalidateRect(hwnd_, NULL, FALSE);
}

void KDrawingEngine::onMouseMove(D2D1_POINT_2L point, WPARAM wparam)
{
	D2D1_POINT_2F fpoint = toDIP(point);

	if (((DWORD)wparam & MK_LBUTTON) && (scene_.ellipse_iter_ != scene_.ellipse_list_.end()))
	{
		if (mode_ == Mode::DRAW)
		{
			// Draw an ellipse centered in the bounding box described
			// by the clicked point and current mouse point.
			D2D1_POINT_2F sz{ (fpoint.x - left_click_.x) * .5f , (fpoint.y - left_click_.y) * .5f };
			*(scene_.ellipse_iter_) = D2D1_ELLIPSE{D2D1_POINT_2F{left_click_.x + sz.x, left_click_.y + sz.y}, sz.x, sz.y};

			scene_.bounding_box_ = D2D1_RECT_F{ left_click_.x, left_click_.y, fpoint.x, fpoint.y };
			scene_.draw_bounding_box_ = true;
		}
		else if (mode_ == Mode::SELECT)
		{
			// Move ellipse by the delta the mouse pointer moved from
			// its previous position and save the new position of the
			// pointer.
			float dx = fpoint.x - prev_point_.x;
			float dy = fpoint.y - prev_point_.y;
			scene_.ellipse_iter_->point.x += dx;
			scene_.ellipse_iter_->point.y += dy;

			prev_point_ = fpoint;
		}

		InvalidateRect(hwnd_, NULL, FALSE);
	}    
}
void KDrawingEngine::onMouseWheelMove(int wheel_data)
{
	if (scene_.ellipse_iter_ != scene_.ellipse_list_.end())
	{
		// TODO: Relocate magic number; 120 is recommended in Windows documentation.
		// See: https://docs.microsoft.com/en-us/windows/win32/learnwin32/other-mouse-operations#mouse-wheel
		int steps = wheel_data / 120;
		float scale = 1.f + (float)steps * 0.01f;
		resizeEllipse(*(scene_.ellipse_iter_), scale);
	}

	InvalidateRect(hwnd_, NULL, FALSE);
}

void KDrawingEngine::onMouseLeave()
{
	scene_.ellipse_iter_ = scene_.ellipse_list_.end();
	scene_.draw_bounding_box_ = false;
	InvalidateRect(hwnd_, NULL, FALSE);    
}

void KDrawingEngine::onCommand(WORD command)
{
	switch (command)
	{
	case ID_DRAW_MODE:
		setMode(Mode::DRAW);
		break;
	case ID_SELECT_MODE:
		setMode(Mode::SELECT);
		break;
	case ID_TOGGLE_MODE:
		if (mode_ == Mode::DRAW)
		{
			setMode(Mode::SELECT);
		}
		else
		{
			setMode(Mode::DRAW);
		}
		break;
	}

	// Force a cursor redraw
	{
		POINT p;
		GetCursorPos(&p);
		SetCursorPos(p.x, p.y);
	}
	
	// Redraw everything else.
	InvalidateRect(hwnd_, NULL, FALSE);
}

void KDrawingEngine::setMode(Mode mode)
{
	mode_ = mode;
}

void KDrawingEngine::trackMouse()
{
	TRACKMOUSEEVENT tme{};
	tme.cbSize = sizeof(tme);
	tme.hwndTrack = hwnd_;
	tme.dwFlags = TME_HOVER | TME_LEAVE;
	tme.dwHoverTime = HOVER_DEFAULT;
	TrackMouseEvent(&tme);
}

void KDrawingEngine::setDPIScale()
{
    // REWRITE: This API call is deprecated.
    UINT dpi = GetDpiForWindow(hwnd_);
    dpi_scale_ = static_cast<float>(dpi) / 96.f;
}

D2D1_POINT_2F KDrawingEngine::toDIP(D2D1_POINT_2L point)
{
    return D2D1_POINT_2F{static_cast<float>(point.x) / dpi_scale_,
                         static_cast<float>(point.y) / dpi_scale_};
}
