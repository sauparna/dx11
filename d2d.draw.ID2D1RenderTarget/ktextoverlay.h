#pragma once

#include <string>
#include <d2d1_2.h>

class KTextOverlay
{
public:

    KTextOverlay(D2D1_SIZE_U bounds);
    ~KTextOverlay();
    void Update();
    void Resize(D2D1_SIZE_U newBounds);

    std::wstring text{};
    D2D1_RECT_F rect{0.f, 0.f, 1.f, 1.f};
    std::wstring modeText{L"DRAW"};

private:
    
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
    float boxWidth{1.f};
    float boxHeight{1.f};
    D2D1_POINT_2F boxTopLeftPoint{0.f, 0.f};
    ///////////////////////////////////////////////////////////////////////////////////////////

    D2D1_SIZE_U bounds{1, 1};
};
