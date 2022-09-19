#pragma once

// Keymapping object based on JUCE's KeyMappingEditorComponent

class  KeyMappingComponent  : public Component
{
public:
    KeyMappingComponent (KeyPressMappingSet& mappingSet) : mappings (mappingSet),
    resetPdButton ("Reset to Pd defaults"), resetMaxButton ("Reset to Max defaults")
    {
        treeItem.reset (new TopLevelItem (*this));

        addAndMakeVisible (resetPdButton);
        resetPdButton.onClick = [this]
        {
            AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon,
                                          "Reset to defaults",
                                          "Are you sure you want to reset all the key-mappings",
                                          "Reset",
                                          {}, this,
                                          ModalCallbackFunction::forComponent (resetKeyMappingsToPdCallback, this));
        };
        
        
        addAndMakeVisible (resetMaxButton);
        resetMaxButton.onClick = [this]
        {
            AlertWindow::showOkCancelBox (MessageBoxIconType::QuestionIcon,
                                          "Reset to defaults",
                                          "Are you sure you want to reset all the key-mappings?",
                                          "Reset",
                                          {}, this,
                                          ModalCallbackFunction::forComponent (resetKeyMappingsToMaxCallback, this));
        };

        addAndMakeVisible (tree);
        tree.setTitle ("Key Mappings");
        tree.setColour (TreeView::backgroundColourId, findColour (backgroundColourId));
        tree.setRootItemVisible (false);
        tree.setDefaultOpenness (true);
        tree.setRootItem (treeItem.get());
        tree.setIndentSize (12);
    }
    
    /** Destructor. */
    ~KeyMappingComponent()
    {
        tree.setRootItem (nullptr);
    }
    
    static void resetKeyMappingsToPdCallback (int result, KeyMappingComponent* owner)
    {
        if (result == 0 && owner == nullptr) return;
        
        owner->getMappings().resetToDefaultMappings();
    }
    
    static void resetKeyMappingsToMaxCallback (int result, KeyMappingComponent* owner)
    {
        if (result == 0 && owner == nullptr) return;
        
        auto& mappings = owner->getMappings();
        
        mappings.resetToDefaultMappings();

        for(int i = CommandIDs::NewObject; i < mappings.getCommandManager().getNumCommands(); i++) {
            mappings.clearAllKeyPresses(static_cast<CommandIDs>(i));
        }
        
        mappings.addKeyPress(CommandIDs::NewObject, KeyPress(78, ModifierKeys::noModifiers, 'n'));
        mappings.addKeyPress(CommandIDs::NewComment, KeyPress(67, ModifierKeys::noModifiers, 'c'));
        mappings.addKeyPress(CommandIDs::NewBang, KeyPress(66, ModifierKeys::noModifiers, 'b'));
        mappings.addKeyPress(CommandIDs::NewMessage, KeyPress(77, ModifierKeys::noModifiers, 'm'));
        mappings.addKeyPress(CommandIDs::NewToggle, KeyPress(84, ModifierKeys::noModifiers, 't'));
        mappings.addKeyPress(CommandIDs::NewNumbox, KeyPress(73, ModifierKeys::noModifiers, 'i'));
        mappings.addKeyPress(CommandIDs::NewFloatAtom, KeyPress(70, ModifierKeys::noModifiers, 'f'));
        mappings.addKeyPress(CommandIDs::NewVerticalSlider, KeyPress(83, ModifierKeys::noModifiers, 's'));
    }
    
    
    /** Returns the KeyPressMappingSet that this component is acting upon. */
    KeyPressMappingSet& getMappings() const noexcept                { return mappings; }
    
    /** Returns the ApplicationCommandManager that this component is connected to. */
    ApplicationCommandManager& getCommandManager() const noexcept   { return mappings.getCommandManager(); }
    
    
    /** This can be overridden to let you change the format of the string used
     to describe a keypress.
     
     This is handy if you're using non-standard KeyPress objects, e.g. for custom
     keys that are triggered by something else externally. If you override the
     method, be sure to let the base class's method handle keys you're not
     interested in.
     */
    String getDescriptionForKeyPress (const KeyPress& key)
    {
        return key.getTextDescription();
    }
    
    //==============================================================================
    /** A set of colour IDs to use to change the colour of various aspects of the editor.
     
     These constants can be used either via the Component::setColour(), or LookAndFeel::setColour()
     methods.
     
     @see Component::setColour, Component::findColour, LookAndFeel::setColour, LookAndFeel::findColour
     */
    enum ColourIds
    {
        backgroundColourId  = 0x100ad00,    /**< The background colour to fill the editor background. */
        textColourId        = 0x100ad01,    /**< The colour for the text. */
    };
    
    
    void parentHierarchyChanged() override
    {
        treeItem->changeListenerCallback (nullptr);
    }
    
    void resized() override
    {
        int h = getHeight();

        const int buttonHeight = 20;
        h -= buttonHeight + 8;
        int x = getWidth() - 8;
        
        resetPdButton.changeWidthToFitText(buttonHeight);
        resetPdButton.setTopRightPosition(x, h + 6);
        
        resetMaxButton.changeWidthToFitText(buttonHeight);
        resetMaxButton.setTopRightPosition(x - (resetPdButton.getWidth() + 10), h + 6);

        tree.setBounds(0, 0, getWidth(), h);
    }
    
    
private:
    //==============================================================================
    KeyPressMappingSet& mappings;
    TreeView tree;
    TextButton resetPdButton;
    TextButton resetMaxButton;
    
    class TopLevelItem;
    class ChangeKeyButton;
    class MappingItem;
    class CategoryItem;
    class ItemComponent;
    std::unique_ptr<TopLevelItem> treeItem;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyMappingComponent)
    
    
    class ChangeKeyButton  : public Button
    {
    public:
        ChangeKeyButton (KeyMappingComponent& kec, CommandID command,
                         const String& keyName, int keyIndex)
        : Button (keyName),
        owner (kec),
        commandID (command),
        keyNum (keyIndex)
        {
            setWantsKeyboardFocus (false);
            setTriggeredOnMouseDown (keyNum >= 0);
            
            setTooltip (keyIndex < 0 ? "Adds a new key-mapping"
                        : "Click to change this key-mapping");
        }
        
        void paintButton (Graphics& g, bool /*isOver*/, bool /*isDown*/) override
        {
            getLookAndFeel().drawKeymapChangeButton (g, getWidth(), getHeight(), *this,
                                                     keyNum >= 0 ? getName() : String());
        }
        
        void clicked() override
        {
            if (keyNum >= 0)
            {
                Component::SafePointer<ChangeKeyButton> button (this);
                PopupMenu m;
                
                m.addItem ("Change this key-mapping",
                           [button]
                           {
                    if (button != nullptr)
                        button.getComponent()->assignNewKey();
                });
                
                m.addSeparator();
                
                m.addItem ("Remove this key-mapping",
                           [button]
                           {
                    if (button != nullptr)
                        button->owner.getMappings().removeKeyPress (button->commandID,
                                                                    button->keyNum);
                });
                
                m.showMenuAsync (PopupMenu::Options().withTargetComponent (this));
            }
            else
            {
                assignNewKey();  // + button pressed..
            }
        }
        
        using Button::clicked;
        
        void fitToContent (const int h) noexcept
        {
            if (keyNum < 0)
                setSize (h, h);
            else
                setSize (jlimit (h * 4, h * 8, 6 + Font ((float) h * 0.6f).getStringWidth (getName())), h);
        }
        
        //==============================================================================
        class KeyEntryWindow  : public AlertWindow
        {
        public:
            KeyEntryWindow (KeyMappingComponent& kec)
            : AlertWindow ("New key-mapping",
                           "Please press a key combination now...",
                           MessageBoxIconType::NoIcon),
            owner (kec)
            {
                addButton ("OK", 1);
                addButton ("Cancel", 0);
                
                // (avoid return + escape keys getting processed by the buttons..)
                for (auto* child : getChildren())
                    child->setWantsKeyboardFocus (false);
                
                setWantsKeyboardFocus (true);
                grabKeyboardFocus();
            }
            
            bool keyPressed (const KeyPress& key) override
            {
                lastPress = key;
                String message ("Key:" + key.getTextDescription());
                                
                                auto previousCommand = owner.getMappings().findCommandForKeyPress (key);
                                
                                if (previousCommand != 0)
                                message << "\n\n("
                                << String("Currently assigned to \"CMDN\"") .replace ("CMDN", owner.getCommandManager().getNameOfCommand(previousCommand))
                                << ')';
                                
                                setMessage (message);
                                return true;
            }
            
            bool keyStateChanged (bool) override
            {
                return true;
            }
            
            KeyPress lastPress;
            
        private:
            KeyMappingComponent& owner;
            
            JUCE_DECLARE_NON_COPYABLE (KeyEntryWindow)
        };
        
        static void assignNewKeyCallback (int result, ChangeKeyButton* button, KeyPress newKey)
        {
            if (result != 0 && button != nullptr)
                button->setNewKey (newKey, true);
        }
        
        void setNewKey (const KeyPress& newKey, bool dontAskUser)
        {
            if (newKey.isValid())
            {
                auto previousCommand = owner.getMappings().findCommandForKeyPress (newKey);
                
                if (previousCommand == 0 || dontAskUser)
                {
                    owner.getMappings().removeKeyPress (newKey);
                    
                    if (keyNum >= 0)
                        owner.getMappings().removeKeyPress (commandID, keyNum);
                    
                    owner.getMappings().addKeyPress (commandID, newKey, keyNum);
                }
                else
                {
                    AlertWindow::showOkCancelBox (MessageBoxIconType::WarningIcon,
                                                  "Change key-mapping",
                                                  String("This key is already assigned to the command \"CMDN\"")
                                                  .replace ("CMDN", owner.getCommandManager().getNameOfCommand (previousCommand))
                                                  + "\n\nDo you want to re-assign it to this new command instead?",
                                                  "Re-assign",
                                                  "Cancel",
                                                  this,
                                                  ModalCallbackFunction::forComponent (assignNewKeyCallback,
                                                                                       this, KeyPress (newKey)));
                }
            }
        }
        
        static void keyChosen (int result, ChangeKeyButton* button)
        {
            if (button != nullptr && button->currentKeyEntryWindow != nullptr)
            {
                if (result != 0)
                {
                    button->currentKeyEntryWindow->setVisible (false);
                    button->setNewKey (button->currentKeyEntryWindow->lastPress, false);
                }
                
                button->currentKeyEntryWindow.reset();
            }
        }
        
        void assignNewKey()
        {
            currentKeyEntryWindow.reset (new KeyEntryWindow (owner));
            currentKeyEntryWindow->enterModalState (true, ModalCallbackFunction::forComponent (keyChosen, this));
        }
        
    private:
        KeyMappingComponent& owner;
        const CommandID commandID;
        const int keyNum;
        std::unique_ptr<KeyEntryWindow> currentKeyEntryWindow;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChangeKeyButton)
    };
    
    //==============================================================================
    class ItemComponent  : public Component
    {
    public:
        ItemComponent (KeyMappingComponent& kec, CommandID command)
        : owner (kec), commandID (command)
        {
            setInterceptsMouseClicks (false, true);
            
            auto keyPresses = owner.getMappings().getKeyPressesAssignedToCommand(commandID);
            
            for (int i = 0; i < jmin((int) maxNumAssignments, keyPresses.size()); ++i)
                addKeyPressButton((keyPresses.getReference(i)).getTextDescription(), i);
            
            addKeyPressButton("Change Key Mapping", -1);
        }
        
        void addKeyPressButton (const String& desc, const int index)
        {
            auto* b = new ChangeKeyButton (owner, commandID, desc, index);
            keyChangeButtons.add (b);

            b->setVisible (keyChangeButtons.size() <= (int) maxNumAssignments);
            addChildComponent (b);
        }
        
        void paint (Graphics& g) override
        {
            g.setFont ((float) getHeight() * 0.7f);
            g.setColour (owner.findColour (KeyMappingComponent::textColourId));
            
            g.drawFittedText (owner.getCommandManager().getNameOfCommand (commandID),
                              4, 0, jmax (40, getChildComponent (0)->getX() - 5), getHeight(),
                              Justification::centredLeft, true);
        }
        
        void resized() override
        {
            int x = getWidth() - 4;
            
            for (int i = keyChangeButtons.size(); --i >= 0;)
            {
                auto* b = keyChangeButtons.getUnchecked(i);
                
                b->fitToContent (getHeight() - 2);
                b->setTopRightPosition (x, 1);
                x = b->getX() - 5;
            }
        }
        
    private:
        std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
        {
            return createIgnoredAccessibilityHandler (*this);
        }
        
        KeyMappingComponent& owner;
        OwnedArray<ChangeKeyButton> keyChangeButtons;
        const CommandID commandID;
        
        enum { maxNumAssignments = 3 };
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ItemComponent)
    };
    
    //==============================================================================
    class MappingItem  : public TreeViewItem
    {
    public:
        MappingItem (KeyMappingComponent& kec, CommandID command)
        : owner (kec), commandID (command)
        {}
        
        String getUniqueName() const override                      { return String ((int) commandID) + "_id"; }
        bool mightContainSubItems() override                       { return false; }
        int getItemHeight() const override                         { return 20; }
        std::unique_ptr<Component> createItemComponent() override  { return std::make_unique<ItemComponent> (owner, commandID); }
        String getAccessibilityName() override                     { return owner.getCommandManager().getNameOfCommand(commandID); }
        
    private:
        KeyMappingComponent& owner;
        const CommandID commandID;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MappingItem)
    };
    
    
    //==============================================================================
    class CategoryItem  : public TreeViewItem
    {
    public:
        CategoryItem (KeyMappingComponent& kec, const String& name)
        : owner (kec), categoryName (name)
        {}
        
        String getUniqueName() const override       { return categoryName + "_cat"; }
        bool mightContainSubItems() override        { return true; }
        int getItemHeight() const override          { return 22; }
        String getAccessibilityName() override      { return categoryName; }
        
        void paintItem (Graphics& g, int width, int height) override
        {
            g.setFont (Font ((float) height * 0.7f, Font::bold));
            g.setColour (owner.findColour (KeyMappingComponent::textColourId));
            
            g.drawText (categoryName, 2, 0, width - 2, height, Justification::centredLeft, true);
        }
        
        void itemOpennessChanged (bool isNowOpen) override
        {
            if (isNowOpen)
            {
                if (getNumSubItems() == 0)
                    for (auto command : owner.getCommandManager().getCommandsInCategory (categoryName))
                        addSubItem (new MappingItem (owner, command));
            }
            else
            {
                clearSubItems();
            }
        }
        
    private:
        KeyMappingComponent& owner;
        String categoryName;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategoryItem)
    };
    
    //==============================================================================
    class TopLevelItem   : public TreeViewItem,
    private ChangeListener
    {
    public:
        TopLevelItem (KeyMappingComponent& kec)   : owner (kec)
        {
            setLinesDrawnForSubItems (false);
            owner.getMappings().addChangeListener (this);
        }
        
        ~TopLevelItem() override
        {
            owner.getMappings().removeChangeListener (this);
        }
        
        bool mightContainSubItems() override             { return true; }
        String getUniqueName() const override            { return "keys"; }
        
        void changeListenerCallback (ChangeBroadcaster*) override
        {
            const OpennessRestorer opennessRestorer (*this);
            clearSubItems();
            
            for (auto category : owner.getCommandManager().getCommandCategories())
            {
                int count = 0;
                
                for (auto command : owner.getCommandManager().getCommandsInCategory (category))
                    ++count;
                
                if (count > 0)
                    addSubItem (new CategoryItem (owner, category));
            }
        }
        
    private:
        KeyMappingComponent& owner;
    };
    
};
