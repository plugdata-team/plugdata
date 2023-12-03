#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Tabbar.h"
#include "Utility/SplitModeEnum.h"
#include "Utility/RateReducer.h"

class ResizableTabbedComponent;
class SplitViewResizer : public Component {
public:
    static inline constexpr int thickness = 6;
    enum ResizerEdge { Left,
        Right,
        Top,
        Bottom };
    float resizerPosition;
    Split::SplitMode splitMode = Split::SplitMode::None;

    SplitViewResizer(ResizableTabbedComponent* left, ResizableTabbedComponent* right, Split::SplitMode mode = Split::SplitMode::None, int flipped = -1);

    void setSplitMode(Split::SplitMode mode);

    bool setResizerPosition(float newPosition, bool checkLeft = true);

    MouseCursor getMouseCursor() override;

    bool hitTest(int x, int y) override;

    void resized() override;

    void paint(Graphics& g) override;

    Rectangle<int> resizeArea;

    ResizableTabbedComponent* splits[2] = { nullptr };

private:
    void mouseDown(MouseEvent const& e) override;

    void mouseDrag(MouseEvent const& e) override;

    float minimumWidth = 100.0f;
    float minimumHeight = 100.0f;

    bool hitEdge = false;

    int leftBounds = 0;
    float dragPositionX = 0;
    int dragPositionY = 0;
    bool draggingSplitview = false;

    RateReducer rateReducer = RateReducer(60);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplitViewResizer)
};
