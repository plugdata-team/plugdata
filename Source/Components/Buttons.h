#pragma once

#include "Constants.h"

class MainToolbarButton final : public TextButton {

public:
    using TextButton::TextButton;

    bool isUndo = false;
    bool isRedo = false;

    String getTooltip() override;

    void paint(Graphics& g) override
    {
        bool const active = isOver() || isDown() || getToggleState();

        auto constexpr cornerSize = Corners::defaultCornerRadius;
        auto const backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto bounds = getLocalBounds().reduced(3, 4).toFloat();

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        auto const textColour = findColour(PlugDataColour::toolbarTextColourId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

#if JUCE_MAC
        bounds = bounds.withTrimmedBottom(2);
#endif

        g.setFont(Fonts::getIconFont().withHeight(getHeight() / 2.7));
        g.setColour(textColour);
        g.drawText(getButtonText(), bounds, Justification::centred);
    }

    // On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
    ~MainToolbarButton()
    {
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto const* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer->getNativeHandle(), true);
            }
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto const* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer->getNativeHandle(), false);
            }
        }
        TextButton::mouseEnter(e);
    }

    void mouseExit(MouseEvent const& e) override
    {
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto const* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer->getNativeHandle(), true);
            }
        }
        TextButton::mouseExit(e);
    }
#endif
};

class ToolbarRadioButton final : public TextButton {

public:
    using TextButton::TextButton;

    void paint(Graphics& g) override
    {
        bool const mouseOver = isOver();
        bool const active = mouseOver || isDown() || getToggleState();

        auto const flatOnLeft = isConnectedOnLeft();
        auto const flatOnRight = isConnectedOnRight();
        auto const flatOnTop = isConnectedOnTop();
        auto const flatOnBottom = isConnectedOnBottom();

        auto const backgroundColour = findColour(active ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarBackgroundColourId).contrasting(mouseOver && !getToggleState() ? 0.0f : 0.035f);

        auto bounds = getLocalBounds().toFloat();
        bounds = bounds.reduced(0.0f, bounds.proportionOfHeight(0.17f));

        g.setColour(backgroundColour);
        Path p;
        p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::defaultCornerRadius, Corners::defaultCornerRadius,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom));
        g.fillPath(p);

        auto const textColour = findColour(PlugDataColour::toolbarTextColourId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

        g.setFont(Fonts::getIconFont().withHeight(getHeight() / 2.8));
        g.setColour(textColour);
        g.drawText(getButtonText(), getLocalBounds(), Justification::centred);
    }

    // On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
    void mouseEnter(const MouseEvent& e) override
    {
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto const* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer->getNativeHandle(), false);
            }
        }
        TextButton::mouseEnter(e);
    }

    void mouseExit(MouseEvent const& e) override
    {
        if (auto const* topLevel = getTopLevelComponent()) {
            if (auto const* peer = topLevel->getPeer()) {
                OSUtils::setWindowMovable(peer->getNativeHandle(), true);
            }
        }
        TextButton::mouseExit(e);
    }
#endif
};

class SmallIconButton : public TextButton {
    using TextButton::TextButton;

    bool hitTest(int x, int y) override
    {
        if (getLocalBounds().reduced(2).contains(x, y))
            return true;

        return false;
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto colour = findColour(PlugDataColour::toolbarTextColourId);

        if (!isEnabled()) {
            colour = Colours::grey;
        } else if (getToggleState()) {
            colour = findColour(PlugDataColour::toolbarActiveColourId);
        } else if (isMouseOver()) {
            colour = findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f);
        }

        Fonts::drawIcon(g, getButtonText(), getLocalBounds(), colour, 12);
    }
};

class WidePanelButton final : public TextButton {
    String icon;
    int iconSize;

public:
    explicit WidePanelButton(String const& icon, int const iconSize = 13)
        : icon(icon)
        , iconSize(iconSize) { };

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        bool const flatOnLeft = isConnectedOnLeft();
        bool const flatOnRight = isConnectedOnRight();
        bool const flatOnTop = isConnectedOnTop();
        bool const flatOnBottom = isConnectedOnBottom();

        float const width = getWidth() - 1.0f;
        float const height = getHeight() - 1.0f;

        constexpr float cornerSize = Corners::largeCornerRadius;
        Path outline;
        outline.addRoundedRectangle(0.5f, 0.5f, width, height, cornerSize, cornerSize,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom));

        g.setColour(findColour(isMouseOver() ? PlugDataColour::panelActiveBackgroundColourId : PlugDataColour::panelForegroundColourId));
        g.fillPath(outline);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(outline, PathStrokeType(1));

        Fonts::drawText(g, getButtonText(), getLocalBounds().reduced(12, 2), findColour(PlugDataColour::panelTextColourId), 15);
        Fonts::drawIcon(g, icon, getLocalBounds().reduced(12, 2).removeFromRight(24), findColour(PlugDataColour::panelTextColourId), iconSize);
    }
};

// Toolbar button for settings panel, with both icon and text
class SettingsToolbarButton final : public TextButton {

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
        auto const b = getLocalBounds().reduced(2.0f, 4.0f);

        if (isMouseOver() || getToggleState()) {
            auto background = findColour(PlugDataColour::toolbarHoverColourId);
            if (getToggleState())
                background = background.darker(0.025f);

            g.setColour(background);
            g.fillRoundedRectangle(b.toFloat(), Corners::defaultCornerRadius);
        }

        auto const textColour = findColour(PlugDataColour::toolbarTextColourId);
        auto const boldFont = Fonts::getBoldFont().withHeight(13.5f);
        auto const iconFont = Fonts::getIconFont().withHeight(13.5f);

        AttributedString attrStr;
        attrStr.setJustification(Justification::centred);
        attrStr.append(icon, iconFont, textColour);
        attrStr.append("  " + text, boldFont, textColour);
        attrStr.draw(g, b.toFloat());
    }
};

class ReorderButton final : public SmallIconButton {
public:
    ReorderButton()
        : SmallIconButton(Icons::Reorder)
    {
        setSize(25, 25);
    }

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::DraggingHandCursor;
    }
};
