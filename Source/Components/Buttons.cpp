/*
 // Copyright (c) 2023-2025 Alexander Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "PluginEditor.h"
#include "LookAndFeel.h"
#include "Buttons.h"

String MainToolbarButton::getTooltip()
{
    auto setTooltip = TextButton::getTooltip();
    if (auto* editor = dynamic_cast<PluginEditor*>(getParentComponent())) {
        if (auto const* cnv = editor->getCurrentCanvas()) {
            if (isUndo) {
                setTooltip = "Undo";
                if (cnv->patch.canUndo() && cnv->patch.lastUndoSequence != "")
                    setTooltip += ": " + cnv->patch.lastUndoSequence.toString();
            } else if (isRedo) {
                setTooltip = "Redo";
                if (cnv->patch.canRedo() && cnv->patch.lastRedoSequence != "")
                    setTooltip += ": " + cnv->patch.lastRedoSequence.toString();
            }
        }
    }
    return setTooltip;
}

void MainToolbarButton::paint(Graphics& g)
{
    bool const active = isOver() || isDown() || getToggleState();

    auto constexpr cornerSize = Corners::defaultCornerRadius;
    auto const backgroundColour = active ? PlugDataColours::toolbarHoverColour : Colours::transparentBlack;
    auto bounds = getLocalBounds().reduced(3, 4).toFloat();

    g.setColour(backgroundColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    auto const textColour = PlugDataColours::toolbarTextColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

#if JUCE_MAC
    bounds = bounds.withTrimmedBottom(2);
#endif

    g.setFont(Fonts::getIconFont().withHeight(getHeight() / 2.7));
    g.setColour(textColour);
    g.drawText(getButtonText(), bounds, Justification::centred);
}

// On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
MainToolbarButton::~MainToolbarButton()
{
    if (auto const* topLevel = getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, true);
        }
    }
}

void MainToolbarButton::mouseEnter(MouseEvent const& e)
{
    if (auto const* topLevel = getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, false);
        }
    }
    TextButton::mouseEnter(e);
}

void MainToolbarButton::mouseExit(MouseEvent const& e)
{
    if (auto const* topLevel = getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, true);
        }
    }
    TextButton::mouseExit(e);
}
#endif

void ToolbarRadioButton::paint(Graphics& g)
{
    bool const mouseOver = isOver();
    bool const active = mouseOver || isDown() || getToggleState();

    auto const flatOnLeft = isConnectedOnLeft();
    auto const flatOnRight = isConnectedOnRight();
    auto const flatOnTop = isConnectedOnTop();
    auto const flatOnBottom = isConnectedOnBottom();

    auto const backgroundColour = (active ? PlugDataColours::toolbarHoverColour : PlugDataColours::toolbarBackgroundColour).contrasting(mouseOver && !getToggleState() ? 0.0f : 0.035f);

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

    auto const textColour = PlugDataColours::toolbarTextColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

    g.setFont(Fonts::getIconFont().withHeight(getHeight() / 2.8));
    g.setColour(textColour);
    g.drawText(getButtonText(), getLocalBounds(), Justification::centred);
}

// On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
void ToolbarRadioButton::mouseEnter(const MouseEvent& e)
{
    if (auto const* topLevel = getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, false);
        }
    }
    TextButton::mouseEnter(e);
}

void ToolbarRadioButton::mouseExit(MouseEvent const& e)
{
    if (auto const* topLevel = getTopLevelComponent()) {
        if (auto* peer = topLevel->getPeer()) {
            OSUtils::setWindowMovable(peer, true);
        }
    }
    TextButton::mouseExit(e);
}
#endif

bool SmallIconButton::hitTest(int const x, int const y)
{
    if (getLocalBounds().reduced(2).contains(x, y))
        return true;

    return false;
}

void SmallIconButton::mouseEnter(MouseEvent const& e)
{
    repaint();
}

void SmallIconButton::mouseExit(MouseEvent const& e)
{
    repaint();
}

void SmallIconButton::paint(Graphics& g)
{
    auto colour = PlugDataColours::toolbarTextColour;

    if (!isEnabled()) {
        colour = Colours::grey;
    } else if (getToggleState()) {
        colour = PlugDataColours::toolbarActiveColour;
    } else if (isMouseOver()) {
        colour = PlugDataColours::toolbarTextColour.brighter(0.8f);
    }

    Fonts::drawIcon(g, getButtonText(), getLocalBounds(), colour, 12);
}

WidePanelButton::WidePanelButton(String const& icon, int const iconSize)
    : icon(icon)
    , iconSize(iconSize)
{
}

void WidePanelButton::mouseEnter(MouseEvent const& e)
{
    repaint();
}

void WidePanelButton::mouseExit(MouseEvent const& e)
{
    repaint();
}

void WidePanelButton::paint(Graphics& g)
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

    g.setColour(isMouseOver() ? PlugDataColours::panelActiveBackgroundColour : PlugDataColours::panelForegroundColour);
    g.fillPath(outline);

    g.setColour(PlugDataColours::outlineColour);
    g.strokePath(outline, PathStrokeType(1));

    Fonts::drawText(g, getButtonText(), getLocalBounds().reduced(12, 2), PlugDataColours::panelTextColour, 15);
    Fonts::drawIcon(g, icon, getLocalBounds().reduced(12, 2).removeFromRight(24), PlugDataColours::panelTextColour, iconSize);
}

SettingsToolbarButton::SettingsToolbarButton(String iconToUse, String textToShow)
    : icon(std::move(iconToUse))
    , text(std::move(textToShow))
{
    setClickingTogglesState(true);
    setConnectedEdges(12);
}

void SettingsToolbarButton::paint(Graphics& g)
{
    auto const b = getLocalBounds().reduced(2.0f, 4.0f);

    if (isMouseOver() || getToggleState()) {
        auto background = PlugDataColours::toolbarHoverColour;
        if (getToggleState())
            background = background.darker(0.025f);

        g.setColour(background);
        g.fillRoundedRectangle(b.toFloat(), Corners::defaultCornerRadius);
    }

    auto const textColour = PlugDataColours::toolbarTextColour;
    auto const boldFont = Fonts::getBoldFont().withHeight(13.5f);
    auto const iconFont = Fonts::getIconFont().withHeight(13.5f);

    AttributedString attrStr;
    attrStr.setJustification(Justification::centred);
    attrStr.append(icon, iconFont, textColour);
    attrStr.append("  " + text, boldFont, textColour);
    attrStr.draw(g, b.toFloat());
}

ReorderButton::ReorderButton()
    : SmallIconButton(Icons::Reorder)
{
    setSize(25, 25);
}

MouseCursor ReorderButton::getMouseCursor()
{
    return MouseCursor::DraggingHandCursor;
}
