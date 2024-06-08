/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>

#include "Buttons.h"
#include "PluginEditor.h"
#include "Objects/ObjectBase.h"

class TouchSelectionHelper : public Component
    , public NVGComponent {

    PluginEditor* editor;

public:
    TouchSelectionHelper(PluginEditor* e)
        : NVGComponent(this)
        , editor(e)
    {
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::ExportState))); // This icon doubles as a "open" icon in the mobile app
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Help)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::Trash)));
        addAndMakeVisible(actionButtons.add(new MainToolbarButton(Icons::More)));

        setCachedComponentImage(new NVGSurface::InvalidationListener(e->nvgSurface, this));

        actionButtons[0]->onClick = [this]() {
            auto* cnv = editor->getCurrentCanvas();
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() == 1 && selection[0]->gui) {
                selection[0]->gui->openFromMenu();
            }
        };
        actionButtons[1]->onClick = [this]() {
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::ShowHelp);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->invoke(info, true);
        };
        actionButtons[2]->onClick = [this]() {
            ApplicationCommandTarget::InvocationInfo info(CommandIDs::Delete);
            info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
            editor->invoke(info, true);
        };

#if JUCE_IOS
        actionButtons[3]->onClick = [this]() {
            OSUtils::showMobileCanvasMenu(editor->getPeer(), [this](int result) {
                if (result < 1)
                    return;
                switch (result) {
                case 1: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::Cut);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                case 2: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::Copy);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                case 3: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::Paste);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                case 4: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::Duplicate);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                case 5: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::Encapsulate);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                case 6: {
                    ApplicationCommandTarget::InvocationInfo info(CommandIDs::ConnectionPathfind);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    editor->commandManager.invoke(info, true);
                    break;
                }
                }
            });
        };
#endif
    }

    void show()
    {
        actionButtons[1]->setEnabled(editor->isCommandActive(CommandIDs::ShowHelp));
        actionButtons[2]->setEnabled(editor->isCommandActive(CommandIDs::Delete));
        setVisible(true);
        toFront(false);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(5);

        for (auto* button : actionButtons) {
            button->setBounds(b.removeFromLeft(48));
        }
    }

    void render(NVGcontext* nvg) override
    {
        componentImage.renderJUCEComponent(nvg, *this, 2.0f);
    }

private:
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(5);

        Path p;
        p.addRoundedRectangle(b.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 9, { 0, 1 });

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(b.toFloat(), Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(b.toFloat(), Corners::largeCornerRadius, 1.0f);
    }

    NVGImage componentImage;
    OwnedArray<MainToolbarButton> actionButtons;
};
