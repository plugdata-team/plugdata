/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <utility>
#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include "Dialogs.h"

#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Components/ArrowPopupMenu.h"
#include "Components/Buttons.h"

#include "Sidebar/Sidebar.h"
#include "Object.h"
#include "Objects/ObjectBase.h"
#include "SaveDialog.h"
#include "SettingsDialog.h"
#include "TextEditorDialog.h"
#include "ObjectBrowserDialog.h"
#include "ObjectReferenceDialog.h"
#include "Heavy/HeavyExportDialog.h"
#include "MainMenu.h"
#include "AddObjectMenu.h"
#include "Canvas.h"
#include "Connection.h"
#include "Deken.h"
#include "PatchStorage.h"

Component* Dialogs::showTextEditorDialog(String const& text, String filename, std::function<void(String, bool)> callback)
{
    auto* editor = new TextEditorDialog(std::move(filename));
    editor->editor.setText(text);
    editor->onClose = std::move(callback);
    return editor;
}

void Dialogs::appendTextToTextEditorDialog(Component* dialog, String const& text)
{
    if (!dialog)
        return;

    auto& editor = dynamic_cast<TextEditorDialog*>(dialog)->editor;
    editor.setText(editor.getText() + text);
}

void Dialogs::showSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String const& filename, std::function<void(int)> callback, int margin, bool withLogo)
{
    if (*target)
        return;

    auto* dialog = new Dialog(target, centre, 265, withLogo ? 270 : 210, false, margin);
    auto* saveDialog = new SaveDialog(dialog, filename, std::move(callback), withLogo);

    dialog->setViewedComponent(saveDialog);
    target->reset(dialog);

    centre->getTopLevelComponent()->toFront(true);
}


void Dialogs::showSettingsDialog(PluginEditor* editor)
{
    auto* dialog = new Dialog(&editor->openedDialog, editor, 690, 500, true);
    auto* settingsDialog = new SettingsDialog(editor);
    dialog->setViewedComponent(settingsDialog);
    editor->openedDialog.reset(dialog);
}

void Dialogs::showMainMenu(PluginEditor* editor, Component* centre)
{
    auto* popup = new MainMenu(editor);

    ArrowPopupMenu::showMenuAsync(popup, PopupMenu::Options().withMinimumWidth(210).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(editor),
        [editor, popup, settingsTree = SettingsFile::getInstance()->getValueTree()](int result) mutable {
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
            case MainMenu::MenuItem::CompiledMode: {
                bool ticked = settingsTree.hasProperty("hvcc_mode") && static_cast<bool>(settingsTree.getProperty("hvcc_mode"));
                settingsTree.setProperty("hvcc_mode", !ticked, nullptr);
                break;
            }
            case MainMenu::MenuItem::Compile: {
                Dialogs::showHeavyExportDialog(&editor->openedDialog, editor);
                break;
            }
            case MainMenu::MenuItem::FindExternals: {
                Dialogs::showDeken(editor);
                break;
            }
                /*
        case MainMenu::MenuItem::Discover: {
            Dialogs::showPatchStorage(editor);
            break;
        } */
            case MainMenu::MenuItem::Settings: {
                Dialogs::showSettingsDialog(editor);
                break;
            }
            case MainMenu::MenuItem::About: {
                auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, true);
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

void Dialogs::showOkayCancelDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(bool)> const& callback)
{

    class OkayCancelDialog : public Component {

    public:
        OkayCancelDialog(Dialog* dialog, String const& title, std::function<void(bool)> const& callback)
            : label("", title)
        {
            setSize(400, 200);
            addAndMakeVisible(label);
            addAndMakeVisible(cancel);
            addAndMakeVisible(okay);

            cancel.setColour(TextButton::buttonColourId, Colours::transparentBlack);
            okay.setColour(TextButton::buttonColourId, Colours::transparentBlack);
            
            cancel.onClick = [dialog, callback] {
                callback(false);
                dialog->closeDialog();
            };

            okay.onClick = [dialog, callback] {
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

    auto* dialog = new Dialog(target, parent, 400, 130, false);
    auto* dialogContent = new OkayCancelDialog(dialog, title, callback);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showHeavyExportDialog(std::unique_ptr<Dialog>* target, Component* parent)
{
    auto* dialog = new Dialog(target, parent, 625, 400, true);
    auto* dialogContent = new HeavyExportDialog(dialog);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showObjectBrowserDialog(std::unique_ptr<Dialog>* target, Component* parent)
{

    auto* dialog = new Dialog(target, parent, 750, 480, true);
    auto* dialogContent = new ObjectBrowserDialog(parent, dialog);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showObjectReferenceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& objectName)
{
    auto* dialog = new Dialog(target, parent, 750, 480, true);
    auto* dialogContent = new ObjectReferenceDialog(dynamic_cast<PluginEditor*>(parent), false);

    dialogContent->showObject(objectName);

    dialog->setViewedComponent(dialogContent);
    target->reset(dialog);
}

void Dialogs::showDeken(PluginEditor* editor)
{
    auto* dialog = new Dialog(&editor->openedDialog, editor, 675, 500, true);
    auto* dialogContent = new Deken();
    dialog->setViewedComponent(dialogContent);
    editor->openedDialog.reset(dialog);
}

void Dialogs::showPatchStorage(PluginEditor* editor)
{
    auto* dialog = new Dialog(&editor->openedDialog, editor, 800, 550, true);
    auto* dialogContent = new PatchStorage();
    dialog->setViewedComponent(dialogContent);
    editor->openedDialog.reset(dialog);
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

bool Dialog::wantsRoundedCorners() const
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
        LocatePatchDialog(Dialog* dialog, String backup, std::function<void(File)> callback)
            : label("", "")
            , backupState(std::move(backup))
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

    auto* dialog = new Dialog(&editor->openedDialog, editor, 400, 130, false);
    auto* dialogContent = new LocatePatchDialog(dialog, backupState, std::move(callback));

    dialog->setViewedComponent(dialogContent);
    editor->openedDialog.reset(dialog);
}

void Dialogs::showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> position)
{
    struct QuickActionsBar : public PopupMenu::CustomComponent {
        struct QuickActionButton : public TextButton {
            QuickActionButton(String buttonText)
                : TextButton(buttonText)
            {
            }

            void paint(Graphics& g) override
            {
                auto textColour = findColour(PlugDataColour::sidebarTextColourId);

                if (!isEnabled()) {
                    textColour = textColour.withAlpha(0.35f);
                } else if (isOver() || isDown()) {
                    auto bounds = getLocalBounds().toFloat();
                    bounds = bounds.withSizeKeepingCentre(bounds.getHeight(), bounds.getHeight());

                    g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                    PlugDataLook::fillSmoothedRectangle(g, bounds, Corners::defaultCornerRadius);

                    textColour = findColour(PlugDataColour::sidebarActiveTextColourId);
                }

                Fonts::drawIcon(g, getButtonText(), std::max(0, getWidth() - getHeight()) / 2, 0, getHeight(), textColour, 12.8f);
            }
        };

        CheckedTooltip tooltipWindow;

        QuickActionsBar(ApplicationCommandManager* commandManager)
            : tooltipWindow(this)
        {
            auto commandIds = Array<CommandID> { CommandIDs::Cut, CommandIDs::Copy, CommandIDs::Paste, CommandIDs::Duplicate, CommandIDs::Delete };

            for (auto* button : Array<QuickActionButton*> { &cut, &copy, &paste, &duplicate, &remove }) {
                addAndMakeVisible(button);
                auto id = commandIds.removeAndReturn(0);

                button->onClick = [commandManager, id]() {
                    if (auto* editor = dynamic_cast<PluginEditor*>(commandManager)) {
                        editor->grabKeyboardFocus();
                    }

                    ApplicationCommandTarget::InvocationInfo info(id);
                    info.invocationMethod = ApplicationCommandTarget::InvocationInfo::fromMenu;
                    commandManager->invoke(info, true);
                };

                if (auto* registeredInfo = commandManager->getCommandForID(id)) {
                    ApplicationCommandInfo info(*registeredInfo);
                    commandManager->getTargetForCommand(id, info);
                    bool canPerformCommand = (info.flags & ApplicationCommandInfo::isDisabled) == 0;
                    button->setEnabled(canPerformCommand);
                } else {
                    button->setEnabled(false);
                }
            }

            cut.setTooltip("Cut");
            copy.setTooltip("Copy");
            paste.setTooltip("Paste");
            duplicate.setTooltip("Duplicate");
            remove.setTooltip("Delete");
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 130;
            idealHeight = 26;
        }

        void resized() override
        {
            auto buttonHeight = 26;
            auto buttonWidth = getWidth() / 5;
            auto bounds = getLocalBounds();

            for (auto* button : Array<TextButton*> { &cut, &copy, &paste, &duplicate, &remove }) {
                button->setBounds(bounds.removeFromLeft(buttonWidth).withHeight(buttonHeight));
            }
        }

        QuickActionButton cut = QuickActionButton(Icons::Cut);
        QuickActionButton copy = QuickActionButton(Icons::Copy);
        QuickActionButton paste = QuickActionButton(Icons::Paste);
        QuickActionButton duplicate = QuickActionButton(Icons::Duplicate);
        QuickActionButton remove = QuickActionButton(Icons::Trash);
    };

    // We have a custom function for this, instead of the default JUCE way, because the default JUCE way is broken on Linux
    // It will not find a target to apply the command to once the popupmenu grabs focus...
    auto addCommandItem = [commandManager = cnv->editor](PopupMenu& menu, const CommandID commandID, String displayName = "") {
        if (auto* registeredInfo = commandManager->getCommandForID(commandID)) {
            ApplicationCommandInfo info(*registeredInfo);
            commandManager->getCommandInfo(commandID, info);

            PopupMenu::Item i;
            i.text = displayName.isNotEmpty() ? std::move(displayName) : info.shortName;
            i.itemID = (int)commandID;
            i.commandManager = commandManager;
            i.isEnabled = (info.flags & ApplicationCommandInfo::isDisabled) == 0;
            i.isTicked = (info.flags & ApplicationCommandInfo::isTicked) != 0;
            menu.addItem(std::move(i));
        }
    };

    cnv->cancelConnectionCreation();

    // Info about selection status
    auto selectedBoxes = cnv->getSelectionOfType<Object>();

    // If we directly right-clicked on an object, make sure it has been added to selection
    if (auto* obj = dynamic_cast<Object*>(originalComponent)) {
        selectedBoxes.addIfNotAlreadyThere(obj);
    } else if (auto* obj = originalComponent->findParentComponentOfClass<Object>()) {
        selectedBoxes.addIfNotAlreadyThere(obj);
    }

    bool hasSelection = !selectedBoxes.isEmpty();
    bool multiple = selectedBoxes.size() > 1;
    bool locked = getValue<bool>(cnv->locked);

    auto object = Component::SafePointer<Object>(hasSelection ? selectedBoxes.getFirst() : nullptr);

    // Find top-level object, so we never trigger it on an object inside a graph
    if (object && object->findParentComponentOfClass<Object>()) {
        while (auto* nextObject = object->findParentComponentOfClass<Object>()) {
            object = nextObject;
        }
    }

    auto* editor = cnv->editor;
    auto params = object && object->gui ? object->gui->getParameters() : ObjectParameters();
    bool canBeOpened = object && object->gui && object->gui->canOpenFromMenu();

    enum MenuOptions {
        Extra = 1,
        Open,
        Help,
        Reference,
        ToFront,
        Forward,
        Backward,
        ToBack,
        Properties
    };
    // Create popup menu
    PopupMenu popupMenu;

    popupMenu.addCustomItem(Extra, std::make_unique<QuickActionsBar>(editor), nullptr, "Quick Actions");
    popupMenu.addSeparator();

    popupMenu.addItem(Open, "Open", object && !multiple && canBeOpened); // for opening subpatches

    popupMenu.addSeparator();
    popupMenu.addItem(Help, "Help", object != nullptr);
    popupMenu.addItem(Reference, "Reference", object != nullptr);
    popupMenu.addSeparator();

    bool selectedConnection = false, noneSegmented = true;
    for (auto& connection : cnv->getSelectionOfType<Connection>()) {
        noneSegmented = noneSegmented && !connection->isSegmented();
        selectedConnection = true;
    }

    popupMenu.addItem("Curved Connection", selectedConnection, selectedConnection && !noneSegmented, [editor, cnv, noneSegmented]() {
        bool segmented = noneSegmented;
        auto* cnv = editor->getCurrentCanvas();

        // cnv->patch.startUndoSequence("ChangeSegmentedPaths");

        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            connection->setSegmented(segmented);
        }

        // cnv->patch.endUndoSequence("ChangeSegmentedPaths");
    });
    addCommandItem(popupMenu, CommandIDs::ConnectionPathfind);

    popupMenu.addSeparator();
    addCommandItem(popupMenu, CommandIDs::Encapsulate);
    popupMenu.addSeparator();

    PopupMenu orderMenu;
    orderMenu.addItem(ToFront, "To Front", object != nullptr && !locked);
    orderMenu.addItem(Forward, "Move forward", object != nullptr && !locked);
    orderMenu.addItem(Backward, "Move backward", object != nullptr && !locked);
    orderMenu.addItem(ToBack, "To Back", object != nullptr && !locked);
    popupMenu.addSubMenu("Order", orderMenu, !locked);

    popupMenu.addSeparator();
    popupMenu.addItem(Properties, "Properties", (originalComponent == cnv || (object && !params.getParameters().isEmpty())) && !locked);
    // showObjectReferenceDialog
    auto callback = [cnv, editor, object, originalComponent, params, position, selectedBoxes](int result) mutable {
        cnv->grabKeyboardFocus();

        // Make sure that iolets don't hang in hovered state
        for (auto* object : cnv->objects) {
            for (auto* iolet : object->iolets)
                reinterpret_cast<Component*>(iolet)->repaint();
        }

        if (result == Properties) {
            if (originalComponent == cnv) {
                Array<ObjectParameters> parameters = {cnv->getInspectorParameters()};
                editor->sidebar->showParameters("canvas", parameters);
            } else if (object && object->gui) {

                cnv->pd->lockAudioThread();
                // this makes sure that objects can handle the "properties" message as well if they like, for example for [else/properties]
                auto* pdClass = pd_class(&static_cast<t_gobj*>(object->getPointer())->g_pd);
                auto propertiesFn = class_getpropertiesfn(pdClass);

                if (propertiesFn)
                    propertiesFn(static_cast<t_gobj*>(object->getPointer()), cnv->patch.getPointer().get());
                cnv->pd->unlockAudioThread();

                Array<ObjectParameters> parameters = {};
                editor->sidebar->showParameters(object->gui->getText(), parameters);
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
        case ToFront: {
            auto objects = cnv->patch.getObjects();

            // The FORWARD double for loop makes sure that they keep their original order
            cnv->patch.startUndoSequence("ToFront");
            for (auto& object : objects) {
                for (auto* selectedBox : selectedBoxes) {
                    if (object == selectedBox->getPointer()) {
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
        case Forward: {
            auto objects = cnv->patch.getObjects();

            // The FORWARD double for loop makes sure that they keep their original order
            cnv->patch.startUndoSequence("MoveForward");
            for (auto& object : objects) {
                for (auto* selectedBox : selectedBoxes) {
                    if (object == selectedBox->getPointer()) {
                        selectedBox->toFront(false);
                        if (selectedBox->gui)
                            selectedBox->gui->moveForward();
                    }
                }
            }
            cnv->patch.startUndoSequence("MoveForward");
            cnv->synchronise();
            break;
        }
        case Backward: {
            auto objects = cnv->patch.getObjects();

            cnv->patch.startUndoSequence("MoveBackward");
            // The REVERSE double for loop makes sure that they keep their original order
            for (int i = objects.size() - 1; i >= 0; i--) {
                for (auto* selectedBox : selectedBoxes) {
                    if (objects[i] == selectedBox->getPointer()) {
                        selectedBox->toBack();
                        if (selectedBox->gui)
                            selectedBox->gui->moveBackward();
                    }
                }
            }
            cnv->patch.endUndoSequence("MoveBackward");
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

    auto* parent = ProjectInfo::canUseSemiTransparentWindows() ? nullptr : editor;

    popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(parent).withTargetScreenArea(Rectangle<int>(position, position.translated(1, 1))), ModalCallbackFunction::create(callback));
}

void Dialogs::showObjectMenu(PluginEditor* editor, Component* target)
{
    AddObjectMenu::show(editor, editor->getLocalArea(target, target->getLocalBounds()));
}
