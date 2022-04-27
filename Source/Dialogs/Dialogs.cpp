/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Dialogs.h"
#include "LookAndFeel.h"

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Standalone/PlugDataWindow.h"

#include <JuceHeader.h>

#include <memory>

#include "SaveDialog.h"
#include "ArrayDialog.h"
#include "SettingsDialog.h"

void Dialogs::showSaveDialog(Component* centre, String filename, std::function<void(int)> callback)
{
    auto* dialog = new SaveDialog(centre, filename);
    dialog->cb = std::move(callback);

    centre->getTopLevelComponent()->addAndMakeVisible(dialog);

    dialog->setBounds((centre->getWidth() / 2.) - 200., 60, 400, 130);
}
void Dialogs::showArrayDialog(Component* centre, std::function<void(int, String, String)> callback)
{
    auto* dialog = new ArrayDialog(centre);
    dialog->cb = std::move(callback);

    centre->getTopLevelComponent()->addAndMakeVisible(dialog);

    dialog->setBounds((centre->getWidth() / 2.) - 200., 60, 300, 180);
}

Component::SafePointer<Component> Dialogs::createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree)
{
    return new SettingsDialog(processor, manager, settingsTree);
}

void Dialogs::showObjectMenu(PlugDataPluginEditor* parent, Component* target)
{
    PopupMenu menu;

    // Custom function because JUCE adds "shortcut:" before some keycommands, which looks terrible!
    auto createCommandItem = [parent](const CommandID commandID, const String& displayName)
    {
        ApplicationCommandInfo info(*parent->getCommandForID(commandID));
        auto* target = parent->ApplicationCommandManager::getTargetForCommand(commandID, info);

        PopupMenu::Item i;
        i.text = displayName;
        i.itemID = (int)commandID;
        i.commandManager = parent;
        i.isEnabled = target != nullptr && (info.flags & ApplicationCommandInfo::isDisabled) == 0;

        String shortcutKey;

        for (auto& keypress : parent->getKeyMappings()->getKeyPressesAssignedToCommand(commandID))
        {
            auto key = keypress.getTextDescriptionWithIcons();

            if (shortcutKey.isNotEmpty()) shortcutKey << ", ";

            if (key.length() == 1 && key[0] < 128)
                shortcutKey << key;
            else
                shortcutKey << key;
        }

        i.shortcutKeyDescription = shortcutKey.trim();

        return i;
    };

    menu.addItem(createCommandItem(CommandIDs::NewObject, "Empty Object"));
    menu.addSeparator();
    menu.addItem(createCommandItem(CommandIDs::NewNumbox, "Number"));
    menu.addItem(createCommandItem(CommandIDs::NewMessage, "Message"));
    menu.addItem(createCommandItem(CommandIDs::NewBang, "Bang"));
    menu.addItem(createCommandItem(CommandIDs::NewToggle, "Toggle"));
    menu.addItem(createCommandItem(CommandIDs::NewVerticalSlider, "Vertical Slider"));
    menu.addItem(createCommandItem(CommandIDs::NewHorizontalSlider, "Horizontal Slider"));
    menu.addItem(createCommandItem(CommandIDs::NewVerticalRadio, "Vertical Radio"));
    menu.addItem(createCommandItem(CommandIDs::NewHorizontalRadio, "Horizontal Radio"));

    menu.addSeparator();

    menu.addItem(createCommandItem(CommandIDs::NewFloatAtom, "Float Atom"));
    menu.addItem(createCommandItem(CommandIDs::NewSymbolAtom, "Symbol Atom"));
    menu.addItem(createCommandItem(CommandIDs::NewListAtom, "List Atom"));

    menu.addSeparator();

    menu.addItem(createCommandItem(CommandIDs::NewArray, "Array"));
    menu.addItem(createCommandItem(CommandIDs::NewGraphOnParent, "GraphOnParent"));
    menu.addItem(createCommandItem(CommandIDs::NewComment, "Comment"));
    menu.addItem(createCommandItem(CommandIDs::NewCanvas, "Canvas"));
    menu.addSeparator();
    menu.addItem(createCommandItem(CommandIDs::NewKeyboard, "Keyboard"));
    menu.addItem(createCommandItem(CommandIDs::NewVUMeter, "VU Meter"));

    menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(target).withParentComponent(parent));
}
