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

            auto shiftIcon = String(CharPointer_UTF8("\xe2\x87\xa7"));
            if (key.contains(shiftIcon)) {
                key = key.replace(shiftIcon, "shift-");
            }
            if (shortcutKey.isNotEmpty())
                shortcutKey << ", ";

            shortcutKey << key;
        }

        i.shortcutKeyDescription = shortcutKey.trim();

        return i;
    };

    menu.addItem("Open Object Browser...", [parent]() mutable {
        Dialogs::showObjectBrowserDialog(&parent->openedDialog, parent);
    });
    
    
    PopupMenu guiMenu;
    {
        guiMenu.addItem(createCommandItem(ObjectIDs::NewNumbox, "Number"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewBang, "Bang"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewToggle, "Toggle"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewButton, "Button"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewVerticalSlider, "Vertical Slider"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewHorizontalSlider, "Horizontal Slider"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewVerticalRadio, "Vertical Radio"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewHorizontalRadio, "Horizontal Radio"));
        
        guiMenu.addSeparator();
        guiMenu.addItem(createCommandItem(ObjectIDs::NewCanvas, "Canvas"));

        guiMenu.addItem(createCommandItem(ObjectIDs::NewKeyboard, "Keyboard"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewVUMeterObject, "VU Meter"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewNumboxTilde, "Signal Numbox"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewOscilloscope, "Oscilloscope"));
        guiMenu.addItem(createCommandItem(ObjectIDs::NewFunction, "Function"));
    }
    
    PopupMenu timeMenu;
    {
        timeMenu.addItem(createCommandItem(ObjectIDs::NewMetro, "metro"));
        timeMenu.addItem(createCommandItem(ObjectIDs::NewTimer, "timer"));
        timeMenu.addItem(createCommandItem(ObjectIDs::NewDelay, "delay"));
        timeMenu.addItem(createCommandItem(ObjectIDs::NewTimedGate, "timed.gate"));
        timeMenu.addItem(createCommandItem(ObjectIDs::NewDateTime, "datetime"));
        timeMenu.addSeparator();
        timeMenu.addItem(createCommandItem(ObjectIDs::NewSignalDelay, "delay~"));
    }
    
    PopupMenu filtersMenu;
    {
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewLop, "lop~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewVcf, "vcf~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewLores, "lores~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewSvf, "svf~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewBob, "bob~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewOnepole, "onepole~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewReson, "reson~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewAllpass, "allpass~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewComb, "comb~"));
        filtersMenu.addItem(createCommandItem(ObjectIDs::NewHip, "hip~"));
    }
    
    PopupMenu oscillatorsMenu;
    {
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewOsc, "osc~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewPhasor, "phasor~"));
        oscillatorsMenu.addSeparator();
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewSaw, "saw~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewSaw2, "saw2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewSquare, "square~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewTriangle, "triangle~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewImp, "imp~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewImp2, "imp2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewWavetable, "wavetable~"));
        oscillatorsMenu.addSeparator();
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlOsc, "bl.osc~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSaw, "bl.saw~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSaw2, "bl.saw2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSquare, "bl.square~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlTriangle, "bl.triangle~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlImp, "bl.imp~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlImp2, "bl.imp2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlWavetable, "bl.wavetable"));

        
    }    
    PopupMenu IOMenu;
    {
        IOMenu.addItem(createCommandItem(ObjectIDs::NewAdc, "adc~"));
        IOMenu.addItem(createCommandItem(ObjectIDs::NewDac, "dac~"));
        IOMenu.addItem(createCommandItem(ObjectIDs::NewOut, "out~"));
    }
    
    PopupMenu midiMenu;
    {
        midiMenu.addItem(createCommandItem(ObjectIDs::NewMidiIn, "midiin"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewMidiOut, "midiout"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewNoteIn, "notein"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewNoteOut, "noteout"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewCtlIn, "ctlin"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewCtlOut, "ctlout"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewPgmIn, "pgmin"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewPgmOut, "pgmout"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewSysexIn, "sysexin"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewSysexOut, "sysexout"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewMtof, "mtof"));
        midiMenu.addItem(createCommandItem(ObjectIDs::NewFtom, "ftom"));

    }

    menu.addSeparator();
    
    menu.addSubMenu("GUI", guiMenu);
    menu.addSubMenu("Time", timeMenu);
    menu.addSubMenu("Filters", filtersMenu);
    menu.addSubMenu("Oscillators", oscillatorsMenu);
    menu.addSubMenu("IO", IOMenu);
    menu.addSubMenu("MIDI", midiMenu);
    
    menu.addSeparator();
    
    menu.addItem(createCommandItem(ObjectIDs::NewObject, "Empty Object"));
    menu.addItem(createCommandItem(ObjectIDs::NewMessage, "New Message"));
    menu.addItem(createCommandItem(ObjectIDs::NewFloatAtom, "Float box"));
    menu.addItem(createCommandItem(ObjectIDs::NewSymbolAtom, "Symbol box"));
    menu.addItem(createCommandItem(ObjectIDs::NewListAtom, "List box"));
    menu.addItem(createCommandItem(ObjectIDs::NewComment, "Comment"));
    menu.addSeparator();
    
    menu.addItem(createCommandItem(ObjectIDs::NewArray, "Array..."));
    menu.addItem(createCommandItem(ObjectIDs::NewGraphOnParent, "GraphOnParent"));

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
