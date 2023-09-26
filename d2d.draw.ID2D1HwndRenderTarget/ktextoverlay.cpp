#include "ktextoverlay.h"

KTextOverlay::KTextOverlay(D2D1_SIZE_U bounds)
    : bounds{bounds},
      boxWidth{bounds.width - (kSeparator + kSeparator)},
      boxHeight{bounds.height / 4.f}
{
    boxTopLeftPoint = D2D1::Point2F(kSeparator,
                                    bounds.height - (boxHeight + kSeparator));
    rect = D2D1::RectF(boxTopLeftPoint.x,
                       boxTopLeftPoint.y,
                       boxTopLeftPoint.x + boxWidth,
                       boxTopLeftPoint.y + boxHeight);
}

KTextOverlay::~KTextOverlay()
{
}

void KTextOverlay::Update()
{
    text = L"Current mode: " + modeText + L"\n" + kMsgText;
}

void KTextOverlay::Resize(D2D1_SIZE_U newBounds)
{
    bounds = newBounds;
    boxTopLeftPoint = D2D1::Point2F(kSeparator,
                                    bounds.height - (boxHeight + kSeparator));
    rect = D2D1::RectF(boxTopLeftPoint.x,
                       boxTopLeftPoint.y,
                       boxTopLeftPoint.x + boxWidth,
                       boxTopLeftPoint.y + boxHeight);
}
