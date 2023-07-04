#pragma once

#include <JuceHeader.h>

class TabComponent;
class TabBarButtonComponent : public TabBarButton
{
public:
    TabBarButtonComponent(TabComponent* tabComponent, const String& name, TabbedButtonBar& bar);

    ~TabBarButtonComponent();

    TabComponent* getTabComponent();

    void resized() override;

    void mouseDrag(MouseEvent const& e) override;
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void lookAndFeelChanged() override;

    void setTabText(String const& text);

    void tabTextChanged(const String& newCurrentTabName);

    void drawTabButton(Graphics& g);
    void drawTabButtonText(Graphics& g);
    Image generateTabBarButtonImage();

    void paint(Graphics& g) override;

private:
    TabComponent* tabComponent;
    TextButton closeTabButton;
    const int boundsOffset = 10;
    Image tabImage;
    bool isDirty = true;
};