#pragma once

// Toolbar button for settings panel, with both icon and text
class SettingsToolbarButton : public TextButton {

    String icon;
    String text;

public:
    SettingsToolbarButton(String iconToUse, String textToShow)
        : icon(std::move(iconToUse))
        , text(std::move(textToShow))
    {
        setClickingTogglesState(true);
        setConnectedEdges(12);
        getProperties().set("Style", "LargeIcon");
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(2.0f, 4.0f);

        if (isMouseOver() || getToggleState()) {
            auto background = findColour(PlugDataColour::toolbarHoverColourId);
            if (getToggleState())
                background = background.darker(0.025f);

            g.setColour(background);
            PlugDataLook::fillSmoothedRectangle(g, b.toFloat(), Corners::defaultCornerRadius);
        }

        auto textColour = findColour(PlugDataColour::toolbarTextColourId);
        auto boldFont = Fonts::getBoldFont().withHeight(13.5f);
        auto iconFont = Fonts::getIconFont().withHeight(13.5f);

        auto textWidth = boldFont.getStringWidth(text);
        auto iconWidth = iconFont.getStringWidth(icon);

        AttributedString attrStr;
        attrStr.setJustification(Justification::centred);
        attrStr.append(icon, iconFont, textColour);
        attrStr.append("  " + text, boldFont, textColour);
        attrStr.draw(g, b.toFloat());
    }
};

class WelcomePanelButton : public Component {

public:
    String iconText;
    String topText;
    String bottomText;

    std::function<void(void)> onClick = []() {};

    WelcomePanelButton(String icon, String mainText, String subText)
        : iconText(std::move(icon))
        , topText(std::move(mainText))
        , bottomText(std::move(subText))
    {
        setInterceptsMouseClicks(true, false);
        setAlwaysOnTop(true);
    }

    void paint(Graphics& g) override
    {
        auto colour = findColour(PlugDataColour::panelTextColourId);
        if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            PlugDataLook::fillSmoothedRectangle(g, Rectangle<float>(1, 1, getWidth() - 2, getHeight() - 2), Corners::largeCornerRadius);
            colour = findColour(PlugDataColour::panelActiveTextColourId);
        }

        Fonts::drawIcon(g, iconText, 20, 5, 40, colour, 24, false);
        Fonts::drawText(g, topText, 60, 7, getWidth() - 60, 20, colour, 16);
        Fonts::drawStyledText(g, bottomText, 60, 25, getWidth() - 60, 16, colour, Thin, 14);
    }

    void mouseUp(MouseEvent const& e) override
    {
        onClick();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }
};

class ReorderButton : public TextButton {
public:
    ReorderButton()
        : TextButton()
    {
        setButtonText(Icons::Reorder);
        setSize(25, 25);
        getProperties().set("Style", "SmallIcon");
    }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::DraggingHandCursor;
    }
};
