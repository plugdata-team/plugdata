#include <memory>

#pragma once

// Keymapping object based on JUCE's KeyMappingEditorComponent

class KeyMappingComponent : public SettingsDialogPanel
    , public ChangeListener {
public:
    explicit KeyMappingComponent(KeyPressMappingSet* mappingSet)
        : mappings(mappingSet)
    {
        mappings->addChangeListener(this);

        addAndMakeVisible(propertiesPanel);
        propertiesPanel.setTitle("Key Mappings");
        propertiesPanel.setColour(TreeView::backgroundColourId, findColour(PlugDataColour::panelBackgroundColourId));

        updateMappings();
    }

    PropertiesPanel* getPropertiesPanel() override
    {
        return &propertiesPanel;
    }

    /** Destructor. */
    ~KeyMappingComponent() override
    {
        if(mappings) mappings->removeChangeListener(this);
    }

    void updateMappings()
    {
        auto& viewport = propertiesPanel.getViewport();
        auto viewY = viewport.getViewPositionY();
        propertiesPanel.clear();

        auto resetMaxDefaults = [this] {
            Dialogs::showOkayCancelDialog(&confirmationDialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset all the key-mappings?",
                [this](int result) {
                    resetKeyMappingsToMaxCallback(result, this);
                });
        };
        auto resetPdDefaults = [this]() {
            Dialogs::showOkayCancelDialog(&confirmationDialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset all the key-mappings?",
                [this](int result) {
                    resetKeyMappingsToPdCallback(result, this);
                });
        };

        auto* resetMaxButton = new PropertiesPanel::ActionComponent(resetPdDefaults, Icons::Reset, "Reset to Pd defaults", true, false);
        auto* resetPdButton = new PropertiesPanel::ActionComponent(resetMaxDefaults, Icons::Reset, "Reset to Max defaults", false, true);

        propertiesPanel.addSection("Reset shortcuts", { resetMaxButton, resetPdButton });

        for (auto const& category : mappings->getCommandManager().getCommandCategories()) {
            Array<PropertiesPanelProperty*> properties;
            for (auto command : mappings->getCommandManager().getCommandsInCategory(category)) {
                properties.add(new KeyMappingProperty(*this, mappings->getCommandManager().getNameOfCommand(command), command));
            }

            propertiesPanel.addSection(category, properties);
        }

        viewport.setViewPosition(0.0f, viewY);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        auto keyMapTree = SettingsFile::getInstance()->getKeyMapTree();

        auto newTree = mappings->createXml(true)->toString();
        keyMapTree.setProperty("keyxml", newTree, nullptr);

        updateMappings();
    }

    static void resetKeyMappingsToPdCallback(int result, KeyMappingComponent* owner)
    {
        if (result == 0 && owner == nullptr)
            return;

        owner->getMappings().resetToDefaultMappings();
        owner->getMappings().sendChangeMessage();
    }

    static void resetKeyMappingsToMaxCallback(int result, KeyMappingComponent* owner)
    {
        if (result == 0 && owner == nullptr)
            return;

        auto& mappings = owner->getMappings();

        mappings.resetToDefaultMappings();

        for (int i = ObjectIDs::NewObject; i < ObjectIDs::NumObjects; i++) {
            mappings.clearAllKeyPresses(static_cast<CommandIDs>(i));
        }

        mappings.addKeyPress(ObjectIDs::NewObject, KeyPress(78, ModifierKeys::noModifiers, 'n'));
        mappings.addKeyPress(ObjectIDs::NewComment, KeyPress(67, ModifierKeys::noModifiers, 'c'));
        mappings.addKeyPress(ObjectIDs::NewBang, KeyPress(66, ModifierKeys::noModifiers, 'b'));
        mappings.addKeyPress(ObjectIDs::NewMessage, KeyPress(77, ModifierKeys::noModifiers, 'm'));
        mappings.addKeyPress(ObjectIDs::NewToggle, KeyPress(84, ModifierKeys::noModifiers, 't'));
        mappings.addKeyPress(ObjectIDs::NewNumbox, KeyPress(73, ModifierKeys::noModifiers, 'i'));
        mappings.addKeyPress(ObjectIDs::NewFloatAtom, KeyPress(70, ModifierKeys::noModifiers, 'f'));
        mappings.addKeyPress(ObjectIDs::NewVerticalSlider, KeyPress(83, ModifierKeys::noModifiers, 's'));

        mappings.sendChangeMessage();
    }

    /** Returns the KeyPressMappingSet that this component is acting upon. */
    KeyPressMappingSet& getMappings() const noexcept { return *mappings; }

    /** Returns the ApplicationCommandManager that this component is connected to. */
    ApplicationCommandManager& getCommandManager() const noexcept { return mappings->getCommandManager(); }

    /** This can be overridden to let you change the format of the string used
     to describe a keypress.

     This is handy if you're using non-standard KeyPress objects, e.g. for custom
     keys that are triggered by something else externally. If you override the
     method, be sure to let the base class's method handle keys you're not
     interested in.
     */
    static String getDescriptionForKeyPress(KeyPress const& key)
    {
        return key.getTextDescription();
    }

    void resized() override
    {
        propertiesPanel.setBounds(getLocalBounds());
    }

private:
    WeakReference<KeyPressMappingSet> mappings;
    PropertiesPanel propertiesPanel;

    std::unique_ptr<Dialog> confirmationDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyMappingComponent)

    class ChangeKeyButton : public Button {
    public:
        ChangeKeyButton(KeyMappingComponent& kec, CommandID command,
            String const& keyName, int keyIndex)
            : Button(keyName)
            , owner(kec)
            , commandID(command)
            , keyNum(keyIndex)
        {
            setWantsKeyboardFocus(false);
            setTriggeredOnMouseDown(keyNum >= 0);

            setTooltip(keyIndex < 0 ? "Adds a new key-mapping"
                                    : "Click to change this key-mapping");
        }

        void paintButton(Graphics& g, bool /*isOver*/, bool /*isDown*/) override
        {
            getLookAndFeel().drawKeymapChangeButton(g, getWidth(), getHeight(), *this,
                keyNum >= 0 ? convertURLtoUTF8(getName()) : String());
        }

        void clicked() override
        {
            if (keyNum >= 0) {
                Component::SafePointer<ChangeKeyButton> button(this);
                PopupMenu m;

                m.addItem("Change this key-mapping",
                    [button] {
                        if (button != nullptr)
                            button.getComponent()->assignNewKey();
                    });

                m.addSeparator();

                m.addItem("Remove this key-mapping",
                    [button] {
                        if (button != nullptr)
                            button->owner.getMappings().removeKeyPress(button->commandID,
                                button->keyNum);
                    });

                m.showMenuAsync(PopupMenu::Options().withTargetComponent(this));
            } else {
                assignNewKey(); // + button pressed..
            }
        }

        using Button::clicked;

        void fitToContent(int const h) noexcept
        {
            if (keyNum < 0)
                setSize(h, h);
            else
                setSize(jlimit(h * 4, h * 8, 6 + Font((float)h * 0.6f).getStringWidth(getName())), h);
        }

        class KeyEntryWindow : public AlertWindow {
        public:
            explicit KeyEntryWindow(KeyMappingComponent& kec)
                : AlertWindow("New key-mapping",
                    "Please press a key combination now...",
                    MessageBoxIconType::NoIcon)
                , owner(kec)
            {
                addButton("OK", 1);
                addButton("Cancel", 0);

                // (avoid return + escape keys getting processed by the buttons..)
                for (auto* child : getChildren())
                    child->setWantsKeyboardFocus(false);

                for (int i = 0; i < getNumButtons(); i++) {
                    auto& button = *getButton(i);
                    auto backgroundColour = findColour(PlugDataColour::dialogBackgroundColourId);
                    button.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
                    button.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
                    button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
                }

                setWantsKeyboardFocus(true);
                grabKeyboardFocus();
                setOpaque(false);
            }

            bool keyPressed(KeyPress const& key) override
            {
                lastPress = key;
                String message("Key:" + key.getTextDescription());

                auto previousCommand = owner.getMappings().findCommandForKeyPress(key);

                if (previousCommand != 0)
                    message << "\n\n("
                            << String("Currently assigned to \"CMDN\"").replace("CMDN", owner.getCommandManager().getNameOfCommand(previousCommand))
                            << ')';

                setMessage(message);
                return true;
            }

            bool keyStateChanged(bool) override
            {
                return true;
            }

            KeyPress lastPress;

        private:
            KeyMappingComponent& owner;

            JUCE_DECLARE_NON_COPYABLE(KeyEntryWindow)
        };

        static void assignNewKeyCallback(int result, ChangeKeyButton* button, KeyPress newKey)
        {
            if (result != 0 && button != nullptr)
                button->setNewKey(newKey, true);
        }

        void setNewKey(KeyPress const& newKey, bool dontAskUser)
        {
            if (newKey.isValid()) {
                auto previousCommand = owner.getMappings().findCommandForKeyPress(newKey);

                if (previousCommand == 0 || dontAskUser) {
                    owner.getMappings().removeKeyPress(newKey);

                    if (keyNum >= 0)
                        owner.getMappings().removeKeyPress(commandID, keyNum);

                    owner.getMappings().addKeyPress(commandID, newKey, keyNum);
                } else {
                    AlertWindow::showOkCancelBox(MessageBoxIconType::WarningIcon,
                        "Change key-mapping",
                        String("This key is already assigned to the command \"CMDN\"")
                                .replace("CMDN", owner.getCommandManager().getNameOfCommand(previousCommand))
                            + "\n\nDo you want to re-assign it to this new command instead?",
                        "Re-assign",
                        "Cancel",
                        this,
                        ModalCallbackFunction::forComponent(assignNewKeyCallback,
                            this, KeyPress(newKey)));
                }
            }
        }

        static void keyChosen(int result, ChangeKeyButton* button)
        {
            if (button != nullptr && button->currentKeyEntryWindow != nullptr) {
                if (result != 0) {
                    button->currentKeyEntryWindow->setVisible(false);
                    button->setNewKey(button->currentKeyEntryWindow->lastPress, false);
                }

                button->currentKeyEntryWindow.reset();
            }
        }

        void assignNewKey()
        {
            currentKeyEntryWindow = std::make_unique<KeyEntryWindow>(owner);
            currentKeyEntryWindow->enterModalState(true, ModalCallbackFunction::forComponent(keyChosen, this));
        }

    private:
        KeyMappingComponent& owner;
        CommandID const commandID;
        int const keyNum;
        std::unique_ptr<KeyEntryWindow> currentKeyEntryWindow;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChangeKeyButton)
    };

    class KeyMappingProperty : public PropertiesPanelProperty {
    public:
        KeyMappingProperty(KeyMappingComponent& kec, String const& name, CommandID command)
            : PropertiesPanelProperty(name)
            , owner(kec)
            , commandID(command)
        {
            setInterceptsMouseClicks(false, true);

            auto keyPresses = owner.getMappings().getKeyPressesAssignedToCommand(commandID);

            for (int i = 0; i < jmin((int)maxNumAssignments, keyPresses.size()); ++i)
                addKeyPressButton((keyPresses.getReference(i)).getTextDescription(), i);

            addKeyPressButton("Change Key Mapping", -1);
        }

        void addKeyPressButton(String const& desc, int const index)
        {
            auto* b = new ChangeKeyButton(owner, commandID, desc, index);
            keyChangeButtons.add(b);

            b->setVisible(keyChangeButtons.size() <= (int)maxNumAssignments);
            addChildComponent(b);
        }

        void resized() override
        {
            int x = getWidth() - 8;

            for (int i = keyChangeButtons.size(); --i >= 0;) {
                auto* b = keyChangeButtons.getUnchecked(i);

                b->fitToContent(getHeight() - 12);
                b->setTopLeftPosition(x - b->getWidth(), 6);
                x = b->getX() - 12;
            }
        }

    private:
        std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
        {
            return createIgnoredAccessibilityHandler(*this);
        }

        KeyMappingComponent& owner;
        OwnedArray<ChangeKeyButton> keyChangeButtons;
        CommandID const commandID;

        enum { maxNumAssignments = 3 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyMappingProperty)
    };
};
