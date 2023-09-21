#include "kscene.h"

KScene::KScene(uint32_t width, uint32_t height)
    : ellipse_iter_{ellipse_list_.end()},
      width_{width},
      height_{height}
{

}

void KScene::update()
{
}

bool KScene::selectShape(D2D1_POINT_2F point)
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

void resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale)
{
	ellipse.radiusX *= scale;
	ellipse.radiusY *= scale;
}

bool insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point)
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

void drawEllipse(D2D1_ELLIPSE& ellipse,
                 ID2D1RenderTarget **rt,
                 ID2D1SolidColorBrush **brush)
{
	(*brush)->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
	(*rt)->FillEllipse(ellipse, *brush);
	(*brush)->SetColor(D2D1::ColorF{D2D1::ColorF::Black, 0.5f});
	(*rt)->DrawEllipse(ellipse, *brush, 1.f);
	(*brush)->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue, 0.5f));
}
