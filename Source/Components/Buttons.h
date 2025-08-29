/*
 // Copyright (c) 2023-2025 Alexander Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Constants.h"

class MainToolbarButton final : public TextButton {

public:
    using TextButton::TextButton;

    bool isUndo:1 = false;
    bool isRedo:1 = false;

    String getTooltip() override;

    void paint(Graphics& g) override;

    // On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
    ~MainToolbarButton();
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;
#endif
};

class ToolbarRadioButton final : public TextButton {

public:
    using TextButton::TextButton;

    void paint(Graphics& g) override;

    // On macOS, we need to make sure that dragging any of these buttons doesn't drag the whole titlebar
#if JUCE_MAC
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(MouseEvent const& e) override;
#endif
};

class SmallIconButton : public TextButton {
    using TextButton::TextButton;

    bool hitTest(int x, int y) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void paint(Graphics& g) override;
};

class WidePanelButton final : public TextButton {
    String icon;
    int iconSize;

public:
    explicit WidePanelButton(String const& icon, int const iconSize = 13);

    void mouseEnter(MouseEvent const& e) override;

    void mouseExit(MouseEvent const& e) override;

    void paint(Graphics& g) override;
};

// Toolbar button for settings panel, with both icon and text
class SettingsToolbarButton final : public TextButton {

    String icon;
    String text;

public:
    SettingsToolbarButton(String iconToUse, String textToShow);

    void paint(Graphics& g) override;
};

class ReorderButton final : public SmallIconButton {
public:
    ReorderButton();

    MouseCursor getMouseCursor() override;
};
