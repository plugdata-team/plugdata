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
#include "MainMenu.h"
#include "Canvas.h"

Component* Dialogs::showTextEditorDialog(String text, String filename, std::function<void(String, bool)> callback)
{
    auto* editor = new TextEditorDialog(filename);
    editor->editor.setText(text);
    editor->onClose = std::move(callback);
    return editor;
}

void Dialogs::showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String filename, std::function<void(int)> callback, int margin)
{
    if (*target)
        return;

    auto* dialog = new Dialog(target, centre, 400, 130, 160, false, margin);
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

void Dialogs::showSettingsDialog(PluginEditor* editor)
{
    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
    auto* settingsDialog = new SettingsDialog(editor, dialog);
    dialog->setViewedComponent(settingsDialog);
    editor->openedDialog.reset(dialog);
}

void Dialogs::showMainMenu(PluginEditor* editor, Component* centre)
{
    auto* popup = new MainMenu(editor);

    popup->showMenuAsync(PopupMenu::Options().withMinimumWidth(220).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
        [editor, popup, centre, settingsTree = SettingsFile::getInstance()->getValueTree()](int result) mutable {
            switch(result) {
            case MainMenu::menuItem::newPatch: {
                editor->newProject();
                break;
            }
            case MainMenu::menuItem::openPatch: {
                editor->openProject();
                break;
            }
            case MainMenu::menuItem::save: {
                if (editor->getCurrentCanvas())
                    editor->saveProject();
                break;
            }
            case MainMenu::menuItem::saveAs: {
                if (editor->getCurrentCanvas())
                    editor->saveProjectAs();
                break;
            }
            case MainMenu::menuItem::close: {
                if (editor->getCurrentCanvas())
                    editor->closeTab(editor->getCurrentCanvas());
                break;
            }
            case MainMenu::menuItem::closeAll: {
                if (editor->getCurrentCanvas())
                    editor->closeAllTabs();
                break;
            }
            case MainMenu::menuItem::compiledMode: {
                bool ticked = settingsTree.hasProperty("hvcc_mode") ? static_cast<bool>(settingsTree.getProperty("hvcc_mode")) : false;
                settingsTree.setProperty("hvcc_mode", !ticked, nullptr);
                break;
            }
            case MainMenu::menuItem::compile: {
                Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
                break;
            }
            case MainMenu::menuItem::autoConnect: {
                bool ticked = settingsTree.hasProperty("autoconnect") ? static_cast<bool>(settingsTree.getProperty("autoconnect")) : false;
                settingsTree.setProperty("autoconnect", !ticked, nullptr);
                break;
            }
            case MainMenu::menuItem::settings: {
                Dialogs::showSettingsDialog(editor);
                break;
            }
            case MainMenu::menuItem::about: {
                auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                auto* aboutPanel = new AboutPanel();
                dialog->setViewedComponent(aboutPanel);
                editor->openedDialog.reset(dialog);
                break;
            }
            default: {
                MessageManager::callAsync([popup]() {
                    delete popup;
                });
                break;
            }
            }
        });
}

class OkayCancelDialog : public Component {

public:
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

void Dialogs::showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position)
{
    cnv->cancelConnectionCreation();

    // Info about selection status
    auto selectedBoxes = cnv->getSelectionOfType<Object>();

    bool hasSelection = !selectedBoxes.isEmpty();
    bool multiple = selectedBoxes.size() > 1;

    Object* object = hasSelection && !multiple ? selectedBoxes.getFirst() : nullptr;

    // Find top-level object, so we never trigger it on an object inside a graph
    if (object && object->findParentComponentOfClass<Object>()) {
        while (auto* nextObject = object->findParentComponentOfClass<Object>()) {
            object = nextObject;
        }
    }

    auto params = object && object->gui ? object->gui->getParameters() : ObjectParameters();
    bool canBeOpened = object && object->gui && object->gui->canOpenFromMenu();

    // Create popup menu
    PopupMenu popupMenu;

    popupMenu.addItem(1, "Open", object && !multiple && canBeOpened); // for opening subpatches
    popupMenu.addSeparator();

    auto* editor = cnv->editor;
    std::function<void(int)> createObjectCallback;
    popupMenu.addSubMenu("Add", createObjectMenu(editor));

    popupMenu.addSeparator();
    popupMenu.addCommandItem(editor, CommandIDs::Cut);
    popupMenu.addCommandItem(editor, CommandIDs::Copy);
    popupMenu.addCommandItem(editor, CommandIDs::Paste);
    popupMenu.addCommandItem(editor, CommandIDs::Duplicate);
    popupMenu.addCommandItem(editor, CommandIDs::Encapsulate);
    popupMenu.addCommandItem(editor, CommandIDs::Delete);
    popupMenu.addSeparator();

    popupMenu.addItem(8, "To Front", object != nullptr);
    popupMenu.addItem(9, "To Back", object != nullptr);
    popupMenu.addSeparator();
    popupMenu.addItem(10, "Help", object != nullptr);
    popupMenu.addItem(11, "Reference", object != nullptr);
    popupMenu.addSeparator();
    popupMenu.addItem(12, "Properties", originalComponent == cnv || (object && !params.empty()));
    // showObjectReferenceDialog
    auto callback = [cnv, editor, object, originalComponent, params, createObjectCallback, position, selectedBoxes](int result) mutable {
        // Set position where new objet will be created
        if (result > 100) {
            cnv->lastMousePosition = cnv->getLocalPoint(nullptr, position);
        }

        if ((!object && result < 100) || result < 1)
            return;

        if (object)
            object->repaint();

        switch (result) {
        case 1: // Open subpatch
            object->gui->openFromMenu();
            break;
        case 8: { // To Front
            // The double for loop makes sure that they keep their original order
            auto objects = cnv->patch.getObjects();

            cnv->patch.startUndoSequence("ToFront");
            for (int i = objects.size() - 1; i >= 0; i--) {
                for (auto* selectedBox : selectedBoxes) {
                    if (objects[i] == selectedBox->getPointer()) {
                        selectedBox->toFront(false);
                        if (selectedBox->gui)
                            selectedBox->gui->moveToFront();
                    }
                }
            }
            cnv->patch.startUndoSequence("ToBack");
            cnv->synchronise();
            break;
        }
        case 9: { // To Back
            auto objects = cnv->patch.getObjects();

            cnv->patch.startUndoSequence("ToBack");
            // The double for loop makes sure that they keep their original order
            for (int i = objects.size() - 1; i >= 0; i--) {
                for (auto* selectedBox : selectedBoxes) {
                    if (objects[i] == selectedBox->getPointer()) {
                        selectedBox->toBack();
                        if (selectedBox->gui)
                            selectedBox->gui->moveToBack();
                    }
                }
            }
            cnv->patch.endUndoSequence("ToBack");
            cnv->synchronise();
            break;
        }
        case 10: // Open help
            object->openHelpPatch();
            break;
        case 11: // Open reference
            Dialogs::showObjectReferenceDialog(&editor->openedDialog, editor, object->gui->getType());
            break;
        case 12:
            if (originalComponent == cnv) {
                // Open help
                editor->sidebar.showParameters("canvas", cnv->getInspectorParameters());
            } else {
                editor->sidebar.showParameters(object->gui->getText(), params);
            }

            break;
        default:
            break;
        }
    };

    popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(editor).withTargetScreenArea(Rectangle<int>(position, position.translated(1, 1))), ModalCallbackFunction::create(callback));
}

void Dialogs::showObjectMenu(PluginEditor* parent, Component* target)
{
    std::function<void(int)> attachToMouseCallback = [parent](int result) {
        if (result > 0) {
            if (auto* cnv = parent->getCurrentCanvas()) {
                cnv->attachNextObjectToMouse = true;
            }
        }
    };

    auto menu = createObjectMenu(parent);

    menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(target).withParentComponent(parent), attachToMouseCallback);
}

PopupMenu Dialogs::createObjectMenu(PluginEditor* parent)
{
    PopupMenu menu;

    // Custom function because JUCE adds "shortcut:" before some keycommands, which looks terrible!
    auto createCommandItem = [parent](const CommandID commandID, String const& displayName) {
        if (commandID < NumEssentialObjects) {

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
        } else {

            auto cnv = Component::SafePointer(parent->getCurrentCanvas());
            bool locked = static_cast<bool>(cnv->locked.getValue()) || static_cast<bool>(cnv->commandLocked.getValue());

            PopupMenu::Item i;
            i.text = displayName;
            i.itemID = (int)commandID;
            i.isEnabled = !locked;
            i.action = [parent, cnv, commandID]() {
                if (!cnv)
                    return;

                auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->lastMousePosition - Point<int>(Object::margin, Object::margin));

                cnv->objects.add(new Object(cnv, objectNames.at(static_cast<ObjectIDs>(commandID)), lastPosition));
                cnv->deselectAll();
                cnv->setSelected(cnv->objects[cnv->objects.size() - 1], true); // Select newly created object
            };

            return i;
        }
    };

    menu.addItem("Open Object Browser...", [parent]() mutable {
        Dialogs::showObjectBrowserDialog(&parent->openedDialog, parent);
    });

    PopupMenu uiMenu;
    {
        uiMenu.addItem(createCommandItem(ObjectIDs::NewNumbox, "Number"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewBang, "Bang"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewToggle, "Toggle"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewButton, "Button"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewVerticalSlider, "Vertical Slider"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewHorizontalSlider, "Horizontal Slider"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewVerticalRadio, "Vertical Radio"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewHorizontalRadio, "Horizontal Radio"));

        uiMenu.addSeparator();
        uiMenu.addItem(createCommandItem(ObjectIDs::NewCanvas, "Canvas"));

        uiMenu.addItem(createCommandItem(ObjectIDs::NewKeyboard, "Keyboard"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewVUMeterObject, "VU Meter"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewNumboxTilde, "Signal Numbox"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewOscilloscope, "Oscilloscope"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewFunction, "Function"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewMessbox, "Messbox"));
        uiMenu.addItem(createCommandItem(ObjectIDs::NewBicoeff, "Bicoeff"));
    }

    PopupMenu generalMenu;
    {
        generalMenu.addItem(createCommandItem(ObjectIDs::NewMetro, "metro"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewCounter, "counter"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewSel, "sel"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewRoute, "route"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewExpr, "expr"));

        generalMenu.addItem(createCommandItem(ObjectIDs::NewLoadbang, "loadbang"));

        generalMenu.addItem(createCommandItem(ObjectIDs::NewPack, "pack"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewUnpack, "unpack"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewPrint, "print"));

        generalMenu.addItem(createCommandItem(ObjectIDs::NewNetsend, "netsend"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewNetreceive, "netreceive"));

        generalMenu.addItem(createCommandItem(ObjectIDs::NewTimer, "timer"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewDelay, "delay"));
        generalMenu.addItem(createCommandItem(ObjectIDs::NewTimedGate, "timed.gate"));
    }

    PopupMenu effectsMenu;
    {
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewCrusher, "crusher~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewSignalDelay, "delay~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewDrive, "drive~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewFlanger, "flanger~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewReverb, "free.rev~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewFreeze, "freeze~"));

        effectsMenu.addItem(createCommandItem(ObjectIDs::NewFreqShift, "freq.shift~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewPhaser, "phaser~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewShaper, "shaper~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewRm, "rm~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewTremolo, "tremolo~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewVibrato, "vibrato~"));
        effectsMenu.addItem(createCommandItem(ObjectIDs::NewVocoder, "vocoder~"));
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
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewPlaits, "plaits~"));
        oscillatorsMenu.addSeparator();
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlOsc, "bl.osc~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSaw, "bl.saw~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSaw2, "bl.saw2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlSquare, "bl.square~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlTriangle, "bl.tri~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlImp, "bl.imp~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlImp2, "bl.imp2~"));
        oscillatorsMenu.addItem(createCommandItem(ObjectIDs::NewBlWavetable, "bl.wavetable~"));
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

    PopupMenu arrayMenu;
    {
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArraySet, "array set"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayGet, "array get"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayDefine, "array define"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArraySize, "array size"));

        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayMin, "array min"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayMax, "array max"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayRandom, "array random"));
        arrayMenu.addItem(createCommandItem(ObjectIDs::NewArrayQuantile, "array quantile"));
    }

    PopupMenu listMenu;
    {
        listMenu.addItem(createCommandItem(ObjectIDs::NewListAppend, "list append"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListPrepend, "list prepend"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListStore, "list store"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListSplit, "list split"));

        listMenu.addItem(createCommandItem(ObjectIDs::NewListTrim, "list trim"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListLength, "list length"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListFromSymbol, "list fromsymbol"));
        listMenu.addItem(createCommandItem(ObjectIDs::NewListToSymbol, "list tosymbol"));
    }

    PopupMenu mathMenu;
    {
        mathMenu.addItem(createCommandItem(ObjectIDs::NewAdd, "+"));
        mathMenu.addItem(createCommandItem(ObjectIDs::NewSubtract, "-"));
        mathMenu.addItem(createCommandItem(ObjectIDs::NewMultiply, "*"));
        mathMenu.addItem(createCommandItem(ObjectIDs::NewDivide, "/"));
        mathMenu.addItem(createCommandItem(ObjectIDs::NewModulo, "%"));

        mathMenu.addItem(createCommandItem(ObjectIDs::NewInverseSubtract, "!-"));
        mathMenu.addItem(createCommandItem(ObjectIDs::NewInverseDivide, "!/"));
    }

    PopupMenu logicMenu;
    {
        logicMenu.addItem(createCommandItem(ObjectIDs::NewBiggerThan, ">"));
        logicMenu.addItem(createCommandItem(ObjectIDs::NewSmallerThan, "<"));
        logicMenu.addItem(createCommandItem(ObjectIDs::NewBiggerThanOrEqual, ">="));
        logicMenu.addItem(createCommandItem(ObjectIDs::NewSmallerThanOrEqual, "<="));
        logicMenu.addItem(createCommandItem(ObjectIDs::NewEquals, "=="));
        logicMenu.addItem(createCommandItem(ObjectIDs::NewNotEquals, "!="));
    }

    PopupMenu ioMenu;
    {
        ioMenu.addItem(createCommandItem(ObjectIDs::NewAdc, "adc~"));
        ioMenu.addItem(createCommandItem(ObjectIDs::NewDac, "dac~"));
        ioMenu.addItem(createCommandItem(ObjectIDs::NewOut, "out~"));
        ioMenu.addItem(createCommandItem(ObjectIDs::NewBlocksize, "blocksize~"));
        ioMenu.addItem(createCommandItem(ObjectIDs::NewSamplerate, "samplerate~"));
        ioMenu.addItem(createCommandItem(ObjectIDs::NewSetdsp, "setdsp~"));
    }

    PopupMenu controlMenu;
    {
        controlMenu.addItem(createCommandItem(ObjectIDs::NewAdsr, "adsr~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewAsr, "asr~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewCurve, "curve~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewDecay, "decay~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewEnvelope, "envelope~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewEnvgen, "envgen~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewLfnoise, "lfnoise~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewSignalLine, "line~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewPhasor, "phasor~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewRamp, "ramp~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewSah, "sah~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewSignalSlider, "slider~"));
        controlMenu.addItem(createCommandItem(ObjectIDs::NewVline, "vline~"));
    }

    PopupMenu signalMathMenu;
    {
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalAdd, "+~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalSubtract, "-~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalMultiply, "*~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalDivide, "/~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalModulo, "%~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalInverseSubtract, "!-~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalInverseDivide, "!/~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalBiggerThan, ">~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalSmallerThan, "<~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalBiggerThanOrEqual, ">=~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalSmallerThanOrEqual, "<=~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalEquals, "==~"));
        signalMathMenu.addItem(createCommandItem(ObjectIDs::NewSignalNotEquals, "!=~"));
    }

    menu.addSeparator();

    menu.addSubMenu("UI", uiMenu);
    menu.addSubMenu("General", generalMenu);
    menu.addSubMenu("MIDI", midiMenu);
    menu.addSubMenu("Array", arrayMenu);
    menu.addSubMenu("List", listMenu);
    menu.addSubMenu("Math", mathMenu);
    menu.addSubMenu("Logic", logicMenu);

    menu.addSeparator();

    menu.addSubMenu("IO~", ioMenu);
    menu.addSubMenu("Effects~", effectsMenu);
    menu.addSubMenu("Oscillators~", oscillatorsMenu);
    menu.addSubMenu("Filters~", filtersMenu);
    menu.addSubMenu("Control~", controlMenu);
    menu.addSubMenu("Math~", signalMathMenu);

    menu.addSeparator();

    menu.addItem(createCommandItem(ObjectIDs::NewObject, "Empty Object"));
    menu.addItem(createCommandItem(ObjectIDs::NewMessage, "Message"));
    menu.addItem(createCommandItem(ObjectIDs::NewFloatAtom, "Float box"));
    menu.addItem(createCommandItem(ObjectIDs::NewSymbolAtom, "Symbol box"));
    menu.addItem(createCommandItem(ObjectIDs::NewListAtom, "List box"));
    menu.addItem(createCommandItem(ObjectIDs::NewComment, "Comment"));
    menu.addSeparator();

    menu.addItem(createCommandItem(ObjectIDs::NewArray, "Array..."));
    menu.addItem(createCommandItem(ObjectIDs::NewGraphOnParent, "GraphOnParent"));
    return menu;
}
