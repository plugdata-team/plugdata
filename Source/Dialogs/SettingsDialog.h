
struct DAWAudioSettings : public Component
{
    explicit DAWAudioSettings(AudioProcessor& p) : processor(p)
    {
        addAndMakeVisible(latencySlider);
        latencySlider.setRange(0, 88200, 1);
        latencySlider.setTextValueSuffix(" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);
        
        addAndMakeVisible(tailLengthSlider);
        tailLengthSlider.setRange(0, 10.0f, 0.01f);
        tailLengthSlider.setTextValueSuffix(" Seconds");
        tailLengthSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);

        addAndMakeVisible(tailLengthLabel);
        tailLengthLabel.setText("Tail Length", dontSendNotification);
        tailLengthLabel.attachToComponent(&tailLengthSlider, true);
        
        addAndMakeVisible(latencyLabel);
        latencyLabel.setText("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);
        
        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.onValueChange = [this, proc]() { proc->setLatencySamples(latencySlider.getValue()); };
        tailLengthSlider.onValueChange = [this, proc]() { proc->tailLength.setValue(tailLengthSlider.getValue());};
    }

    void resized() override
    {
        latencySlider.setBounds(90, 5, getWidth() - 130, 20);
        tailLengthSlider.setBounds(90, 30, getWidth() - 130, 20);
    }

    void visibilityChanged() override
    {
        if(!isVisible()) return;
        
        auto* proc = dynamic_cast<PlugDataAudioProcessor*>(&processor);
        latencySlider.setValue(processor.getLatencySamples());
        tailLengthSlider.setValue(static_cast<float>(proc->tailLength.getValue()));
    }

    AudioProcessor& processor;
    Label latencyLabel;
    Label tailLengthLabel;
    
    Slider latencySlider;
    Slider tailLengthSlider;
};

class SearchPathComponent : public Component, public TableListBoxModel
{
   public:
    SearchPathComponent(ValueTree libraryTree) : tree(std::move(libraryTree))
    {
        table.setModel(this);
        table.setColour(ListBox::backgroundColourId, findColour(ResizableWindow::backgroundColourId));
        table.setRowHeight(24);
        table.setOutlineThickness(0);
        table.deselectAllRows();
        
        table.setColour(TableListBox::backgroundColourId, Colours::transparentBlack);

        table.getHeader().setStretchToFitActive(true);
        table.setHeaderHeight(0);
        table.getHeader().addColumn("Library Path", 1, 800, 50, 800, TableHeaderComponent::defaultFlags);

        addButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        addButton.setConnectedEdges(12);
        addButton.setName("statusbar:add");
        addButton.onClick = [this]()
        {
            openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                    [this](const FileChooser& f)
                                    {
                                        auto result = f.getResult();
                                        if (!result.exists()) return;

                                        auto child = ValueTree("Path");
                                        child.setProperty("Path", result.getFullPathName(), nullptr);

                                        tree.appendChild(child, nullptr);

                                        loadData();
                                    });
        };

        removeButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        removeButton.setConnectedEdges(12);
        removeButton.setName("statusbar:add");
        removeButton.onClick = [this]() mutable
        {
            int idx = table.getSelectedRow();
            tree.removeChild(idx, nullptr);
            loadData();
        };

        resetButton.onClick = [this]()
        {
            File abstractionsDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Abstractions");

            auto defaultPath = ValueTree("Path");
            defaultPath.setProperty("Path", abstractionsDir.getFullPathName(), nullptr);

            tree.removeAllChildren(nullptr);
            tree.appendChild(defaultPath, nullptr);
            loadData();
        };

        addAndMakeVisible(table);
        addAndMakeVisible(addButton);
        addAndMakeVisible(removeButton);
        addAndMakeVisible(resetButton);

        loadData();
    }

    void loadData()
    {
        items.clear();

        for (auto child : tree)
        {
            items.add(child.getProperty("Path"));
        }

        table.updateContent();
        table.selectRow(items.size() - 1);
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override
    {
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        g.setColour(rowIsSelected ? Colours::white : findColour(ComboBox::textColourId));
        const String item = tree.getChild(rowNumber).getProperty("Path").toString();

        g.drawText(item, 4, 0, width - 4, height, Justification::centredLeft, true);

    }

    int getNumRows() override
    {
        return items.size();
    }

    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        return nullptr;
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        tableBounds.removeFromBottom(30);

        table.setBounds(tableBounds);

        const int buttonHeight = 20;
        const int y = getHeight() - (buttonHeight + 7);
        const int x = getWidth() - 8;

        resetButton.changeWidthToFitText(buttonHeight);
        resetButton.setTopRightPosition(x, y + 5);

        addButton.setBounds(6, y, 30, 30);
        removeButton.setBounds(34, y, 30, 30);
    }
    
    void paint(Graphics& g) override
    {

        auto* viewport = table.getViewport();
        PlugDataLook::paintStripes(g, table.getRowHeight(), viewport->getViewedComponent()->getHeight(), table, table.getSelectedRow(), table.getViewport()->getViewPositionY(), true);
    
    }


   private:
    FileChooser openChooser = FileChooser("Choose path", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory), "");

    TextButton addButton = TextButton(Icons::Add);
    TextButton removeButton = TextButton(Icons::Clear);
    TextButton resetButton = TextButton("reset to defaults");

    TableListBox table;
    ValueTree tree;
    StringArray items;
};

struct SettingsComponent : public Component
{
    SettingsComponent(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree)
    {
        toolbarButtons = {new TextButton(Icons::Audio), new TextButton(Icons::Search), new TextButton(Icons::Keyboard)};

        auto* editor = dynamic_cast<ApplicationCommandManager*>(processor.getActiveEditor());

        if (manager)
        {
            panels.add(new AudioDeviceSelectorComponent(*manager, 1, 2, 1, 2, true, true, true, false));
        }
        else
        {
            panels.add(new DAWAudioSettings(processor));
        }

        panels.add(new SearchPathComponent(settingsTree.getChildWithName("Paths")));
        panels.add(new KeyMappingEditorComponent(*editor->getKeyMappings(), true));

        for (int i = 0; i < toolbarButtons.size(); i++)
        {
            toolbarButtons[i]->setClickingTogglesState(true);
            toolbarButtons[i]->setRadioGroupId(0110);
            toolbarButtons[i]->setConnectedEdges(12);
            toolbarButtons[i]->setName("toolbar:settings");
            addAndMakeVisible(toolbarButtons[i]);

            addChildComponent(panels[i]);
            toolbarButtons[i]->onClick = [this, i]() mutable { showPanel(i); };
        }

        toolbarButtons[0]->setToggleState(true, sendNotification);

    }

    void showPanel(int idx)
    {
        panels[currentPanel]->setVisible(false);
        panels[idx]->setVisible(true);
        currentPanel = idx;
        repaint();
    }

    void resized() override
    {
        int toolbarPosition = 2;
        for (auto& button : toolbarButtons)
        {
            button->setBounds(toolbarPosition, 2, 70, toolbarHeight - 2);
            toolbarPosition += 70;
        }

        for (auto* panel : panels)
        {
            if (panel == panels.getLast())
            {
                panel->setBounds(8, toolbarHeight, getWidth() - 8, getHeight() - toolbarHeight - 4);
                continue;
            }
            panel->setBounds(2, toolbarHeight, getWidth() - 2, getHeight() - toolbarHeight - 4);
        }
    }

    
    int currentPanel = 0;
    OwnedArray<Component> panels;
    AudioDeviceManager* deviceManager = nullptr;

    static constexpr int toolbarHeight = 45;

    OwnedArray<TextButton> toolbarButtons;
};

struct SettingsDialog : public Component
{
    AudioProcessor& audioProcessor;
    SettingsComponent settingsComponent;
    ComponentBoundsConstrainer constrainer;

    SettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, const ValueTree& settingsTree) : audioProcessor(processor), settingsComponent(processor, manager, settingsTree)
    {
        closeButton.reset(getLookAndFeel().createDocumentWindowButton(4));

        setCentrePosition(400, 400);
        setSize(600, 550);

        setVisible(false);
        
        addAndMakeVisible(&settingsComponent);
        addAndMakeVisible(closeButton.get());

        settingsComponent.addMouseListener(this, false);
        
        closeButton->onClick = [this]()
        {
            dynamic_cast<PlugDataAudioProcessor*>(&audioProcessor)->saveSettings();
           
            MessageManager::callAsync([this](){
                getTopLevelComponent()->removeChildComponent(this);
                delete this;
            });
        };
        
        background.reset(new BlackoutComponent(processor.getActiveEditor(), this, closeButton->onClick));

        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);

    }

    ~SettingsDialog() override
    {
        settingsComponent.removeMouseListener(this);
    }
    
    void visibilityChanged() override {
        background->setVisible(isVisible());
    }

    void resized() override
    {
        closeButton->setBounds(getWidth() - 35, 8, 28, 28);
        settingsComponent.setBounds(getLocalBounds().reduced(1));
    }
    
    void paint(Graphics& g) override
    {
        //g.fillAll(findColour(PlugDataColour::canvasColourId));
        
        g.setColour(findColour(PlugDataColour::canvasColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 5.0f);
        
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        
        auto toolbarBounds = Rectangle<float>(1, 1, getWidth() - 2, SettingsComponent::toolbarHeight);
        g.fillRoundedRectangle(toolbarBounds, 5.0f);
        g.fillRect(toolbarBounds.withTop(10.0f));
        
        if (settingsComponent.currentPanel > 0)
        {
            auto statusbarBounds = getLocalBounds().reduced(1).removeFromBottom(32).toFloat();
            g.setColour(findColour(PlugDataColour::toolbarColourId));
            
            g.fillRect(statusbarBounds.withHeight(20));
            g.fillRoundedRectangle(statusbarBounds, 5.0f);
        }
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 5.0f, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(1.0f, SettingsComponent::toolbarHeight, static_cast<float>(getWidth() - 2), SettingsComponent::toolbarHeight);
        
        if (settingsComponent.currentPanel > 0)
        {
            g.drawLine(1.0f, getHeight() - 33, static_cast<float>(getWidth() - 2), getHeight() - 33);
        }
        
    }

    std::unique_ptr<BlackoutComponent> background;
    std::unique_ptr<Button> closeButton;
};
