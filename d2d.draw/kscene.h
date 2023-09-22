#pragma once

#include <list>
#include <string>
#include <d2d1_2.h>

// The list iterator points to an ellipse that has been selected with
// a mouse-click on it. The iterator points to ellipseList_.end() to
// indicate absence of any selection. The top-most ellipse on the
// drawing surface is the last element in the list.

class KScene
{
public:

	KScene(D2D1_SIZE_U bounds);
    void resize(D2D1_SIZE_U bounds);
    void update();

	bool selectShape(D2D1_POINT_2F point);
    void resizeEllipse(D2D1_ELLIPSE& ellipse, FLOAT scale);

	std::list<D2D1_ELLIPSE> ellipse_list_;
	std::list<D2D1_ELLIPSE>::iterator ellipse_iter_;
	D2D1_RECT_F bounding_box_{};
	bool draw_bounding_box_{false};

    std::wstring text_{};
    D2D1_RECT_F text_rect_{0.f, 0.f, 1.f, 1.f};
    
    std::wstring mode_text_{L"DRAW"};

private:

    bool insideEllipse(D2D1_ELLIPSE& ellipse, D2D1_POINT_2F point);
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Text constants.
    ///////////////////////////////////////////////////////////////////////////////////////////
    std::wstring kMsgText{L"Press M to toggle DRAW/EDIT mode.\n"
                          L"Left-click + drag mouse to draw ellipse in DRAW mode.\n"
                          L"Left-click + drag mouse to move ellipse in EDIT mode.\n"
                          L"Roll mouse-wheel to resize an ellipse in EDIT mode.\n"};
    ///////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////
    // Text layout variables.
    ///////////////////////////////////////////////////////////////////////////////////////////
    const float kSeparator{10.f};
    float text_box_width_{1.f};
    float text_box_height_{1.f};
    D2D1_POINT_2F text_box_point_{0.f, 0.f};
    ///////////////////////////////////////////////////////////////////////////////////////////

    D2D1_SIZE_U bounds_{1, 1};
};
