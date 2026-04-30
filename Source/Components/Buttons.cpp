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

    g.setColour(PlugDataColours::toolbarOutlineColour);
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

CalloutMenuButton::CalloutMenuButton(String const& iconString, String const& descriptionString, bool const toggleButton, String const& keyboardShortcutString)
    : icon(iconString)
    , description(descriptionString)
    , keyboardShortcut(keyboardShortcutString)
{
    setClickingTogglesState(toggleButton);
}

void CalloutMenuButton::paint(Graphics& g)
{
    g.setColour(isMouseOver() ? PlugDataColours::popupMenuActiveBackgroundColour : PlugDataColours::popupMenuBackgroundColour);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), Corners::defaultCornerRadius);

    auto colour = PlugDataColours::toolbarTextColour;
    Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14.5);

    if (getToggleState()) {
        colour = PlugDataColours::toolbarActiveColour;
    }

    Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);

    auto shortcutBounds = getLocalBounds().removeFromRight(64).withSize(64, 24).translated(-8, 4);
#if JUCE_MAC
    for (int i = keyboardShortcut.length() - 1; i >= 0; i--) {
        auto font = Fonts::getSemiBoldFont().withHeight(10.5f);
        auto text = keyboardShortcut.substring(i, i + 1);
        auto width = std::max(Fonts::getStringWidthInt(text, font) + 4, 16);
        auto b = shortcutBounds.removeFromRight(width).toFloat().reduced(1.0f, 5.0f).translated(1.5f, 0.5f);

        g.setColour(PlugDataColours::popupMenuTextColour.withAlpha(0.9f));
        g.fillRoundedRectangle(b.toFloat(), 3.0f);

        g.setColour(PlugDataColours::popupMenuBackgroundColour);

        g.setFont(Fonts::getSemiBoldFont().withHeight(11));
        g.drawText(text, b, Justification::centred);
    }
#elif JUCE_WINDOWS || JUCE_LINUX || JUCE_BSD
    auto keys = StringArray::fromTokens(keyboardShortcut, "+", "");
    for (int i = keys.size() - 1; i >= 0; i--) {
        auto font = Fonts::getSemiBoldFont().withHeight(10.5f);
        auto width = std::max(Fonts::getStringWidthInt(keys[i].trim(), font) + 8, 15);
        auto b = shortcutBounds.removeFromRight(width).reduced(1, 5);

        g.setColour(PlugDataColours::popupMenuTextColour.withAlpha(0.9f));
        g.fillRoundedRectangle(b.toFloat(), 3.0f);

        g.setColour(PlugDataColours::popupMenuBackgroundColour);

        g.setFont(font);
        g.drawText(keys[i], b, Justification::centred);
    }
#endif
}

int CalloutMenuButton::getIdealWidth() const
{
    return Fonts::getStringWidth(icon + description, 15.f) + Fonts::getStringWidth(keyboardShortcut, 10.5f); // just an approximation
}
