#pragma once

class GlobalMouseListener : public MouseListener {
    Component* target;

public:
    explicit GlobalMouseListener(Component* targetComponent = nullptr)
        : target(targetComponent)
    {
        Desktop::getInstance().addGlobalMouseListener(this);
    }

    ~GlobalMouseListener() override
    {
        Desktop::getInstance().removeGlobalMouseListener(this);
    }

    std::function<void(MouseEvent const&)> globalMouseDown = [](MouseEvent const&) { };
    std::function<void(MouseEvent const&)> globalMouseUp = [](MouseEvent const&) { };
    std::function<void(MouseEvent const&)> globalMouseDrag = [](MouseEvent const&) { };
    std::function<void(MouseEvent const&)> globalMouseMove = [](MouseEvent const&) { };
    std::function<void(MouseEvent const&, const MouseWheelDetails&)> globalMouseWheel = [](const MouseEvent& event, const MouseWheelDetails& wheel) { };

    void mouseDown(MouseEvent const& e) override
    {
        globalMouseDown(target ? e.getEventRelativeTo(target) : e.withNewPosition(e.getScreenPosition()));
    }

    void mouseUp(MouseEvent const& e) override
    {
        globalMouseUp(target ? e.getEventRelativeTo(target) : e.withNewPosition(e.getScreenPosition()));
    }

    void mouseDrag(MouseEvent const& e) override
    {
        globalMouseDrag(target ? e.getEventRelativeTo(target) : e.withNewPosition(e.getScreenPosition()));
    }

    void mouseMove(MouseEvent const& e) override
    {
        globalMouseMove(target ? e.getEventRelativeTo(target) : e.withNewPosition(e.getScreenPosition()));
    }
    
     void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override
    {
        globalMouseWheel(target ? e.getEventRelativeTo(target) : e.withNewPosition(e.getScreenPosition()), wheel);
    }

};
