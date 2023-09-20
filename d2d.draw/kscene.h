#pragma once

#include <list>
#include <d2d1_2.h>

// The list iterator points to an ellipse that has been selected with
// a mouse-click on it. The iterator points to m_ellipseList.end() to
// indicate absence of any selection. The top-most ellipse is the last
// element in the list.

class KScene
{
public:

	KScene(uint32_t width, uint32_t height);
    void update();
	bool selectShape(D2D1_POINT_2F point);

    uint32_t width_{1};
    uint32_t height_{1};
	std::list<D2D1_ELLIPSE> ellipse_list_;
	std::list<D2D1_ELLIPSE>::iterator ellipse_iter_;
	D2D1_RECT_F bounding_box_{};
	bool draw_bounding_box_{false};
};

void resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale);
bool insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point);
void drawEllipse(D2D1_ELLIPSE& ellipse, ID2D1RenderTarget **rt, ID2D1SolidColorBrush **brush);
