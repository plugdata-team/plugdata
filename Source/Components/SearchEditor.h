#pragma once

class SearchEditor final : public TextEditor {
public:
    SearchEditor()
    {
        clearButton.setAlwaysOnTop(true);
        clearButton.onClick = [this] {
            setText("", sendNotification);
            grabKeyboardFocus();
        };

        addAndMakeVisible(clearButton);
    }

    void resized() override
    {
        TextEditor::resized();
        clearButton.setBounds(getLocalBounds().removeFromRight(30));
    }

    bool keyPressed(KeyPress const& key) override
    {
        // Make sure that the dialog above this can still receive escape event
        if (key.getKeyCode() == KeyPress::escapeKey) {
            if (auto* parentComponent = getParentComponent()) {
                parentComponent->grabKeyboardFocus();
            }

            // Don't claim this keypress
            return false;
        }

        return TextEditor::keyPressed(key);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(backgroundColour));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);

        if (hasKeyboardFocus(false)) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::defaultCornerRadius, 2.0f);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto const textToShowWhenEmpty = getTextToShowWhenEmpty();
        if (textToShowWhenEmpty.isNotEmpty()
            && getTotalNumChars() == 0) {
            g.setColour(findColour(TextEditor::textColourId).withAlpha(0.5f));
            g.setFont(getFont().withHeight(13.f));

            g.drawText(textToShowWhenEmpty, getBorder().subtractedFrom(getLocalBounds()).toFloat().translated(4, 2), Justification::centredLeft, true);
        }
    }

    void setBackgroundColour(PlugDataColour const newBackgroundColour)
    {
        backgroundColour = newBackgroundColour;
    }

private:
    PlugDataColour backgroundColour = PlugDataColour::toolbarHoverColourId;
    SmallIconButton clearButton = SmallIconButton(Icons::ClearText);
};
