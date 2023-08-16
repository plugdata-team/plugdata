#pragma once

#include <JuceHeader.h>

class TabComponent;
class TabBarButtonComponent : public TabBarButton, public ChangeListener
{
public:
    TabBarButtonComponent(TabComponent* tabComponent, const String& name, TabbedButtonBar& bar);

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

    void tabTextChanged(const String& newCurrentTabName);

    void setFocusForTabSplit();

    void drawTabButton(Graphics& g, Rectangle<int> customBounds = Rectangle<int>());
    void drawTabButtonText(Graphics& g, Rectangle<int> customBounds = Rectangle<int>());
    ScaledImage generateTabBarButtonImage();

    void paint(Graphics& g) override;
    
    void closeTab();

private:
    TabComponent* tabComponent;
    ComponentAnimator* ghostTabAnimator;
    TextButton closeTabButton;
    const int boundsOffset = 10;
    ScaledImage tabImage;
    bool isDragging = false;
    bool closeButtonUpdatePending = false;
};
