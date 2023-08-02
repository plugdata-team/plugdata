#pragma once

class ReorderButton : public TextButton {
public:
    ReorderButton()
        : TextButton() {
            setButtonText(Icons::Reorder);
            setSize(25, 25);
            getProperties().set("Style", "SmallIcon");
        }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::DraggingHandCursor;
    }
};