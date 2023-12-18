/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// This class is a workaround for the poor customisability of JUCE popupmenus
// We simply want an arrow at the top that points to the origin of the menu,
// but we cannot solve this through the LookAndFeel because there is no real way to get the screen bounds of the actual menu
// So instead, we have to get the menu by using Component::getCurrentlyModalComponent()

class ArrowPopupMenu : public Component
    , public ComponentListener {
public:
    explicit ArrowPopupMenu(Component* target)
        : targetComponent(target)
    {
        setWantsKeyboardFocus(false);
        setInterceptsMouseClicks(false, false);
    }

    ~ArrowPopupMenu() override
    {
        if (menuComponent) {
            menuComponent->removeComponentListener(this);
        }
    }

    void attachToMenu(Component* menuToAttachTo, Component* parent)
    {
        menuParent = parent;
        menuComponent = menuToAttachTo;

        menuComponent->addComponentListener(this);

        setAlwaysOnTop(true);
        setVisible(true);

        auto menuMargin = getLookAndFeel().getPopupMenuBorderSize();

        // Apply a slight offset to the menu so we have enough space for the arrow...
        menuToAttachTo->setBounds(menuToAttachTo->getBounds().translated(-15, menuMargin - 3));
    }

    void paint(Graphics& g) override
    {
        auto targetArea = getLocalArea(targetComponent, targetComponent->getLocalBounds());

        auto arrowHeight = 12;
        auto arrowWidth = 22;

        Path arrow;
        Rectangle<float> arrowBounds;
        Rectangle<float> extensionBounds;
        int verticalMargin = Desktop::canUseSemiTransparentWindows() ? 6 : 1;
        // Check if we need to draw an arrow up or down
        if (targetArea.getY() <= menuBounds.getY()) {
            arrowBounds = Rectangle<float>(targetArea.getCentreX() - (arrowWidth / 2.0f), menuBounds.getY() - arrowHeight + verticalMargin, arrowWidth, arrowHeight);

            extensionBounds = arrowBounds;
            extensionBounds = extensionBounds.removeFromBottom(1).withTrimmedBottom(-2).reduced(1, 0);

            arrow.startNewSubPath(arrowBounds.getBottomLeft());
            arrow.lineTo(arrowBounds.getCentreX(), arrowBounds.getY());
            arrow.lineTo(arrowBounds.getBottomRight());
        } else {
            arrowBounds = Rectangle<float>(targetArea.getCentreX() - (arrowWidth / 2.0f), menuBounds.getBottom() - verticalMargin, arrowWidth, arrowHeight);

            extensionBounds = arrowBounds;
            extensionBounds = extensionBounds.removeFromTop(1).withTrimmedTop(-2).reduced(1, 0);

            arrow.startNewSubPath(arrowBounds.getTopLeft());
            arrow.lineTo(arrowBounds.getCentreX(), arrowBounds.getBottom());
            arrow.lineTo(arrowBounds.getTopRight());
        }

        auto arrowOutline = arrow;
        arrow.closeSubPath();

        // Reduce clip region before drawing shadow to ensure there's no shadow at the bottom
        if (ProjectInfo::canUseSemiTransparentWindows()) {
            g.saveState();
            g.reduceClipRegion(arrowBounds.toNearestInt());

            StackShadow::renderDropShadow(g, arrowOutline, Colour(0, 0, 0).withAlpha(0.3f), 8, { 0, targetArea.getY() <= menuBounds.getY() ? 1 : -1 });

            g.restoreState();
        }

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillRect(extensionBounds);
        g.fillPath(arrow);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(arrowOutline, PathStrokeType(1.0f));
    }

    static void showMenuAsync(PopupMenu* menu, PopupMenu::Options const& options, std::function<void(int)> const& userCallback)
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

    void componentMovedOrResized(Component& component, bool moved, bool resized) override
    {
        if (!menuComponent)
            return;

        auto menuMargin = getLookAndFeel().getPopupMenuBorderSize();

        if (menuParent) {
            auto targetBounds = menuParent->getLocalArea(targetComponent, targetComponent->getLocalBounds());
            auto menuTop = menuParent->getLocalArea(menuComponent.getComponent(), menuComponent->getLocalBounds()).removeFromTop(menuMargin + 1);

            menuParent->addAndMakeVisible(this);
            setBounds(targetBounds.getUnion(menuTop));
        } else {
            addToDesktop(ComponentPeer::windowIsTemporary);
            setBounds(targetComponent->getScreenBounds().getUnion(menuComponent->getScreenBounds().removeFromTop(menuMargin + 1)));
        }

        menuBounds = getLocalArea(menuComponent.getComponent(), menuComponent->getLocalBounds());
    }

private:
    Rectangle<int> menuBounds;
    Component* targetComponent = nullptr;
    Component* menuParent = nullptr;
    SafePointer<Component> menuComponent;
};
