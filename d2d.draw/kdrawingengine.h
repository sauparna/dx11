#pragma once

#include <memory>
#include "kd2dsurface.h"

enum class Mode{Draw, Select};

class KDrawingEngine : public KWindow
{
public:
    KDrawingEngine(uint32_t surface_width, uint32_t surface_height);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void Run();
    
private:
    void onKeyDown(WPARAM wparam, LPARAM lparam);
	void onLButtonDown(D2D1_POINT_2L point);
	void onLButtonUp();
	void onMouseMove(D2D1_POINT_2L point, WPARAM wparam);
	void onMouseWheelMove(int wheel_data);
	void onMouseLeave();
	void trackMouse();
    
    std::unique_ptr<KD2DSurface> surface_{};
	KScene		  scene_;
	D2D1_POINT_2F left_click_;
	D2D1_POINT_2F prev_point_;
	HCURSOR		  h_cursor_;
	Mode		  mode_{Mode::Draw};
};
