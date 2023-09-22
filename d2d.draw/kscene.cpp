#include "kscene.h"

KScene::KScene(D2D1_SIZE_U bounds)
    : ellipse_iter_{ellipse_list_.end()},
      bounds_{bounds},
      text_box_width_{bounds.width - (kSeparator + kSeparator)},
      text_box_height_{bounds.height / 4.f}
{
    text_box_point_ = D2D1::Point2F(kSeparator,
                                    bounds.height - (text_box_height_ + kSeparator));
    text_rect_ = D2D1::RectF(text_box_point_.x,
                             text_box_point_.y,
                             text_box_point_.x + text_box_width_,
                             text_box_point_.y + text_box_height_);
}

void KScene::resize(D2D1_SIZE_U bounds)
{
    bounds_ = bounds;
    text_box_point_ = D2D1::Point2F(kSeparator,
                                    bounds.height - (text_box_height_ + kSeparator));
    text_rect_ = D2D1::RectF(text_box_point_.x,
                             text_box_point_.y,
                             text_box_point_.x + text_box_width_,
                             text_box_point_.y + text_box_height_);
}

void KScene::update()
{
    text_ = L"Current mode: " + mode_text_ + L"\n" + kMsgText;
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

void KScene::resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale)
{
	ellipse.radiusX *= scale;
	ellipse.radiusY *= scale;
}

bool KScene::insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point)
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
