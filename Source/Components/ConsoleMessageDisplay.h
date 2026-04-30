/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#pragma once

#pragma once

class ConsoleMessageDisplay final : public Component {
public:
    ConsoleMessageDisplay(PluginEditor* e)
        : editor(e)
    {
        setInterceptsMouseClicks(false, false);
        setVisible(false);

        setCachedComponentImage(new NVGSurface::InvalidationListener(e->nvgSurface, this));
    }

    void showMessage(SmallString const& message, bool const isWarning)
    {
        if (message.isEmpty()) {
            hide();
            return;
        }

        currentMessage = message.toString();
        currentIsWarning = isWarning;

        if (auto* parent = getParentComponent())
            parent->resized();
        repaint();

        setVisible(true);
        setBounds(getBounds().withWidth(getDesiredWidth()).withRight(getRight()));

        hideTimer.startTimer(2000);
    }

    void hide()
    {
        setVisible(false);
        currentMessage = { };
        if (auto* parent = getParentComponent())
            parent->resized();
    }

    int getDesiredWidth() const
    {
        if (currentMessage.isEmpty())
            return 0;
        auto const attr = makeAttributedString(currentMessage);
        TextLayout layout;
        layout.createLayout(attr, 10000.0f);
        return static_cast<int>(std::ceil(layout.getWidth())) + 28;
    }

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds();
        auto const textColour = currentIsWarning ? Colours::orange : PlugDataColours::toolbarTextColour;

        auto const b = getLocalBounds().reduced(5);
        StackShadow::drawShadowForRect(g, b.reduced(3.0f), 10, Corners::largeCornerRadius, 0.4f, 1);

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);

        AttributedString attr;
        attr.setJustification(Justification::centred);
        attr.append(currentMessage,
            Fonts::getDefaultFont().withHeight(13.0f),
            textColour);
        attr.draw(g, bounds.reduced(5).toFloat().reduced(10, 0));
    }

    AttributedString makeAttributedString(String const& text) const
    {
        AttributedString attr;
        attr.setJustification(Justification::centredLeft);
        auto const textColour = currentIsWarning ? Colours::orange : PlugDataColours::toolbarTextColour;
        attr.append(text, Fonts::getDefaultFont().withHeight(13.0f), textColour);
        return attr;
    }

    PluginEditor* editor;
    String currentMessage;
    bool currentIsWarning = false;

    TimedCallback hideTimer = TimedCallback([this] {
        hideTimer.stopTimer();
        hide();
    });

    NVGImage componentImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleMessageDisplay)
};
