/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>

#include "Buttons.h"
#include "PluginEditor.h"

class TouchSelectionHelper : public Component {

public:
    TouchSelectionHelper(PluginEditor* editor)
    {
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Copy)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Paste)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Help)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Trash)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::More)));
        
        actionButtons[0]->onClick = [editor](){
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::Copy);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->ApplicationCommandManager::invoke(info, true);
        };
        actionButtons[1]->onClick = [editor](){
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::Paste);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->ApplicationCommandManager::invoke(info, true);
        };
        actionButtons[2]->onClick = [editor](){
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::ShowReference);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->ApplicationCommandManager::invoke(info, true);
        };
        actionButtons[3]->onClick = [editor](){
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::Delete);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->ApplicationCommandManager::invoke(info, true);
        };
        actionButtons[4]->onClick = [editor](){
            OSUtils::showMobileCanvasMenu(editor->getPeer());
        };
        
    }
    
    void resized() override
    {
        auto b = getLocalBounds().reduced(4);
        
        for(auto* button : actionButtons)
        {
            button->setBounds(b.removeFromLeft(36));
        }
    }

private:

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(4);
        
        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 6, { 0, 1 });
        
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    OwnedArray<MainToolbarButton> actionButtons;
};
