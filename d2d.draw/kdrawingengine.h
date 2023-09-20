#pragma once

#include <memory>
#include "kd2dsurface.h"

enum class Mode {DRAW, SELECT};

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
	void setMode(Mode mode);
	void trackMouse();
    void onCommand(WORD command);
    void setDPIScale();
    D2D1_POINT_2F toDIP(D2D1_POINT_2L point);
    
    std::unique_ptr<KD2DSurface> surface_{};
	KScene		  scene_;
	D2D1_POINT_2F left_click_;
	D2D1_POINT_2F prev_point_;
	HACCEL		  h_accel_tab_;
	HCURSOR		  h_cursor_;
	Mode		  mode_{Mode::DRAW};
    float         dpi_scale_{};
};
