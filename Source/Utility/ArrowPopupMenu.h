/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// This class is a workaround for the poor customisability of JUCE popupmenus
// We simply want an arrow at the top that points to the origin of the menu,
// but we cannot solve this through the LookAndFeel because there is no real way to get the screen bounds of the actual menu
// So instead, we have to get the menu by using Component::getCurrentlyModalComponent()


class ArrowPopupMenu : public Component{
public:
    ArrowPopupMenu(Component* target)
        : targetComponent(target)
    {
        setWantsKeyboardFocus(false);
        setInterceptsMouseClicks(false, false);
    }

    void attachToMenu(Component* menuToAttachTo, Component* parent)
    {
        
        setAlwaysOnTop(true);
        setVisible(true);
        
        auto menuMargin = getLookAndFeel().getPopupMenuBorderSize();
        
        // Apply a slight offset to the menu so we have enough space for the arrow...
        menuToAttachTo->setBounds(menuToAttachTo->getBounds().translated(-15, menuMargin - 3));
        
        if(parent)
        {
            auto targetBounds = parent->getLocalArea(targetComponent, targetComponent->getLocalBounds());
            auto menuTop = parent->getLocalArea(menuToAttachTo, menuToAttachTo->getLocalBounds()).removeFromTop(menuMargin + 1);
            parent->addAndMakeVisible(this);
            setBounds(targetBounds.getUnion(menuTop));
        }
        else
        {
            addToDesktop(ComponentPeer::windowIsTemporary);
            setBounds(targetComponent->getScreenBounds().getUnion(menuToAttachTo->getScreenBounds().removeFromTop(menuMargin + 1)));
        }

        menuBounds = getLocalArea(menuToAttachTo, menuToAttachTo->getLocalBounds());
    }

    void paint(Graphics& g) override
    {
        auto localArea = getLocalArea(targetComponent, targetComponent->getLocalBounds());

        auto margin = getLookAndFeel().getPopupMenuBorderSize();
        
        // Heuristic to make it align for all popup menu margins
        auto menuMargin = margin - jmap<float>(margin, 2.0f, 10.0f, 0.5f, 2.5f);
        
        auto arrowHeight = 12;
        auto arrowWidth = 22;

        auto arrowBounds = Rectangle<float>(localArea.getCentreX() - (arrowWidth / 2.0f), menuBounds.getY() - arrowHeight + menuMargin, arrowWidth, arrowHeight);
        
        Path arrow;
        arrow.startNewSubPath(arrowBounds.getBottomLeft());
        arrow.lineTo(arrowBounds.getCentreX(), arrowBounds.getY());
        arrow.lineTo(arrowBounds.getBottomRight());
        
        auto arrowOutline = arrow;
        arrow.closeSubPath();
        
        // Reduce clip region before drawing shadow to ensure there's no shadow at the bottom
        g.saveState();
        g.reduceClipRegion(arrowBounds.toNearestInt());
        
        StackShadow::renderDropShadow(g, arrowOutline, Colour(0, 0, 0).withAlpha(0.25f));
        
        g.restoreState();
        
        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillPath(arrow);
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(arrowOutline, PathStrokeType(1.0f));
        
    }

    static void showMenuAsync(PopupMenu* menu, const PopupMenu::Options& options, std::function<void (int)> userCallback)
    {
        auto* target = options.getTargetComponent();
        
        auto* arrow = new ArrowPopupMenu(target);
 
        menu->showMenuAsync(options, [userCallback, arrow](int result) {
            arrow->removeFromDesktop();
            delete arrow;
            userCallback(result);
        });

        if (auto* popupMenuComponent = Component::getCurrentlyModalComponent(0)) {
            arrow->attachToMenu(popupMenuComponent, options.getParentComponent());
        }
    }

private:
    
    Rectangle<int> menuBounds;
    Component* targetComponent;
};
