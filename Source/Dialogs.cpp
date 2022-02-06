/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Dialogs.h"

#include <JuceHeader.h>

#include <memory>

#include "Inspector.h"

SaveDialog::SaveDialog()
{
  // setLookAndFeel(&mainLook);
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

void SaveDialog::resized()
{
  savelabel.setBounds(20, 25, 200, 30);
  cancel.setBounds(20, 80, 80, 25);
  dontsave.setBounds(200, 80, 80, 25);
  save.setBounds(300, 80, 80, 25);
}

void SaveDialog::paint(Graphics& g)
{
  g.setColour(MainLook::firstBackground);
  g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);
}

void SaveDialog::show(Component* centre, std::function<void(int)> callback)
{
  auto* dialog = new SaveDialog;
  dialog->cb = std::move(callback);

  centre->addAndMakeVisible(dialog);

  dialog->setBounds((centre->getWidth() / 2.) - 200., 40, 400, 130);
}

ArrayDialog::ArrayDialog()
{
  // setLookAndFeel(&mainLook);
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

  setOpaque(false);
  shadower.setOwner(this);
}

void ArrayDialog::show(Component* centre, std::function<void(int, String, String)> callback)
{
  auto* dialog = new ArrayDialog;
  dialog->cb = std::move(callback);

  dialog->nameEditor.setText("array1");
  dialog->sizeEditor.setText("100");

  centre->addAndMakeVisible(dialog);

  dialog->setBounds((centre->getWidth() / 2.) - 200., 37, 300, 180);
}

void ArrayDialog::resized()
{
  label.setBounds(20, 7, 200, 30);
  cancel.setBounds(30, getHeight() - 40, 80, 25);
  ok.setBounds(getWidth() - 110, getHeight() - 40, 80, 25);

  nameEditor.setBounds(65, 45, getWidth() - 85, 25);
  sizeEditor.setBounds(65, 85, getWidth() - 85, 25);
  nameLabel.setBounds(8, 45, 52, 25);
  sizeLabel.setBounds(8, 85, 52, 25);
}

void ArrayDialog::paint(Graphics& g)
{
  g.setColour(MainLook::firstBackground);
  g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);

  g.setColour(findColour(ComboBox::outlineColourId).darker());
  g.drawRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f, 1.5f);
}

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
    table.getHeader().setColour(TableHeaderComponent::backgroundColourId, MainLook::highlightColour);

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
  void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override { g.fillAll((row % 2) ? MainLook::firstBackground : MainLook::secondBackground); }

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

  int getNumRows() override { return items.size(); }

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

    // auto b = table.getHeader().getBounds().removeFromRight(25);
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
      // deleteButton.setLookAndFeel(&look);

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

SettingsComponent::SettingsComponent(Resources& r, AudioProcessor& processor, AudioDeviceManager* manager, ValueTree settingsTree, std::function<void()> updatePaths) : lnf(r)
{
  for (auto& button : toolbarButtons)
  {
    button->setClickingTogglesState(true);
    button->setRadioGroupId(0110);
    button->setLookAndFeel(&lnf);
    button->setConnectedEdges(12);
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

void SettingsComponent::paint(Graphics& g)
{
  auto highlightColour = Colour(0xff42a2c8).darker(0.2);

  g.setColour(MainLook::firstBackground);
  g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f);

  g.setColour(findColour(ComboBox::outlineColourId).darker());
  g.drawRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 3.0f, 1.5f);

  g.setColour(highlightColour);
  g.fillRect(0, 42, getWidth(), 4);
}

void SettingsComponent::resized()
{
  int toolbarPosition = 0;
  for (auto& button : toolbarButtons)
  {
    button->setBounds(toolbarPosition, 0, 70, toolbarHeight);
    toolbarPosition += 70;
  }

  audioSetupComp->setBounds(0, toolbarHeight, getWidth(), getHeight() - toolbarHeight);

  libraryPanel->setBounds(0, toolbarHeight, getWidth(), getHeight() - toolbarHeight);
}
