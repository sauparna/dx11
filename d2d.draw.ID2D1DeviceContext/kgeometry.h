#pragma once

#include <list>
#include <d2d1_2.h>

// The list iterator points to an ellipse that has been selected with
// a mouse-click on it. The iterator points to ellipseList_.end() to
// indicate absence of any selection. The top-most ellipse on the
// drawing surface is the last element in the list.

class KGeometry
{
public:

	KGeometry(D2D1_SIZE_U bounds);
    void resize(D2D1_SIZE_U bounds);
    void update();

	bool selectShape(D2D1_POINT_2F point);
    void resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale);

	std::list<D2D1_ELLIPSE> ellipse_list_;
	std::list<D2D1_ELLIPSE>::iterator ellipse_iter_;
	D2D1_RECT_F bounding_box_{};
	bool draw_bounding_box_{false};

private:

    bool insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point);
    
    D2D1_SIZE_U bounds_{1, 1};
};
