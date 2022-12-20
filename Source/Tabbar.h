/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

struct WelcomePanel : public Component
{
    
    struct WelcomeButton : public Component
    {
        String iconText;
        String topText;
        String bottomText;
        
        std::function<void(void)> onClick = [](){};
        
        WelcomeButton(String icon, String mainText, String subText) :
        iconText(icon),
        topText(mainText),
        bottomText(subText)
        {
            setInterceptsMouseClicks(true, false);
            setAlwaysOnTop(true);
            
        }
        
        void paint(Graphics& g)
        {
            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
            
            if(isMouseOver()) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                g.fillRoundedRectangle(1, 1, getWidth() - 2, getHeight() - 2, 6.0f);
            }
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            
            g.setFont(lnf->iconFont.withHeight(24));
            g.drawText(iconText, 20, 5, 40, 40, Justification::centredLeft);
            
            g.setFont(lnf->defaultFont.withHeight(16));
            g.drawText(topText, 60, 7, getWidth() - 60, 20, Justification::centredLeft);
            
            g.setFont(lnf->thinFont.withHeight(14));
            g.drawText(bottomText, 60, 25, getWidth() - 60, 16, Justification::centredLeft);
        }
        
        void mouseUp(const MouseEvent& e)
        {
            onClick();
        }
        
        void mouseEnter(const MouseEvent& e)
        {
            repaint();
        }
        
        void mouseExit(const MouseEvent& e)
        {
            repaint();
        }
        
    };
    
    WelcomePanel() {
        newButton = std::make_unique<WelcomeButton>(Icons::New, "New Patch", "Create a new empty patch");
        openButton = std::make_unique<WelcomeButton>(Icons::Open, "Open Patch", "Open a saved patch");
        
        addAndMakeVisible(newButton.get());
        addAndMakeVisible(openButton.get());
    }
    
    void resized() override
    {
        newButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, -30));
        openButton->setBounds(getLocalBounds().withSizeKeepingCentre(275, 50).translated(15, 30));
        
    }
    
    void paint(Graphics& g) override
    {
        auto styles = Font(32).getAvailableStyles();
        
        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        
        g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));
        
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(32));
        g.drawText("No Patch Open", 0, getHeight() / 2 - 150, getWidth(), 40, Justification::centred);
        
        g.setFont(lnf->thinFont.withHeight(23));
        g.drawText("Open a file to begin patching", 0,  getHeight() / 2 - 120, getWidth(), 40, Justification::centred);
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
    }
    
    std::unique_ptr<WelcomeButton> newButton;
    std::unique_ptr<WelcomeButton> openButton;
};

struct TabComponent : public TabbedComponent
{
    std::function<void(int)> onTabChange = [](int) {};
    std::function<void()> newTab = []() {};
    std::function<void()> openProject = [](){};
    
    TextButton newButton = TextButton(Icons::Add);

    TabComponent() : TabbedComponent(TabbedButtonBar::TabsAtTop)
    {
        addAndMakeVisible(newButton);
        newButton.setName("tabbar:newbutton");
        newButton.onClick = [this](){
            newTab();
            
        };

        addAndMakeVisible(welcomePanel);
        
        welcomePanel.newButton->onClick = [this](){
            newTab();
        };
        
        welcomePanel.openButton->onClick = [this](){
            openProject();
        };
  
        setVisible(false);
        setTabBarDepth(0);
        
        
    }
    
    void currentTabChanged(int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        if(getNumTabs() == 0) {
            setTabBarDepth(0);
            getTabbedButtonBar().setVisible(false);
            welcomePanel.setVisible(true);
        }
        else {
            getTabbedButtonBar().setVisible(true);
            welcomePanel.setVisible(false);
            setTabBarDepth(28);
        }
        
        onTabChange(newCurrentTabIndex);
    }
    
    void resized() override
    {
        int depth = getTabBarDepth();
        auto content = getLocalBounds();
        
        welcomePanel.setBounds(content);
        newButton.setBounds(0, 0, depth, depth);
        
        auto tabBounds = content.removeFromTop(depth).withTrimmedLeft(depth);
        tabs->setBounds(tabBounds);

        for(int c = 0; c < getNumTabs(); c++) {
            if (auto* comp = getTabContentComponent(c)) {
                comp->setBounds (content);
            }
        }
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, getTabBarDepth(), getWidth(), getTabBarDepth());
        
        g.drawLine(Line<float>(getTabBarDepth() - 0.5f, 0, getTabBarDepth() - 0.5f, getTabBarDepth()), 1.0f);

        g.drawLine(0, 0, getWidth(), 0);
    }
    
    WelcomePanel welcomePanel;
};
