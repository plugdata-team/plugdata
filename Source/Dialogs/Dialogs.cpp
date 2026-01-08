/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_devices/juce_audio_devices.h>

#include <utility>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/CachedStringWidth.h"

#include "Dialogs.h"

#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Components/ArrowPopupMenu.h"
#include "Components/Buttons.h"
#include "Components/SearchEditor.h"

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
#include "Standalone/PlugDataWindow.h"
#include "PatchStore.h"
#include "AboutPanel.h"

Dialog::Dialog(std::unique_ptr<Dialog>* ownerPtr, Component* editor, int const childWidth, int const childHeight, bool const showCloseButton, int const margin)
    : height(childHeight)
    , width(childWidth)
    , parentComponent(editor)
    , owner(ownerPtr)
    , backgroundMargin(margin)
{
    parentComponent->addAndMakeVisible(this);
    setBounds(0, 0, parentComponent->getWidth(), parentComponent->getHeight());
    setAlwaysOnTop(true);
    setWantsKeyboardFocus(true);

    if (showCloseButton) {
        closeButton.reset(getLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());
        closeButton->onClick = [this] {
            parentComponent->toFront(true);
            closeDialog();
        };
        closeButton->setAlwaysOnTop(true);
    }

    // Make sure titlebar buttons are greyed out when a dialog is showing
    if (auto* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent())) {
        if (ProjectInfo::isStandalone) {
            if (auto* closeButton = window->getCloseButton())
                closeButton->setEnabled(false);
            if (auto* minimiseButton = window->getMinimiseButton())
                minimiseButton->setEnabled(false);
            if (auto* maximiseButton = window->getMaximiseButton())
                maximiseButton->setEnabled(false);
        }
        window->repaint();
    }

    if (auto* pluginEditor = dynamic_cast<PluginEditor*>(editor)) {
        pluginEditor->nvgSurface.setRenderThroughImage(true);
    }
}

bool Dialog::wantsRoundedCorners() const
{
    // Check if the editor wants rounded corners
    if (auto const* editor = dynamic_cast<PluginEditor*>(parentComponent)) {
        return editor->wantsRoundedCorners();
    }
    // Otherwise assume rounded corners for the rest of the UI
    return true;
}

Component* Dialogs::showTextEditorDialog(String const& text, String filename, std::function<void(String, bool)> closeCallback, std::function<void(String)> saveCallback, float const desktopScale, bool const enableSyntaxHighlighting)
{
#if ENABLE_TESTING
    return nullptr;
#endif
    auto* editor = new TextEditorDialog(std::move(filename), enableSyntaxHighlighting, std::move(closeCallback), std::move(saveCallback), desktopScale);
    editor->editor.setText(text);
    return editor;
}

void Dialogs::clearTextEditorDialog(Component* dialog)
{
    if (!dialog)
        return;

    auto& editor = dynamic_cast<TextEditorDialog*>(dialog)->editor;
    editor.setText("");
}

void Dialogs::appendTextToTextEditorDialog(Component* dialog, String const& text)
{
    if (!dialog)
        return;

    auto& editor = dynamic_cast<TextEditorDialog*>(dialog)->editor;
    editor.setText(editor.getText() + text);
}

void Dialogs::showAskToSaveDialog(std::unique_ptr<Dialog>* target, Component* centre, String const& filename, std::function<void(int)> callback, int const margin, bool const withLogo)
{
    if (*target)
        return;

    auto* dialog = new Dialog(target, centre, 265, withLogo ? 270 : 210, false, margin);
    auto* saveDialog = new SaveDialog(dialog, filename, std::move(callback), withLogo);

    dialog->setViewedComponent(saveDialog);
    target->reset(dialog);

#if !JUCE_IOS
    centre->getTopLevelComponent()->toFront(true);
#endif
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
#if JUCE_IOS
    OSUtils::showMobileMainMenu(editor->getPeer(), [editor](int result) {
        if (result < 0)
            return;

        switch (result) {
        case 1: {
            editor->getTabComponent().newPatch();
            break;
        }
        case 2: {
            editor->getTabComponent().openPatch();
            break;
        }
        case 3: {
            if (auto* cnv = editor->getCurrentCanvas())
                cnv->save();
            break;
        }
        case 4: {
            if (auto* cnv = editor->getCurrentCanvas())
                cnv->saveAs();
            break;
        }
        case 5: {
            Dialogs::showSettingsDialog(editor);
            break;
        }
        case 6: {
            auto* dialog = new Dialog(&editor->openedDialog, editor, 360, 490, true);
            auto* aboutPanel = new AboutPanel();
            dialog->setViewedComponent(aboutPanel);
            editor->openedDialog.reset(dialog);
            break;
        }
        case 7: {
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::selectedThemes[0]);
            break;
        }
        case 8: {
            SettingsFile::getInstance()->setProperty("theme", PlugDataLook::selectedThemes[1]);
            break;
        }
        }
    });
    return;
#endif

    auto* popup = new MainMenu(editor);
    auto* parent = ProjectInfo::canUseSemiTransparentWindows() ? editor->getCalloutAreaComponent() : nullptr;

    ArrowPopupMenu::showMenuAsync(popup, PopupMenu::Options().withMinimumWidth(210).withMaximumNumColumns(1).withTargetComponent(centre).withParentComponent(parent),
        [editor, popup, settingsTree = SettingsFile::getInstance()->getValueTree()](int const result) mutable {
            switch (result) {
            case MainMenu::MenuItem::NewPatch: {
                editor->getTabComponent().newPatch();
                break;
            }
            case MainMenu::MenuItem::OpenPatch: {
                editor->getTabComponent().openPatch();
                break;
            }
            case MainMenu::MenuItem::Save: {
                if (auto* cnv = editor->getCurrentCanvas())
                    cnv->save();
                break;
            }
            case MainMenu::MenuItem::SaveAs: {
                if (auto* cnv = editor->getCurrentCanvas())
                    cnv->saveAs();
                break;
            }
            case MainMenu::MenuItem::CompiledMode: {
                bool const ticked = settingsTree.hasProperty("hvcc_mode") && static_cast<bool>(settingsTree.getProperty("hvcc_mode"));
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
            case MainMenu::MenuItem::Discover: {
                Dialogs::showStore(editor);
                break;
            }
            case MainMenu::MenuItem::Settings: {
                Dialogs::showSettingsDialog(editor);
                break;
            }
            case MainMenu::MenuItem::About: {
                auto* dialog = new Dialog(&editor->openedDialog, editor, 360, 490, true);
                auto* aboutPanel = new AboutPanel();
                dialog->setViewedComponent(aboutPanel);
                editor->openedDialog.reset(dialog);
                break;
            }
            default: {
                break;
            }
            }

            MessageManager::callAsync([popup, editor] {
                editor->showCalloutArea(false);
                delete popup;
            });
        });

    if (ProjectInfo::canUseSemiTransparentWindows()) {
        editor->showCalloutArea(true);
    }
}

void Dialogs::showMultiChoiceDialog(std::unique_ptr<Dialog>* target, Component* parent, String const& title, std::function<void(int)> const& callback, StringArray const& options, String const& icon)
{

    class MultiChoiceDialog : public Component {

        TextLayout layout;
        String icon;

    public:
        MultiChoiceDialog(Dialog* dialog, String const& title, std::function<void(int)> const& callback, StringArray const& options, String const& icon)
            : icon(icon)
            , label("", title)
        {
            auto attributedTitle = AttributedString(title);
            attributedTitle.setJustification(Justification::horizontallyCentred);
            attributedTitle.setFont(Fonts::getBoldFont().withHeight(14));
            attributedTitle.setColour(findColour(PlugDataColour::panelTextColourId));

            for (int i = 0; i < options.size(); i++) {
                auto* button = buttons.add(new TextButton(options[i]));

                auto backgroundColour = findColour(PlugDataColour::dialogBackgroundColourId);
                button->setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
                button->setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
                button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
                addAndMakeVisible(button);
                button->onClick = [dialog, callback, i] {
                    callback(i);
                    dialog->closeDialog();
                };
            }

            auto constexpr width = 270;
            layout.createLayout(attributedTitle, width - 32);
            setSize(width, getBestHeight());

            setOpaque(false);
        }

        int getBestHeight() const
        {
            return buttons.size() * 34 + layout.getHeight() + 116;
        }

        void paint(Graphics& g) override
        {
            AttributedString warningIcon(icon);
            warningIcon.setFont(Fonts::getIconFont().withHeight(48));
            warningIcon.setColour(findColour(PlugDataColour::panelTextColourId));
            warningIcon.setJustification(Justification::centred);
            warningIcon.draw(g, getLocalBounds().toFloat().removeFromTop(90));

            auto contentBounds = getLocalBounds().withTrimmedTop(66).reduced(16);
            layout.draw(g, contentBounds.removeFromTop(48).toFloat());
        }

        void resized() override
        {
            auto contentBounds = getLocalBounds().reduced(16);
            contentBounds.removeFromTop(layout.getHeight() + 90);

            for (auto* button : buttons) {
                button->setBounds(contentBounds.removeFromTop(28));
                contentBounds.removeFromTop(6);
            }
        }

    private:
        Label label;
        OwnedArray<TextButton> buttons;
    };

    auto* dialog = new Dialog(target, parent, 270, 220, false);
    auto* dialogContent = new MultiChoiceDialog(dialog, title, callback, options, icon);

    dialog->height = dialogContent->getBestHeight();
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
    auto* dialogContent = new ObjectBrowserDialog(parent);

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

void Dialogs::showStore(PluginEditor* editor)
{
    auto* dialog = new Dialog(&editor->openedDialog, editor, 850, 550, true);
    auto* dialogContent = new PatchStore();
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

void Dialogs::showCanvasRightClickMenu(Canvas* cnv, Component* originalComponent, Point<int> const position)
{
#if JUCE_IOS
    // OSUtils::showMobileCanvasMenu(cnv->getPeer());
    return;
#endif

    struct QuickActionsBar : public PopupMenu::CustomComponent {
        struct QuickActionButton : public TextButton {
            explicit QuickActionButton(String const& buttonText)
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
                    g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius);

                    textColour = findColour(PlugDataColour::sidebarTextColourId);
                }

                Fonts::drawIcon(g, getButtonText(), std::max(0, getWidth() - getHeight()) / 2, 0, getHeight(), textColour, 12.8f);
            }
        };

        explicit QuickActionsBar(PluginEditor* editor)
        {
            auto commandIds = StackArray<CommandID, 5> { CommandIDs::Cut, CommandIDs::Copy, CommandIDs::Paste, CommandIDs::Duplicate, CommandIDs::Delete };

            int index = 0;
            for (auto* button : StackArray<QuickActionButton*, 5> { &cut, &copy, &paste, &duplicate, &remove }) {
                addAndMakeVisible(button);
                auto const id = commandIds[index];

                button->setCommandToTrigger(&editor->commandManager, id, false);

                if (auto* registeredInfo = editor->commandManager.getCommandForID(id)) {
                    ApplicationCommandInfo info(*registeredInfo);
                    editor->commandManager.getTargetForCommand(id, info);
                    bool const canPerformCommand = (info.flags & ApplicationCommandInfo::isDisabled) == 0;
                    button->setEnabled(canPerformCommand);
                } else {
                    button->setEnabled(false);
                }
                index++;
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
            auto const buttonWidth = getWidth() / 5;
            auto bounds = getLocalBounds();

            for (auto* button : SmallArray<TextButton*> { &cut, &copy, &paste, &duplicate, &remove }) {
                constexpr auto buttonHeight = 26;
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
    auto addCommandItem = [editor = cnv->editor](PopupMenu& menu, CommandID const commandID, String const& displayName = "") {
        if (auto* registeredInfo = editor->commandManager.getCommandForID(commandID)) {
            ApplicationCommandInfo info(*registeredInfo);
            editor->getCommandInfo(commandID, info);

            PopupMenu::Item i;
            i.text = displayName.isNotEmpty() ? std::move(displayName) : info.shortName;
            i.itemID = static_cast<int>(commandID);
            i.commandManager = &editor->commandManager;
            i.isEnabled = (info.flags & ApplicationCommandInfo::isDisabled) == 0;
            i.isTicked = (info.flags & ApplicationCommandInfo::isTicked) != 0;
            menu.addItem(std::move(i));
        }
    };

    cnv->cancelConnectionCreation();

    // Info about selection status
    auto selectedBoxes = cnv->getSelectionOfType<Object>();

    // If we directly right-clicked on an object, make sure it has been added to selection
    if (!originalComponent) {
        return;
    }
    if (auto* obj = dynamic_cast<Object*>(originalComponent)) {
        selectedBoxes.add_unique(obj);
    } else if (auto* parentOfTypeObject = originalComponent->findParentComponentOfClass<Object>()) {
        selectedBoxes.add_unique(parentOfTypeObject);
    }

    bool const hasSelection = selectedBoxes.not_empty();
    bool const multiple = selectedBoxes.size() > 1;
    bool const locked = getValue<bool>(cnv->locked);

    auto object = Component::SafePointer<Object>(hasSelection ? selectedBoxes.front() : nullptr);

    // Find top-level object, so we never trigger it on an object inside a graph
    if (object && object->findParentComponentOfClass<Object>()) {
        while (auto* nextObject = object->findParentComponentOfClass<Object>()) {
            object = nextObject;
        }
    }

    auto* editor = cnv->editor;
    auto params = object && object->gui ? object->gui->getParameters() : ObjectParameters();

    enum MenuOptions {
        Extra = 200,
        Help,
        Reference,
        ToFront,
        Forward,
        Backward,
        ToBack,
        Properties,

        AlignLeft,
        AlignHCentre,
        AlignRight,
        AlignHDistribute,
        AlignTop,
        AlignVCentre,
        AlignBottom,
        AlignVDistribute
    };
    // Create popup menu
    PopupMenu popupMenu;

    popupMenu.addCustomItem(Extra, std::make_unique<QuickActionsBar>(editor), nullptr, "Quick Actions");
    popupMenu.addSeparator();

    if (!multiple && object && object->gui) {
        object->gui->getMenuOptions(popupMenu);
    } else {
        popupMenu.addItem(-1, "Open", false);
    }

    popupMenu.addSeparator();
    popupMenu.addItem(Help, "Help", hasSelection && !multiple);
    popupMenu.addItem(Reference, "Reference", hasSelection && !multiple);
    popupMenu.addSeparator();

    bool selectedConnection = false, noneSegmented = true;
    for (auto const& connection : cnv->getSelectionOfType<Connection>()) {
        noneSegmented = noneSegmented && !connection->isSegmented();
        selectedConnection = true;
    }

    popupMenu.addItem("Curved Connection", selectedConnection, selectedConnection && !noneSegmented, [editor, noneSegmented] {
        bool const segmented = noneSegmented;
        auto* cnv = editor->getCurrentCanvas();

        // cnv->patch.startUndoSequence("ChangeSegmentedPaths");

        for (auto const& connection : cnv->getSelectionOfType<Connection>()) {
            connection->setSegmented(segmented);
        }

        // cnv->patch.endUndoSequence("ChangeSegmentedPaths");
    });
    addCommandItem(popupMenu, CommandIDs::ConnectionPathfind);

    popupMenu.addSeparator();
    addCommandItem(popupMenu, CommandIDs::Encapsulate);
    addCommandItem(popupMenu, CommandIDs::Triggerize);
    popupMenu.addSeparator();

    PopupMenu orderMenu;
    orderMenu.addItem(ToFront, "To Front", object != nullptr && !locked);
    orderMenu.addItem(Forward, "Move forward", object != nullptr && !locked);
    orderMenu.addItem(Backward, "Move backward", object != nullptr && !locked);
    orderMenu.addItem(ToBack, "To Back", object != nullptr && !locked);
    popupMenu.addSubMenu("Order", orderMenu, !locked);

    class AlignmentMenuItem : public PopupMenu::CustomComponent {

        String menuItemIcon;
        String menuItemText;

        bool isActive = true;

    public:
        AlignmentMenuItem(String icon, String text, bool const isActive = true)
            : menuItemIcon(std::move(icon))
            , menuItemText(std::move(text))
            , isActive(isActive)
        {
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = 170;
            idealHeight = 24;
        }

        void paint(Graphics& g) override
        {
            auto r = getLocalBounds();

            auto const colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isItemHighlighted() && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(0, 1), Corners::defaultCornerRadius);
            }
            g.setColour(colour);

            r.reduce(jmin(5, r.getWidth() / 20), 0);

            auto const maxFontHeight = static_cast<float>(r.getHeight()) / 1.3f;
            auto const iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).withSizeKeepingCentre(maxFontHeight, maxFontHeight);

            if (menuItemIcon.isNotEmpty()) {
                Fonts::drawIcon(g, menuItemIcon, iconArea.translated(3.0f, 0.0f), colour, std::min(15.0f, maxFontHeight), true);
            }
            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));

            int const fontHeight = std::min(17.0f, maxFontHeight);
            r.removeFromRight(3);
            Fonts::drawFittedText(g, menuItemText, r, colour, fontHeight);
        }
    };

    PopupMenu alignMenu;
    addCommandItem(alignMenu, CommandIDs::Tidy);
    alignMenu.addSeparator();

    auto alignIsActive = cnv->getSelectionOfType<Object>().size() > 1;
    auto distributeIsActive = cnv->getSelectionOfType<Object>().size() > 2;

    alignMenu.addCustomItem(AlignLeft, std::make_unique<AlignmentMenuItem>(Icons::AlignLeft, "Align left", alignIsActive), nullptr, "Align left");
    alignMenu.addCustomItem(AlignHCentre, std::make_unique<AlignmentMenuItem>(Icons::AlignVCentre, "Align centre", alignIsActive), nullptr, "Align centre");
    alignMenu.addCustomItem(AlignRight, std::make_unique<AlignmentMenuItem>(Icons::AlignRight, "Align right", alignIsActive), nullptr, "Align right");
    alignMenu.addCustomItem(AlignHDistribute, std::make_unique<AlignmentMenuItem>(Icons::AlignHDistribute, "Space horizonally", distributeIsActive), nullptr, "Space horizonally");
    alignMenu.addSeparator();
    alignMenu.addCustomItem(AlignTop, std::make_unique<AlignmentMenuItem>(Icons::AlignTop, "Align top", alignIsActive), nullptr, "Align top");
    alignMenu.addCustomItem(AlignVCentre, std::make_unique<AlignmentMenuItem>(Icons::AlignHCentre, "Align middle", alignIsActive), nullptr, "Align middle");
    alignMenu.addCustomItem(AlignBottom, std::make_unique<AlignmentMenuItem>(Icons::AlignBottom, "Align bottom", alignIsActive), nullptr, "Align bottom");
    alignMenu.addCustomItem(AlignVDistribute, std::make_unique<AlignmentMenuItem>(Icons::AlignVDistribute, "Space vertically", distributeIsActive), nullptr, "Space vertically");
    popupMenu.addSubMenu("Align", alignMenu, !locked);

    popupMenu.addSeparator();
    popupMenu.addItem(Properties, "Properties", (originalComponent == cnv || (object && params.getParameters().not_empty())) && !locked);
    // showObjectReferenceDialog
    auto callback = [cnv, editor, object, originalComponent, selectedBoxes](int const result) mutable {
        cnv->grabKeyboardFocus();
        editor->showCalloutArea(false);

        // Make sure that iolets don't hang in hovered state
        for (auto* o : cnv->objects) {
            for (auto* iolet : o->iolets)
                iolet->repaint();
        }

        if (result == Properties) {
            auto toShow = SmallArray<Component*>();

            if (originalComponent == cnv) {
                SmallArray<ObjectParameters, 6> parameters = { cnv->getInspectorParameters() };
                toShow.add(cnv);
                editor->sidebar->forceShowParameters(toShow, parameters);
            } else if (object && object->gui) {
                // this makes sure that objects can handle the "properties" message as well if they like, for example for [else/properties]
                if (auto gobj = object->gui->ptr.get<t_gobj>()) {
                    auto const* pdClass = pd_class(&object->getPointer()->g_pd);
                    if (auto const propertiesFn = class_getpropertiesfn(pdClass)) {
                        propertiesFn(gobj.get(), cnv->patch.getRawPointer());
                    }
                }

                SmallArray<ObjectParameters, 6> parameters = { object->gui->getParameters() };
                toShow.add(object);
                editor->sidebar->forceShowParameters(toShow, parameters);
            }

            return;
        }

        if ((!object && result < 100) || result <= 0) {
            return;
        }

        if (object)
            object->repaint();

        switch (result) {
        case ToFront: {
            auto objects = cnv->patch.getObjects();

            // The FORWARD double for loop makes sure that they keep their original order
            cnv->patch.startUndoSequence("ToFront");
            for (auto& o : objects) {
                for (auto* selectedBox : selectedBoxes) {
                    if (o == selectedBox->getPointer()) {
                        selectedBox->toFront(false);
                        if (selectedBox->gui)
                            selectedBox->gui->moveToFront();
                    }
                }
            }
            cnv->patch.endUndoSequence("ToFront");
            cnv->synchronise();
            break;
        }
        case Forward: {
            auto objects = cnv->patch.getObjects();

            // The FORWARD double for loop makes sure that they keep their original order
            cnv->patch.startUndoSequence("MoveForward");
            for (auto& o : objects) {
                for (auto* selectedBox : selectedBoxes) {
                    if (o == selectedBox->getPointer()) {
                        selectedBox->toFront(false);
                        if (selectedBox->gui)
                            selectedBox->gui->moveForward();
                    }
                }
            }
            cnv->patch.endUndoSequence("MoveForward");
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
            Dialogs::showObjectReferenceDialog(&editor->openedDialog, editor, object->getType());
            break;
        case AlignLeft:
            cnv->alignObjects(Align::Left);
            break;
        case AlignHCentre:
            cnv->alignObjects(Align::HCentre);
            break;
        case AlignRight:
            cnv->alignObjects(Align::Right);
            break;
        case AlignHDistribute:
            cnv->alignObjects(Align::HDistribute);
            break;
        case AlignTop:
            cnv->alignObjects(Align::Top);
            break;
        case AlignVCentre:
            cnv->alignObjects(Align::VCentre);
            break;
        case AlignBottom:
            cnv->alignObjects(Align::Bottom);
            break;
        case AlignVDistribute:
            cnv->alignObjects(Align::VDistribute);
            break;
        default:
            break;
        }
    };

    auto* parent = ProjectInfo::canUseSemiTransparentWindows() ? editor->getCalloutAreaComponent() : nullptr;
    if (parent)
        parent->addToDesktop(ComponentPeer::windowIsTemporary);

    popupMenu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(parent).withTargetScreenArea(Rectangle<int>(position, position.translated(1, 1))), ModalCallbackFunction::create(callback));
}

void Dialogs::showObjectMenu(PluginEditor* editor, Component const* target)
{
    AddObjectMenu::show(editor, target->getScreenBounds());
}

void Dialogs::dismissFileDialog()
{
    fileChooser.reset(nullptr);
}

void Dialogs::showOpenDialog(std::function<void(URL)> const& callback, bool const canSelectFiles, bool const canSelectDirectories, String const& extension, String const& lastFileId, Component* parentComponent)
{
    bool nativeDialog = SettingsFile::getInstance()->wantsNativeDialog();
    auto initialFile = lastFileId.isNotEmpty() ? SettingsFile::getInstance()->getLastBrowserPathForId(lastFileId) : ProjectInfo::appDataDir;
    if (!initialFile.exists())
        initialFile = ProjectInfo::appDataDir;

    auto fileChooserText = "Choose file to open...";

    if (!canSelectFiles && canSelectDirectories) {
        fileChooserText = "Select directory...";
    }

#if JUCE_IOS
    fileChooser = std::make_unique<FileChooser>(fileChooserText, initialFile, "*", nativeDialog, false, parentComponent);
#else
    fileChooser = std::make_unique<FileChooser>(fileChooserText, initialFile, extension, nativeDialog, false, nullptr);
#endif
    auto openChooserFlags = FileBrowserComponent::openMode;

    if (canSelectFiles)
        openChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(openChooserFlags | FileBrowserComponent::canSelectFiles);
    if (canSelectDirectories)
        openChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(openChooserFlags | FileBrowserComponent::canSelectDirectories);

    fileChooser->launchAsync(openChooserFlags,
        [callback, lastFileId](FileChooser const& fileChooser) {
            auto const result = fileChooser.getResult();

            auto const lastDir = result.isDirectory() ? result : result.getParentDirectory();
            if (result.exists()) {
                SettingsFile::getInstance()->setLastBrowserPathForId(lastFileId, lastDir);
                callback(fileChooser.getURLResult());
            }
            Dialogs::fileChooser = nullptr;
        });
}

void Dialogs::showSaveDialog(std::function<void(URL)> const& callback, String const& extension, String const& lastFileId, Component* parentComponent, bool const directoryMode)
{
    bool nativeDialog = SettingsFile::getInstance()->wantsNativeDialog();
    auto initialFile = lastFileId.isNotEmpty() ? SettingsFile::getInstance()->getLastBrowserPathForId(lastFileId) : ProjectInfo::appDataDir;
    if (!initialFile.exists())
        initialFile = ProjectInfo::appDataDir;

#if JUCE_IOS
    fileChooser = std::make_unique<FileChooser>("Choose save location...", initialFile, extension, nativeDialog, false, parentComponent);
#else
    fileChooser = std::make_unique<FileChooser>("Choose save location...", initialFile, extension, nativeDialog, false, nullptr);
#endif
    auto saveChooserFlags = FileBrowserComponent::saveMode;

    if (directoryMode)
        saveChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(saveChooserFlags | FileBrowserComponent::canSelectDirectories);
    else
        saveChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(saveChooserFlags | FileBrowserComponent::canSelectFiles);

    // TODO: checks if this still causes issues
#if !JUCE_LINUX && !JUCE_BSD
    saveChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(saveChooserFlags | FileBrowserComponent::warnAboutOverwriting);
#endif

    fileChooser->launchAsync(saveChooserFlags,
        [callback, lastFileId](FileChooser const& fileChooser) {
            auto const result = fileChooser.getResult();
            auto parentDirectory = result.getParentDirectory();
            if (parentDirectory.exists()) {
                SettingsFile::getInstance()->setLastBrowserPathForId(lastFileId, parentDirectory);
                callback(fileChooser.getURLResult());
                Dialogs::fileChooser = nullptr;
            }
        });
}
