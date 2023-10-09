#pragma once


class MainToolbarButton : public TextButton {
    
public:

    using TextButton::TextButton;
    
    void paint(Graphics& g) override
    {
        bool active = isOver() || isDown() || getToggleState();

        auto cornerSize = Corners::defaultCornerRadius;
        auto backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto bounds = getLocalBounds().reduced(3, 4).toFloat();

        g.setColour(backgroundColour);
        PlugDataLook::fillSmoothedRectangle(g, bounds, cornerSize);
        
        auto textColour = findColour(PlugDataColour::toolbarTextColourId).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);
        
        AttributedString attributedIcon;
        attributedIcon.append(getButtonText(), Fonts::getIconFont().withHeight(getHeight() / 2.7), textColour);
        attributedIcon.setJustification(Justification::centred);
        
#if JUCE_MAC
        bounds = bounds.withTrimmedBottom(2);
#endif
        attributedIcon.draw(g, bounds);
    }
};

class ToolbarRadioButton : public TextButton {

public:
    
    using TextButton::TextButton;
    
    void paint(Graphics& g) override
    {
        bool mouseOver = isOver();
        bool active = mouseOver || isDown() || getToggleState();

        auto cornerSize = Corners::defaultCornerRadius;
        auto flatOnLeft = isConnectedOnLeft();
        auto flatOnRight = isConnectedOnRight();
        auto flatOnTop = isConnectedOnTop();
        auto flatOnBottom = isConnectedOnBottom();

        auto backgroundColour = findColour(active ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarBackgroundColourId).contrasting((mouseOver && !getToggleState()) ? 0.0f : 0.025f);

        
        auto bounds = getLocalBounds().toFloat();
        bounds = bounds.reduced(0.0f, bounds.proportionOfHeight(0.17f));

        g.setColour(backgroundColour);
        PlugDataLook::fillSmoothedRectangle(g, bounds, Corners::defaultCornerRadius,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom));
        
        auto textColour = findColour(PlugDataColour::toolbarTextColourId).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);
        
        AttributedString attributedIcon;
        attributedIcon.append(getButtonText(), Fonts::getIconFont().withHeight(getHeight() / 2.8), textColour);
        attributedIcon.setJustification(Justification::centred);
        attributedIcon.draw(g, getLocalBounds().toFloat());
    }
};

class SmallIconButton : public TextButton {
    using TextButton::TextButton;
    
    void mouseEnter(const MouseEvent& e) override
    {
        repaint();
    }
    
    void mouseExit(const MouseEvent& e) override
    {
        repaint();
    }
    
    void paint(Graphics& g) override
    {
        auto font = Fonts::getIconFont().withHeight(13.5);
        g.setFont(font);

        if (!isEnabled()) {
            g.setColour(Colours::grey);
        } else if (getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
        } else if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f));
        } else {
            g.setColour(findColour(PlugDataColour::toolbarTextColourId));
        }

        int const yIndent = jmin(4, proportionOfHeight(0.3f));
        int const cornerSize = jmin(getHeight(), getWidth()) / 2;

        int const fontHeight = roundToInt(font.getHeight() * 0.6f);
        int const leftIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnLeft() ? 4 : 2));
        int const rightIndent = jmin(fontHeight, 2 + cornerSize / (isConnectedOnRight() ? 4 : 2));
        int const textWidth = getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::centred, 2);
    }
    
};

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

class ReorderButton : public SmallIconButton {
public:
    ReorderButton()
        : SmallIconButton()
    {
        setButtonText(Icons::Reorder);
        setSize(25, 25);
    }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::DraggingHandCursor;
    }
};
