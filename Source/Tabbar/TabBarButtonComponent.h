/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

class CloseTabButton;
class TabComponent;
class TabBarButtonComponent : public TabBarButton
    , public ChangeListener {
public:
    TabBarButtonComponent(TabComponent* tabComponent, String const& name, TabbedButtonBar& bar);

    ~TabBarButtonComponent();

    TabComponent* getTabComponent();

    void resized() override;

    void updateCloseButtonState();

    void mouseDrag(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    void lookAndFeelChanged() override;

    void setTabText(String const& text);

    void tabTextChanged(String const& newCurrentTabName);

    void setFocusForTabSplit();

    void drawTabButton(Graphics& g, Rectangle<int> customBounds = Rectangle<int>());
    void drawTabButtonText(Graphics& g, Rectangle<int> customBounds = Rectangle<int>());
    ScaledImage generateTabBarButtonImage();

    void closeTab();

private:
    TabComponent* tabComponent;
    ComponentAnimator* ghostTabAnimator;
    std::unique_ptr<CloseTabButton> closeTabButton;
    ScaledImage tabImage;
    bool isDragging = false;
    bool closeButtonUpdatePending = false;
};
