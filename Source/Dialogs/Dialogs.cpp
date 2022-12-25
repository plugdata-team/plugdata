/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#include <JuceHeader.h>

#include "Dialogs.h"

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "SaveDialog.h"
#include "ArrayDialog.h"
#include "SettingsDialog.h"
#include "TextEditorDialog.h"
#include "ObjectBrowserDialog.h"
#include "ObjectReferenceDialog.h"
#include "../Heavy/HeavyExportDialog.h"
#include "Canvas.h"

Component* Dialogs::showTextEditorDialog(String text, String filename, std::function<void(String, bool)> callback)
{
    auto* editor = new TextEditorDialog(filename);
    editor->editor.setText(text);
    editor->onClose = std::move(callback);
    return editor;
}

void Dialogs::showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String filename, std::function<void(int)> callback)
{
    if (*target)
        return;

    auto* dialog = new Dialog(target, centre, 400, 130, 160, false);
    auto* saveDialog = new SaveDialog(centre, dialog, filename, callback);

    dialog->setViewedComponent(saveDialog);
    target->reset(dialog);
}
void Dialogs::showArrayDialog(std::unique_ptr<Dialog>* target, Component* centre, std::function<void(int, String, String)> callback)
{
    if (*target)
        return;

    auto* dialog = new Dialog(target, centre, 300, 180, 200, false);
    auto* arrayDialog = new ArrayDialog(centre, dialog, callback);
    dialog->setViewedComponent(arrayDialog);
    target->reset(dialog);
}

void Dialogs::createSettingsDialog(AudioProcessor* processor, AudioDeviceManager* manager, Component* centre, ValueTree const& settingsTree)
{
    SettingsPopup::showSettingsPopup(processor, manager, centre, settingsTree);
}

void Dialogs::showObjectMenu(PluginEditor* parent, Component* target)
{
    PopupMenu menu;

    // Custom function because JUCE adds "shortcut:" before some keycommands, which looks terrible!
    auto createCommandItem = [parent](const CommandID commandID, String const& displayName) {
        ApplicationCommandInfo info(*parent->getCommandForID(commandID));
        auto* target = parent->ApplicationCommandManager::getTargetForCommand(commandID, info);

        PopupMenu::Item i;
        i.text = displayName;
        i.itemID = (int)commandID;
        i.commandManager = parent;
        i.isEnabled = target != nullptr && (info.flags & ApplicationCommandInfo::isDisabled) == 0;

        String shortcutKey;

        for (auto& keypress : parent->getKeyMappings()->getKeyPressesAssignedToCommand(commandID)) {
            auto key = keypress.getTextDescriptionWithIcons();

            if (shortcutKey.isNotEmpty())
                shortcutKey << ", ";

            shortcutKey << key;
        }

        i.shortcutKeyDescription = shortcutKey.trim();

        return i;
    };

    menu.addItem("Open Object Browser", [parent]() mutable {
        Dialogs::showObjectBrowserDialog(&parent->openedDialog, parent);
    });

    menu.addSeparator();
    menu.addItem(createCommandItem(CommandIDs::NewObject, "Empty Object"));
    menu.addSeparator();
    menu.addItem(createCommandItem(CommandIDs::NewNumbox, "Number"));
    menu.addItem(createCommandItem(CommandIDs::NewMessage, "Message"));
    menu.addItem(createCommandItem(CommandIDs::NewBang, "Bang"));
    menu.addItem(createCommandItem(CommandIDs::NewToggle, "Toggle"));
    menu.addItem(createCommandItem(CommandIDs::NewButton, "Button"));
    menu.addItem(createCommandItem(CommandIDs::NewVerticalSlider, "Vertical Slider"));
    menu.addItem(createCommandItem(CommandIDs::NewHorizontalSlider, "Horizontal Slider"));
    menu.addItem(createCommandItem(CommandIDs::NewVerticalRadio, "Vertical Radio"));
    menu.addItem(createCommandItem(CommandIDs::NewHorizontalRadio, "Horizontal Radio"));

    menu.addSeparator();

    menu.addItem(createCommandItem(CommandIDs::NewFloatAtom, "Float box"));
    menu.addItem(createCommandItem(CommandIDs::NewSymbolAtom, "Symbol box"));
    menu.addItem(createCommandItem(CommandIDs::NewListAtom, "List box"));
    menu.addSeparator();

    menu.addItem(createCommandItem(CommandIDs::NewArray, "Array"));
    menu.addItem(createCommandItem(CommandIDs::NewGraphOnParent, "GraphOnParent"));
    menu.addItem(createCommandItem(CommandIDs::NewComment, "Comment"));
    menu.addItem(createCommandItem(CommandIDs::NewCanvas, "Canvas"));
    menu.addSeparator();
    menu.addItem(createCommandItem(CommandIDs::NewKeyboard, "Keyboard"));
    menu.addItem(createCommandItem(CommandIDs::NewVUMeterObject, "VU Meter"));
    menu.addItem(createCommandItem(CommandIDs::NewNumboxTilde, "Signal Numbox"));
    menu.addItem(createCommandItem(CommandIDs::NewOscilloscope, "Oscilloscope"));
    menu.addItem(createCommandItem(CommandIDs::NewFunction, "Function"));

    menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(target).withParentComponent(parent),
        [parent](int result) {
            if (result > 0) {
                if (auto* cnv = parent->getCurrentCanvas()) {
                    cnv->attachNextObjectToMouse = true;
                }
            }
        });
}

struct OkayCancelDialog : public Component {

    OkayCancelDialog(Dialog* dialog, String const& title, std::function<void(bool)> callback)
        : label("", title)
    {
        setSize(400, 200);
        addAndMakeVisible(label);
        addAndMakeVisible(cancel);
        addAndMakeVisible(okay);

        cancel.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        okay.setColour(TextButton::buttonColourId, Colours::transparentBlack);

        cancel.onClick = [this, dialog, callback] {
            callback(false);
            dialog->closeDialog();
        };

        okay.onClick = [this, dialog, callback] {
            callback(true);
            dialog->closeDialog();
        };

        cancel.changeWidthToFitText();
        okay.changeWidthToFitText();
        setOpaque(false);
    }

    void resized() override
    {
        label.setBounds(20, 25, 360, 30);
        cancel.setBounds(20, 80, 80, 25);
        okay.setBounds(300, 80, 80, 25);
    }

private:
    Label label;

    TextButton cancel = TextButton("Cancel");
    TextButton okay = TextButton("OK");
};

void Dialogs::showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> callback)
{
    auto* dialog = new Dialog(target, parent, 400, 130, 160, false);
    auto* dialogContent = new OkayCancelDialog(dialog, title, callback);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent)
{
    auto* dialog = new Dialog(target, parent, 625, 400, parent->getBounds().getCentreY() + 200, true);
    auto* dialogContent = new HeavyExportDialog(dialog);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent)
{

    auto* dialog = new Dialog(target, parent, 750, 450, parent->getBounds().getCentreY() + 200, true);
    auto* dialogContent = new ObjectBrowserDialog(parent, dialog);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String objectName)
{
    auto* dialog = new Dialog(target, parent, 750, 450, parent->getBounds().getCentreY() + 200, true);
    auto* dialogContent = new ObjectReferenceDialog(dynamic_cast<PluginEditor*>(parent), false);

    dialogContent->showObject(objectName);
    
    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

StringArray DekenInterface::getExternalPaths()
{
    StringArray searchPaths;

    for (auto package : PackageManager::getInstance()->packageState) {
        if (!package.hasProperty("AddToPath") || !static_cast<bool>(package.getProperty("AddToPath"))) {
            continue;
        }

        searchPaths.add(package.getProperty("Path"));
    }

    return searchPaths;
}

bool Dialog::wantsRoundedCorners()
{
    // Check if the editor wants rounded corners
    if (auto* editor = dynamic_cast<PluginEditor*>(parentComponent)) {
        return editor->wantsRoundedCorners();
    }
    // Otherwise assume rounded corners for the rest of the UI
    else {
        return true;
    }
}
