#ifndef MENU_ITEM_DEFINED
struct TouchPopupMenuItem // needs to be included in OSUtils for the iOS native touch menu
{
    juce::String title;
    std::function<void()> callback;
    bool active;
    int subMenuIndex = -1;
};
#define MENU_ITEM_DEFINED 1
#endif

#ifndef ONLY_MENU_ITEM_DEF
#pragma once
class TouchPopupMenu
{
    class MenuComponent : public Component
    {
    public:
        
        MenuComponent(String const& title, SmallArray<TouchPopupMenuItem>& items, SmallArray<SmallArray<TouchPopupMenuItem>> sub) : menuTitle(title), currentTitle(title), menuItems(items), subMenus(sub)
        {
            updateSize();
            setVisible(true);
        }
        
        void updateSize()
        {
            auto width = 220;
            for(auto& item : currentItems())
                width = std::max(Fonts::getStringWidthInt(item.title) + 40, width);

            auto height = getNumMenuItems() * itemHeight + 36;
            setSize(width, height);
            repaint();
        }

        void navigateTo(int subMenuIndex)
        {
            navigationStack.add(subMenuIndex);
            updateSize();
        }

        void navigateBack()
        {
            if (navigationStack.size() > 0)
                navigationStack.pop_back();
            updateSize();
        }
        
        int getNumMenuItems()
        {
            if (navigationStack.empty())
                return menuItems.size();
            
            return subMenus[navigationStack.back()].size() + 1;
        }

        SmallArray<TouchPopupMenuItem>& currentItems()
        {
            if (navigationStack.empty())
                return menuItems;
            return subMenus[navigationStack.back()];
        }

        void mouseDown(const MouseEvent& e) override
        {
            auto pos = e.y - 36;
            pressedIndex = static_cast<int>(pos / itemHeight);
            repaint();
        }
        
        void mouseDrag(const MouseEvent& e) override
        {
            auto pos = e.y - 36;
            pressedIndex = static_cast<int>(pos / itemHeight);
            repaint();
        }

        void mouseUp(const MouseEvent& e) override
        {
            pressedIndex = -1;
            auto& items = currentItems();
            
            bool inSubMenu = !navigationStack.empty();
            int pos = (int)((e.y - 36) / itemHeight);

            if (inSubMenu)
            {
                if (pos == 0)
                {
                    currentTitle = menuTitle;
                    navigateBack();
                    return;
                }
                pos -= 1;
            }

            if (pos >= 0 && pos < items.size())
            {
                auto& item = items[pos];
                if (item.active && item.subMenuIndex >= 0) {
                    currentTitle = item.title;
                    navigateTo(item.subMenuIndex);
                }
                else if (item.active && item.callback)
                {
                    if(currentCalloutBox)
                        currentCalloutBox->dismiss();
                    item.callback();
                }
            }
            repaint();
        }

        void paint(Graphics& g) override
        {
            auto b = getLocalBounds();
            auto& items = currentItems();
            bool inSubMenu = !navigationStack.empty();
            
            // NOTE: title will break if we have submenus nesting deeper than 1
            g.setColour(PlugDataColours::popupMenuTextColour);
            g.setFont(Fonts::getSemiBoldFont().withHeight(15.f));
            g.drawText(currentTitle, b.removeFromTop(34), Justification::centred);

            g.setColour(PlugDataColours::outlineColour.withAlpha(0.7f));
            g.fillRect(b.removeFromTop(1).reduced(12, 0));
            
            g.setFont(Fonts::getDefaultFont().withHeight(16.f));

            int rowIndex = 0;

            // Back button
            if (inSubMenu)
            {
                auto row = b.removeFromTop(itemHeight);
                bool pressed = pressedIndex == rowIndex;

                if (pressed)
                {
                    g.setColour(PlugDataColours::popupMenuActiveBackgroundColour);
                    g.fillRoundedRectangle(row.toFloat().reduced(4), Corners::defaultCornerRadius);
                }

                auto arrowH = 0.6f * g.getCurrentFont().getAscent();
                auto arrowArea = row.reduced(8, 0);
                auto x = static_cast<float>(arrowArea.getX()) + arrowH;
                auto halfH = static_cast<float>(row.getCentreY());

                Path backArrow;
                backArrow.startNewSubPath(x, halfH - arrowH * 0.5f);
                backArrow.lineTo(x - arrowH * 0.5f, halfH);
                backArrow.lineTo(x, halfH + arrowH * 0.5f);
                g.setColour(PlugDataColours::popupMenuTextColour);
                g.strokePath(backArrow, PathStrokeType(1.5f));

                Fonts::drawFittedText(g, "Back", row.reduced(8, 0), PlugDataColours::popupMenuTextColour, 1, 1.0f, 15, Justification::centred);

                g.setColour(PlugDataColours::outlineColour.withAlpha(0.7f));
                g.fillRect(row.removeFromBottom(1).reduced(12, 0));
            
                rowIndex++;
            }

            for (int i = 0; i < items.size(); ++i, ++rowIndex)
            {
                auto& item = items[i];
                auto itemTextColour = PlugDataColours::popupMenuTextColour.withAlpha(item.active ? 1.0f : 0.5f);
                auto row = b.removeFromTop(itemHeight);
                bool pressed = pressedIndex == rowIndex;

                if (item.active && pressed)
                {
                    g.setColour(PlugDataColours::popupMenuActiveBackgroundColour);
                    g.fillRoundedRectangle(row.toFloat().reduced(4), Corners::defaultCornerRadius);
                }

                if (i < items.size() - 1)
                {
                    g.setColour(PlugDataColours::outlineColour.withAlpha(0.7f));
                    g.fillRect(row.removeFromBottom(1).reduced(12, 0));
                }

                auto textArea = row.reduced(12, 0);

                // Submenu chevron
                if (item.subMenuIndex >= 0)
                {
                    auto arrowH = 0.6f * g.getCurrentFont().getAscent();
                    auto x = static_cast<float>(textArea.removeFromRight(static_cast<int>(arrowH) + 3).getX());
                    auto halfH = static_cast<float>(row.getCentreY());

                    Path path;
                    path.startNewSubPath(x, halfH - arrowH * 0.5f);
                    path.lineTo(x + arrowH * 0.5f, halfH);
                    path.lineTo(x, halfH + arrowH * 0.5f);

                    g.setColour(itemTextColour.withMultipliedAlpha(0.5f));
                    g.strokePath(path, PathStrokeType(1.5f));
                }

                Fonts::drawFittedText(g, item.title, textArea, itemTextColour, 1, 1.0f, 15, Justification::centred);
            }
        }


        static constexpr int itemHeight = 40;
    private:
        String menuTitle;
        String currentTitle;
        int pressedIndex = -1;
        SmallArray<int> navigationStack;
        SmallArray<TouchPopupMenuItem> menuItems;
        SmallArray<SmallArray<TouchPopupMenuItem>> subMenus;
    };

public:
    
    void addItem(String title, std::function<void()> callback, bool active = true)
    {
        menuItems.add({title, std::move(callback), active});
    }

    void addSubMenu(String title, TouchPopupMenu& submenu, bool active = true)
    {
        int index = subMenus.size();
        subMenus.add(SmallArray<TouchPopupMenuItem>(submenu.menuItems));
        menuItems.add({title, {}, active, index});
    }

    void showMenu(PluginEditor* editor, Component* centre, String const& title)
    {
#if JUCE_IOS
        auto position = centre->getScreenPosition() + Point<int>(centre->getWidth() * 0.5f, 0);
        OSUtils::showiOSNativeMenu(editor->getPeer(), title, menuItems, subMenus, position);
#else
        currentCalloutBox = &editor->showCalloutBox(std::make_unique<MenuComponent>(title, menuItems, subMenus), centre->getScreenBounds());
#endif
    }
    
private:
    SmallArray<TouchPopupMenuItem> menuItems;
    SmallArray<SmallArray<TouchPopupMenuItem>> subMenus;
    static inline Component::SafePointer<CallOutBox> currentCalloutBox;
};
#endif
