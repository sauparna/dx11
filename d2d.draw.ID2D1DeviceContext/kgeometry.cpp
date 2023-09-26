#include "kgeometry.h"

KGeometry::KGeometry(D2D1_SIZE_U bounds)
    : ellipse_iter_{ellipse_list_.end()},
      bounds_{bounds}
{
}

void KGeometry::resize(D2D1_SIZE_U bounds)
{
    bounds_ = bounds;
}

void KGeometry::update()
{
}

bool KGeometry::selectShape(D2D1_POINT_2F point)
{
	for (auto i = ellipse_list_.rbegin(); i != ellipse_list_.rend(); ++i)
	{
		if (insideEllipse(*i, point))
		{
			ellipse_iter_ = (++i).base();
			return true;
		}
	}

	return false;
}

void KGeometry::resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale)
{
	ellipse.radiusX *= scale;
	ellipse.radiusY *= scale;
}

bool KGeometry::insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point)
{
	float rx2 = ellipse.radiusX * ellipse.radiusX;
	float ry2 = ellipse.radiusY * ellipse.radiusY;

	if (rx2 == 0.f || ry2 == 0.f)
	{
		return false;
	}

	float x = point.x - ellipse.point.x;
	float y = point.y - ellipse.point.y;
	float d = ((x * x) / rx2) + ((y * y) / ry2);

	return d <= 1.f;
}
