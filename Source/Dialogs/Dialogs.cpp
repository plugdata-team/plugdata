/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Dialogs.h"

#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/ArrowPopupMenu.h"

#include "Sidebar/Sidebar.h"
#include "Object.h"
#include "Objects/ObjectBase.h"
#include "SaveDialog.h"
#include "ArrayDialog.h"
#include "SettingsDialog.h"
#include "TextEditorDialog.h"
#include "ObjectBrowserDialog.h"
#include "ObjectReferenceDialog.h"
#include "Heavy/HeavyExportDialog.h"
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

    centre->getTopLevelComponent()->toFront(true);
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

    ArrowPopupMenu::showMenuAsync(popup, PopupMenu::Options().withMinimumWidth(220).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
        [editor, popup, centre, settingsTree = SettingsFile::getInstance()->getValueTree()](int result) mutable {
            switch (result) {
            case MainMenu::MenuItem::NewPatch: {
                editor->newProject();
                break;
            }
            case MainMenu::MenuItem::OpenPatch: {
                editor->openProject();
                break;
            }
            case MainMenu::MenuItem::Save: {
                if (editor->getCurrentCanvas())
                    editor->saveProject();
                break;
            }
            case MainMenu::MenuItem::SaveAs: {
                if (editor->getCurrentCanvas())
                    editor->saveProjectAs();
                break;
            }
            case MainMenu::MenuItem::Close: {
                if (auto* canvas = editor->getCurrentCanvas()) {
                    MessageManager::callAsync([editor, cnv = Component::SafePointer(canvas)]() mutable {
                        if (cnv && cnv->patch.isDirty()) {
                            Dialogs::showSaveDialog(&editor->openedDialog, editor, cnv->patch.getTitle(),
                                [cnv, editor](int result) mutable {
                                    if (!cnv)
                                        return;
                                    if (result == 2)
                                        editor->saveProject([editor, cnv]() mutable { editor->closeTab(cnv); });
                                    else if (result == 1)
                                        editor->closeTab(cnv);
                                });
                        } else {
                            editor->closeTab(cnv);
                        }
                    });
                }
                break;
            }
            case MainMenu::MenuItem::CloseAll: {
                if (editor->getCurrentCanvas())
                    editor->closeAllTabs();
                break;
            }
            case MainMenu::MenuItem::CompiledMode: {
                bool ticked = settingsTree.hasProperty("hvcc_mode") ? static_cast<bool>(settingsTree.getProperty("hvcc_mode")) : false;
                settingsTree.setProperty("hvcc_mode", !ticked, nullptr);
                break;
            }
            case MainMenu::MenuItem::Compile: {
                Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
                break;
            }
            case MainMenu::MenuItem::EnablePalettes: {
                bool ticked = settingsTree.hasProperty("show_palettes") ? static_cast<bool>(settingsTree.getProperty("show_palettes")) : false;
                settingsTree.setProperty("show_palettes", !ticked, nullptr);
                editor->resized();
                break;
            }
            case MainMenu::MenuItem::PluginMode: {
                if (auto* cnv = editor->getCurrentCanvas()) {
                    editor->enablePluginMode(cnv);
                }
                break;
            }
            case MainMenu::MenuItem::AutoConnect: {
                bool ticked = settingsTree.hasProperty("autoconnect") ? static_cast<bool>(settingsTree.getProperty("autoconnect")) : false;
                settingsTree.setProperty("autoconnect", !ticked, nullptr);
                break;
            }
            case MainMenu::MenuItem::Settings: {
                Dialogs::showSettingsDialog(editor);
                break;
            }
            case MainMenu::MenuItem::About: {
                auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, editor->getBounds().getCentreY() + 250, true);
                auto* aboutPanel = new AboutPanel();
                dialog->setViewedComponent(aboutPanel);
                editor->openedDialog.reset(dialog);
                break;
            }
            default: {
                break;
            }
            }

            MessageManager::callAsync([popup]() {
                delete popup;
            });
        });
}

void Dialogs::showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> callback)
{

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

void Dialogs::askToLocatePatch(PluginEditor* editor, String const& backupState, std::function<void(File)> callback)
{
    class LocatePatchDialog : public Component {

    public:
        LocatePatchDialog(Dialog* dialog, String const& backup, std::function<void(File)> callback)
            : label("", "")
            , backupState(backup)
        {
            setSize(400, 200);
            addAndMakeVisible(label);
            addAndMakeVisible(locate);
            addAndMakeVisible(loadFromState);

            locate.setColour(TextButton::buttonColourId, Colours::transparentBlack);
            loadFromState.setColour(TextButton::buttonColourId, Colours::transparentBlack);

            locate.onClick = [this, dialog, callback] {
                callback(File());

                openChooser = std::make_unique<FileChooser>("Choose file to open", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pd", SettingsFile::getInstance()->wantsNativeDialog());

                openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [callback](FileChooser const& f) {
                    File openedFile = f.getResult();
                    if (openedFile.existsAsFile()) {
                        callback(openedFile);
                    }
                });

                dialog->closeDialog();
            };

            loadFromState.onClick = [this, dialog, callback] {
                if (backupState.isEmpty())
                    backupState = pd::Instance::defaultPatch;

                auto patchFile = File::createTempFile(".pd");
                patchFile.replaceWithText(backupState);

                callback(patchFile);
                dialog->closeDialog();
            };

            locate.changeWidthToFitText();
            loadFromState.changeWidthToFitText();
            setOpaque(false);
        }

        void resized() override
        {
            label.setBounds(20, 25, 360, 30);
            loadFromState.setBounds(20, 80, 80, 25);
            locate.setBounds(300, 80, 80, 25);
        }

    private:
        Label label;
        String backupState;

        std::unique_ptr<FileChooser> openChooser;

        TextButton loadFromState = TextButton("Use saved state");
        TextButton locate = TextButton("Locate...");
    };

    auto* dialog = new Dialog(&editor->openedDialog, editor, 400, 130, 160, false);
    auto* dialogContent = new LocatePatchDialog(dialog, backupState, callback);

    dialog->setViewedComponent(dialogContent);
    editor->openedDialog.reset(dialog);
}

void Dialogs::showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position)
{
    cnv->cancelConnectionCreation();
    cnv->isShowingMenu = true;

    // Info about selection status
    auto selectedBoxes = cnv->getSelectionOfType<Object>();

    bool hasSelection = !selectedBoxes.isEmpty();
    bool multiple = selectedBoxes.size() > 1;

    Object* object = hasSelection ? selectedBoxes.getFirst() : nullptr;

    // Find top-level object, so we never trigger it on an object inside a graph
    if (object && object->findParentComponentOfClass<Object>()) {
        while (auto* nextObject = object->findParentComponentOfClass<Object>()) {
            object = nextObject;
        }
    }

    auto params = object && object->gui ? object->gui->getParameters() : ObjectParameters();
    bool canBeOpened = object && object->gui && object->gui->canOpenFromMenu();

    enum MenuOptions {
        Open = 1,
        Help,
        Reference,
        ToFront,
        ToBack,
        Properties
    };
    // Create popup menu
    PopupMenu popupMenu;

    popupMenu.addItem(Open, "Open", object && !multiple && canBeOpened); // for opening subpatches

    popupMenu.addSeparator();
    popupMenu.addItem(Help, "Help", object != nullptr);
    popupMenu.addItem(Reference, "Reference", object != nullptr);
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

    popupMenu.addItem(ToFront, "To Front", object != nullptr);
    popupMenu.addItem(ToBack, "To Back", object != nullptr);
    popupMenu.addSeparator();
    popupMenu.addItem(Properties, "Properties", originalComponent == cnv || (object && !params.empty()));
    // showObjectReferenceDialog
    auto callback = [cnv, editor, object, originalComponent, params, createObjectCallback, position, selectedBoxes](int result) mutable {
        cnv->isShowingMenu = false;
        cnv->grabKeyboardFocus();

        // Make sure that iolets don't hang in hovered state
        for (auto* object : cnv->objects) {
            for (auto* iolet : object->iolets)
                reinterpret_cast<Component*>(iolet)->repaint();
        }

        // Set position where new objet will be created
        if (result > 100) {
            cnv->lastMousePosition = cnv->getLocalPoint(nullptr, position);
        }

        if (result == Properties) {
            if (originalComponent == cnv) {
                editor->sidebar->showParameters("canvas", cnv->getInspectorParameters());
            } else if (object && object->gui) {
                editor->sidebar->showParameters(object->gui->getText(), params);
            }

            return;
        }

        if ((!object && result < 100) || result <= 0) {
            return;
        }

        if (object)
            object->repaint();

        switch (result) {
        case Open: // Open subpatch
            object->gui->openFromMenu();
            break;
        case ToFront: { // To Front
            auto objects = cnv->patch.getObjects();

            // The FORWARD double for loop makes sure that they keep their original order
            cnv->patch.startUndoSequence("ToFront");
            for (int i = 0; i < objects.size(); i++) {
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
        case ToBack: {
            auto objects = cnv->patch.getObjects();

            cnv->patch.startUndoSequence("ToBack");
            // The REVERSE double for loop makes sure that they keep their original order
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
        case Help:
            object->openHelpPatch();
            break;
        case Reference:
            Dialogs::showObjectReferenceDialog(&editor->openedDialog, editor, object->gui->getType());
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

    ArrowPopupMenu::showMenuAsync(&menu, PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(target).withParentComponent(parent), attachToMouseCallback);
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
        } else if (auto cnv = Component::SafePointer(parent->getCurrentCanvas())) {

            bool locked = getValue<bool>(cnv->locked) || getValue<bool>(cnv->commandLocked);

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

        return PopupMenu::Item();
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

    menu.addItem(createCommandItem(ObjectIDs::NewObject, "Empty Object"));
    menu.addItem(createCommandItem(ObjectIDs::NewMessage, "Message"));
    menu.addItem(createCommandItem(ObjectIDs::NewFloatAtom, "Float box"));
    menu.addItem(createCommandItem(ObjectIDs::NewSymbolAtom, "Symbol box"));
    menu.addItem(createCommandItem(ObjectIDs::NewListAtom, "List box"));
    menu.addItem(createCommandItem(ObjectIDs::NewComment, "Comment"));
    menu.addSeparator();

    menu.addItem(createCommandItem(ObjectIDs::NewArray, "Array..."));
    menu.addItem(createCommandItem(ObjectIDs::NewGraphOnParent, "GraphOnParent"));

    menu.addSeparator();

    bool active = false;
    if (auto* cnv = parent->getCurrentCanvas()) {
        active = !getValue<bool>(cnv->locked);
    }

    menu.addSubMenu("UI", uiMenu, active);
    menu.addSubMenu("General", generalMenu, active);
    menu.addSubMenu("MIDI", midiMenu, active);
    menu.addSubMenu("Array", arrayMenu, active);
    menu.addSubMenu("List", listMenu, active);
    menu.addSubMenu("Math", mathMenu, active);
    menu.addSubMenu("Logic", logicMenu, active);

    menu.addSeparator();

    menu.addSubMenu("IO~", ioMenu, active);
    menu.addSubMenu("Effects~", effectsMenu, active);
    menu.addSubMenu("Oscillators~", oscillatorsMenu, active);
    menu.addSubMenu("Filters~", filtersMenu, active);
    menu.addSubMenu("Control~", controlMenu, active);
    menu.addSubMenu("Math~", signalMathMenu, active);

    return menu;
}
