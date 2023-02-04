#include <JuceHeader.h>

class GlobalMouseListener : public MouseListener {
    Component* target;

public:
    GlobalMouseListener(Component* targetComponent)
        : target(targetComponent)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }

    ~GlobalMouseListener()
    {
        Desktop::getInstance().removeGlobalMouseListener(this);
    }

    std::function<void(MouseEvent const& e)> globalMouseDown = [](MouseEvent const&) {};
    std::function<void(MouseEvent const& e)> globalMouseUp = [](MouseEvent const&) {};
    std::function<void(MouseEvent const& e)> globalMouseDrag = [](MouseEvent const&) {};
    std::function<void(MouseEvent const& e)> globalMouseMove = [](MouseEvent const&) {};

    void mouseDown(MouseEvent const& e) override
    {
        globalMouseDown(e.getEventRelativeTo(target));
    }

    void mouseUp(MouseEvent const& e) override
    {
        globalMouseUp(e.getEventRelativeTo(target));
    }

    void mouseDrag(MouseEvent const& e) override
    {
        globalMouseDrag(e.getEventRelativeTo(target));
    }

    void mouseMove(MouseEvent const& e) override
    {
        globalMouseMove(e.getEventRelativeTo(target));
    }
};
