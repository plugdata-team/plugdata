/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Dialogs.h"
#include "LookAndFeel.h"

#include <JuceHeader.h>

#include <memory>

struct SaveDialog : public Component
{
    SaveDialog()
    {
        setSize(400, 200);
        addAndMakeVisible(savelabel);
        addAndMakeVisible(cancel);
        addAndMakeVisible(dontsave);
        addAndMakeVisible(save);
        cancel.onClick = [this]
        {
            cb(0);
            delete this;  // yikesss
        };
        save.onClick = [this]
        {
            cb(2);
            delete this;  // yikesss
        };
        dontsave.onClick = [this]
        {
            cb(1);
            delete this;  // yikesss
        };
        cancel.changeWidthToFitText();
        dontsave.changeWidthToFitText();
        save.changeWidthToFitText();
        setOpaque(false);

        shadower.setOwner(this);
    }

    void resized() override
    {
        savelabel.setBounds(20, 25, 200, 30);
        cancel.setBounds(20, 80, 80, 25);
        dontsave.setBounds(200, 80, 80, 25);
        save.setBounds(300, 80, 80, 25);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);
    }

    std::function<void(int)> cb;

   private:
    DropShadow shadow = DropShadow(Colour{10, 10, 10}, 12, {0, 0});
    DropShadower shadower = DropShadower(shadow);

    Label savelabel = Label("savelabel", "Save Changes?");

    TextButton cancel = TextButton("Cancel");
    TextButton dontsave = TextButton("Don't Save");
    TextButton save = TextButton("Save");
};

struct ArrayDialog : public Component
{
    ArrayDialog()
    {
        setSize(400, 200);

        addAndMakeVisible(label);
        addAndMakeVisible(cancel);
        addAndMakeVisible(ok);

        cancel.onClick = [this]
        {
            cb(0, "", "");
            delete this;
        };
        ok.onClick = [this]
        {
            // Check if input is valid
            if (nameEditor.isEmpty())
            {
                nameEditor.setColour(TextEditor::outlineColourId, Colours::red);
                nameEditor.giveAwayKeyboardFocus();
                nameEditor.repaint();
            }
            if (sizeEditor.getText().getIntValue() < 0)
            {
                sizeEditor.setColour(TextEditor::outlineColourId, Colours::red);
                sizeEditor.giveAwayKeyboardFocus();
                sizeEditor.repaint();
            }
            if (nameEditor.getText().isNotEmpty() && sizeEditor.getText().getIntValue() >= 0)
            {
                cb(1, nameEditor.getText(), sizeEditor.getText());
                delete this;
            }
        };

        sizeEditor.setInputRestrictions(10, "0123456789");

        cancel.changeWidthToFitText();
        ok.changeWidthToFitText();

        addAndMakeVisible(nameLabel);
        addAndMakeVisible(sizeLabel);

        addAndMakeVisible(nameEditor);
        addAndMakeVisible(sizeEditor);

        nameEditor.setText("array1");
        sizeEditor.setText("100");

        setOpaque(false);
        shadower.setOwner(this);
    }

    void resized() override
    {
        label.setBounds(20, 7, 200, 30);
        cancel.setBounds(30, getHeight() - 40, 80, 25);
        ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

        nameEditor.setBounds(65, 45, getWidth() - 85, 25);
        sizeEditor.setBounds(65, 85, getWidth() - 85, 25);
        nameLabel.setBounds(8, 45, 52, 25);
        sizeLabel.setBounds(8, 85, 52, 25);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);

        g.setColour(findColour(ComboBox::outlineColourId).darker());
        g.drawRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f, 1.5f);
    }

    std::function<void(int, String, String)> cb;

   private:
    DropShadow shadow = DropShadow(Colour{10, 10, 10}, 12, {0, 0});
    DropShadower shadower = DropShadower(shadow);

    Label label = Label("savelabel", "Array Properties");

    Label nameLabel = Label("namelabel", "Name:");
    Label sizeLabel = Label("sizelabel", "Size:");

    TextEditor nameEditor;
    TextEditor sizeEditor;

    TextButton cancel = TextButton("Cancel");
    TextButton ok = TextButton("OK");
};

struct DAWAudioSettings : public Component
{
    explicit DAWAudioSettings(AudioProcessor& p) : processor(p)
    {
        addAndMakeVisible(latencySlider);
        latencySlider.setRange(0, 88200, 1);
        latencySlider.setTextValueSuffix(" Samples");
        latencySlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxRight, false, 100, 20);

        latencySlider.onValueChange = [this]() { processor.setLatencySamples(latencySlider.getValue()); };

        addAndMakeVisible(latencyLabel);
        latencyLabel.setText("Latency", dontSendNotification);
        latencyLabel.attachToComponent(&latencySlider, true);
    }

    void resized() override
    {
        latencySlider.setBounds(90, 5, getWidth() - 130, 20);
    }

    void visibilityChanged() override
    {
        latencySlider.setValue(processor.getLatencySamples());
    }

    AudioProcessor& processor;
    Label latencyLabel;
    Slider latencySlider;
};

class LibraryComponent : public Component, public TableListBoxModel
{
   public:
    LibraryComponent(ValueTree libraryTree, std::function<void()> updatePaths) : tree(std::move(libraryTree)), updateFunc(std::move(updatePaths))
    {
        table.setModel(this);
        table.setColour(ListBox::backgroundColourId, Colour(25, 25, 25));
        table.setRowHeight(30);

        // give it a border
        table.setColour(ListBox::outlineColourId, Colours::transparentBlack);
        table.setColour(ListBox::textColourId, Colours::white);

        table.setOutlineThickness(1);

        setColour(ListBox::textColourId, Colours::white);
        setColour(ListBox::outlineColourId, Colours::white);
        // setColour(ListBox::outlineColourId, Colours::white);

        // we could now change some initial settings..
        table.getHeader().setSortColumnId(1, true);    // sort forwards by the ID column
        table.getHeader().setColumnVisible(7, false);  // hide the "length" column until the user shows it

        // un-comment this line to have a go of stretch-to-fit mode
        table.getHeader().setStretchToFitActive(true);

        table.getHeader().setColour(TableHeaderComponent::textColourId, Colours::white);
        table.getHeader().setColour(TableHeaderComponent::backgroundColourId, findColour(Slider::thumbColourId));

        table.getHeader().addColumn("Library Path", 1, 800, 50, 800, TableHeaderComponent::defaultFlags);

        addButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        addButton.setConnectedEdges(12);
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

        addAndMakeVisible(table);
        addAndMakeVisible(addButton);

        loadData();
    }

    void loadData()
    {
        items.clear();

        for (auto child : tree)
        {
            items.add(child.getProperty("Path"));
            table.updateContent();
            table.selectRow(items.size() - 1);
        }

        updateFunc();
    }

    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override
    {
        g.fillAll((row % 2) ? findColour(ComboBox::backgroundColourId) : findColour(ResizableWindow::backgroundColourId));
    }

    // This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom
    // components.
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
    {
        g.setColour(Colours::white);
        // g.setFont (font);

        const String item = tree.getChild(rowNumber).getProperty("Path").toString();

        g.drawText(item, 2, 0, width - 4, height, Justification::centredLeft, true);

        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRect(width - 1, 0, 1, height);
    }

    int getNumRows() override
    {
        return items.size();
    }

    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;

        auto* rowComponent = new FileComponent(
            [this](int row)
            {
                tree.getChild(row).setProperty("Path", items[row], nullptr);
                updateFunc();
            },
            &items.getReference(rowNumber), rowNumber);

        rowComponent->deleteButton.onClick = [this, rowNumber]() mutable
        {
            tree.removeChild(rowNumber, nullptr);
            loadData();
        };

        return rowComponent;
    }

    void resized() override
    {
        table.setBounds(getLocalBounds());
        addButton.setBounds(getWidth() - 30, 0, 30, 30);
    }

    struct FileComponent : public Label
    {
        FileComponent(std::function<void(int)> cb, String* value, int rowIdx) : callback(std::move(cb)), row(rowIdx)
        {
            setEditable(false, true);

            setText(String(*value), dontSendNotification);

            addAndMakeVisible(deleteButton);
            deleteButton.toFront(true);
            deleteButton.setConnectedEdges(12);

            onTextChange = [this, value]()
            {
                *value = getText();
                callback(row);
            };
        }

        void resized() override
        {
            Label::resized();
            deleteButton.setBounds(getWidth() - 28, 0, 28, getHeight());
        }

        TextEditor* createEditorComponent() override
        {
            auto* editor = Label::createEditorComponent();

            return editor;
        }

        TextButton deleteButton = TextButton("x");

       private:
        std::function<void(int)> callback;

        int row;
    };

   private:
    FileChooser openChooser = FileChooser("Choose path", File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory), "");

    std::function<void()> updateFunc;

    TextButton addButton = TextButton("+");

    TableListBox table;
    ValueTree tree;
    StringArray items;
};

struct SettingsComponent : public Component
{
    SettingsComponent(AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths)
    {
        toolbarButtons = {new TextButton(Icons::Audio), new TextButton(Icons::Search)};

        for (auto& button : toolbarButtons)
        {
            button->setClickingTogglesState(true);
            button->setRadioGroupId(0110);
            button->setConnectedEdges(12);
            button->setName("toolbar:settings");
            addAndMakeVisible(button);
        }

        auto* libraryList = new LibraryComponent(settingsTree.getChildWithName("Paths"), std::move(updatePaths));
        libraryPanel.reset(libraryList);

        if (manager)
        {
            audioSetupComp = std::make_unique<AudioDeviceSelectorComponent>(*manager, 1, 2, 1, 2, true, true, true, false);
        }
        else
        {
            audioSetupComp = std::make_unique<DAWAudioSettings>(processor);
        }

        addAndMakeVisible(audioSetupComp.get());

        toolbarButtons[0]->onClick = [this]()
        {
            audioSetupComp->setVisible(true);
            libraryPanel->setVisible(false);
            resized();
        };

        toolbarButtons[1]->onClick = [this]()
        {
            audioSetupComp->setVisible(false);
            libraryPanel->setVisible(true);
            // make other panel visible
            resized();
        };

        addChildComponent(libraryPanel.get());

        toolbarButtons[0]->setToggleState(true, sendNotification);
    }

    void paint(Graphics& g) override
    {
        auto highlightColour = Colour(0xff42a2c8).darker(0.2);

        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);

        g.setColour(highlightColour);
        g.fillRect(2, 42, getWidth() - 4, 4);
    }

    void resized() override
    {
        int toolbarPosition = 2;
        for (auto& button : toolbarButtons)
        {
            button->setBounds(toolbarPosition, 0, 70, toolbarHeight);
            toolbarPosition += 70;
        }

        audioSetupComp->setBounds(0, toolbarHeight, getWidth(), getHeight() - toolbarHeight);

        libraryPanel->setBounds(0, toolbarHeight, getWidth(), getHeight() - toolbarHeight);
    }

    AudioDeviceManager* deviceManager = nullptr;
    std::unique_ptr<Component> audioSetupComp;

    std::unique_ptr<Component> libraryPanel;

    int toolbarHeight = 50;

    OwnedArray<TextButton> toolbarButtons;
};

struct SettingsDialog : public Component
{
    AudioProcessor& audioProcessor;

    SettingsComponent settingsComponent;
    ComponentDragger dragger;

    DropShadow shadow = DropShadow(Colour{10, 10, 10}, 12, {0, 0});
    DropShadower shadower = DropShadower(shadow);

    ComponentBoundsConstrainer constrainer;

    SettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths) : audioProcessor(processor), settingsComponent(processor, manager, std::move(settingsTree), std::move(updatePaths))
    {
        shadower.setOwner(this);
        closeButton.reset(getLookAndFeel().createDocumentWindowButton(4));

        setCentrePosition(400, 400);
        setSize(600, 550);

        setVisible(false);

        addAndMakeVisible(&settingsComponent);
        addAndMakeVisible(closeButton.get());

        settingsComponent.addMouseListener(this, false);

        closeButton->onClick = [this]() { setVisible(false); };

        constrainer.setMinimumOnscreenAmounts(600, 400, 400, 400);
    }

    ~SettingsDialog() override
    {
        settingsComponent.removeMouseListener(this);
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (e.getPosition().getY() < 30)
        {
            dragger.startDraggingComponent(this, e);
        }
    }

    void mouseDrag(const MouseEvent& e) override
    {
        if (e.getMouseDownPosition().getY() < 30)
        {
            dragger.dragComponent(this, e, &constrainer);
        }
    }

    void resized() override
    {
        closeButton->setBounds(getWidth() - 31, 3, 28, 28);
        settingsComponent.setBounds(getLocalBounds());
    }

    void paintOverChildren(Graphics& g) override
    {
        // Draw window title
        g.setColour(Colours::white);
        g.drawText("Settings", 0, 0, getWidth(), 30, Justification::centred, true);

        g.setColour(findColour(ComboBox::outlineColourId).darker());
        g.drawRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f, 1.5f);
    }

    void closeButtonPressed()
    {
        setVisible(false);
    }

    std::unique_ptr<Button> closeButton;
};

void Dialogs::showSaveDialog(Component* centre, std::function<void(int)> callback)
{
    auto* dialog = new SaveDialog;
    dialog->cb = std::move(callback);

    centre->addAndMakeVisible(dialog);

    dialog->setBounds((centre->getWidth() / 2.) - 200., 40, 400, 130);
}
void Dialogs::showArrayDialog(Component* centre, std::function<void(int, String, String)> callback)
{
    auto* dialog = new ArrayDialog;
    dialog->cb = std::move(callback);

    centre->addAndMakeVisible(dialog);

    dialog->setBounds((centre->getWidth() / 2.) - 200., 37, 300, 180);
}

std::unique_ptr<Component> Dialogs::createSettingsDialog(AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths)
{
    return std::make_unique<SettingsDialog>(processor, manager, settingsTree, updatePaths);
}

void Dialogs::showObjectMenu(Component* parent, Component* target, std::function<void(String)> cb)
{
    PopupMenu menu;

    menu.addItem(16, "Empty Object");
    menu.addSeparator();

    menu.addItem(1, "Numbox");
    menu.addItem(2, "Message");
    menu.addItem(3, "Bang");
    menu.addItem(4, "Toggle");
    menu.addItem(5, "Horizontal Slider");
    menu.addItem(6, "Vertical Slider");
    menu.addItem(7, "Horizontal Radio");
    menu.addItem(8, "Vertical Radio");

    menu.addSeparator();

    menu.addItem(9, "Float Atom");    // 11
    menu.addItem(10, "Symbol Atom");  // 12
    menu.addItem(16, "List Atom");  // 16

    menu.addSeparator();

    menu.addItem(11, "Array");          // 9
    menu.addItem(12, "GraphOnParent");  // 13
    menu.addItem(13, "Comment");        // 14
    menu.addItem(14, "Canvas");         // 10
    menu.addSeparator();
    menu.addItem(15, "Keyboard");

    auto callback = [cb](int choice)
    {
        if (choice < 1) return;

        String boxName;

        switch (choice)
        {
            case 1:
                boxName = "nbx";
                break;

            case 2:
                boxName = "msg";
                break;

            case 3:
                boxName = "bng";
                break;

            case 4:
                boxName = "tgl";
                break;

            case 5:
                boxName = "hsl";
                break;

            case 6:
                boxName = "vsl";
                break;

            case 7:
                boxName = "hradio";
                break;

            case 8:
                boxName = "vradio";
                break;
            case 9:
                boxName = "floatatom";
                break;

            case 10:
                boxName = "symbolatom";
                break;
            case 16:
                boxName = "listbox";
                break;

            case 11:
            {
                boxName = "array";
                break;
            }
            case 12:
                boxName = "graph";
                break;

            case 13:
                boxName = "comment";
                break;
            case 14:
                boxName = "cnv";
                break;
            case 15:
                boxName = "keyboard";
                break;
            default:
                break;
        }
        cb(boxName);
    };

    menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(target).withParentComponent(parent), ModalCallbackFunction::create(callback));
}
