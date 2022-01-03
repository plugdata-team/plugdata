/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PluginEditor.h"
#include "Canvas.h"
#include "Connection.h"
#include "Dialogs.h"
#include "Edge.h"
#include "Pd/x_libpd_mod_utils.h"

#include "juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h"

//==============================================================================

PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p, Console* debugConsole)
    : AudioProcessorEditor(&p)
    , pd(p)
    , levelmeter(p.parameters, p.meterSource)
{
    console = debugConsole;

    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, MainLook::firstBackground);
    tabbar.setColour(TabbedComponent::outlineColourId, MainLook::firstBackground);

    setLookAndFeel(&mainLook);

    addAndMakeVisible(levelmeter);

    tabbar.onTabChange = [this](int idx) {
        Edge::connectingEdge = nullptr;

        if (idx == -1)
            return;

        // update GraphOnParent when changing tabs
        for (auto* box : getCurrentCanvas()->boxes) {
            if (box->graphics && box->graphics->getGUI().getType() == pd::Type::GraphOnParent) {
                auto* cnv = box->graphics.get()->getCanvas();
                if (cnv)
                    cnv->synchronise();
            }
            if (box->graphics && (box->graphics->getGUI().getType() == pd::Type::Subpatch || box->graphics->getGUI().getType() == pd::Type::GraphOnParent)) {
                box->updatePorts();
            }
        }

        auto* cnv = getCurrentCanvas();
        if (cnv->patch.getPointer()) {
            cnv->patch.setCurrent();
        }

        cnv->checkBounds();

        timerCallback();
    };
    
    addAndMakeVisible(tabbar);
    addAndMakeVisible(console);
    addChildComponent(inspector);

    bypassButton.setTooltip("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setConnectedEdges(12);
    bypassButton.setLookAndFeel(&statusbarLook);
    addAndMakeVisible(bypassButton);
    bypassButton.onClick = [this]() {
        pd.setBypass(!bypassButton.getToggleState());
    };

    bypassButton.setToggleState(true, dontSendNotification);
    

    lockButton.setTooltip("Lock");
    lockButton.setClickingTogglesState(true);
    lockButton.setConnectedEdges(12);
    lockButton.setLookAndFeel(&statusbarLook2);
    lockButton.onClick = [this]() {
        pd.locked = lockButton.getToggleState();

        lockButton.setButtonText(lockButton.getToggleState() ? CharPointer_UTF8("\xef\x80\xa3") : CharPointer_UTF8("\xef\x82\x9c"));

        sendChangeMessage();
    };
    addAndMakeVisible(lockButton);

    connectionStyleButton.setTooltip("Curve connections");
    connectionStyleButton.setClickingTogglesState(true);
    connectionStyleButton.setConnectedEdges(12);
    connectionStyleButton.setLookAndFeel(&statusbarLook);
    connectionStyleButton.onClick = [this]() {
        pd.settingsTree.setProperty(Identifiers::connectionStyle, connectionStyleButton.getToggleState(), nullptr);
        for (auto* connection : getCurrentCanvas()->connections)
            connection->resized();
    };
    connectionStyleButton.setToggleState((bool)pd.settingsTree.getProperty(Identifiers::connectionStyle), dontSendNotification);

    addAndMakeVisible(connectionStyleButton);

    for (auto& button : toolbarButtons) {
        button.setLookAndFeel(&toolbarLook);
        button.setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // New button
    toolbarButtons[0].setTooltip("New Project");
    toolbarButtons[0].onClick = [this]() {
        auto createFunc = [this]() {
            tabbar.clearTabs();
            canvases.clear();
            auto* cnv = canvases.add(new Canvas(*this, false));
            cnv->title = "Untitled Patcher";
            mainCanvas = cnv;
            mainCanvas->createPatch();
            addTab(cnv);
        };

        if (getMainCanvas()->changed()) {
            SaveDialog::show(this, [this, createFunc](int result) {
                if (result == 2) {
                    saveProject([this, createFunc]() mutable {
                        createFunc();
                    });
                } else if (result == 1) {
                    createFunc();
                }
            });
        } else {
            createFunc();
        }
    };

    // Open button
    toolbarButtons[1].setTooltip("Open Project");
    toolbarButtons[1].onClick = [this]() {
        openProject();
    };

    // Save button
    toolbarButtons[2].setTooltip("Open Project");
    toolbarButtons[2].onClick = [this]() {
        saveProject();
    };

    //  Undo button
    toolbarButtons[3].setTooltip("Undo");
    toolbarButtons[3].onClick = [this]() {
        getCurrentCanvas()->undo();
    };

    // Redo button
    toolbarButtons[4].setTooltip("Redo");
    toolbarButtons[4].onClick = [this]() {
        getCurrentCanvas()->redo();
    };

    // New object button
    toolbarButtons[5].setTooltip("Create Object");
    toolbarButtons[5].onClick = [this]() {
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

        menu.addItem(9, "Float Atom"); // 11
        menu.addItem(10, "Symbol Atom"); // 12

        menu.addSeparator();

        menu.addItem(11, "Array"); // 9
        menu.addItem(12, "GraphOnParent"); // 13
        menu.addItem(13, "Comment"); // 14
        menu.addItem(14, "Panel"); // 10
        menu.addSeparator();
        menu.addItem(15, "Keyboard");
       

        auto callback = [this](int choice) {
            if (choice < 1)
                return;

            String boxName;

            switch (choice) {
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
                    
            case 11: {
                ArrayDialog::show(this, [this](int result, String name, String size) {
                    if (result) {
                        auto* cnv = getCurrentCanvas();
                        auto* box = new Box(cnv, "graph " + name + " " + size);
                        cnv->boxes.add(box);
                    }
                });
                return;
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
            }
            auto* cnv = getCurrentCanvas();
            cnv->boxes.add(new Box(cnv, boxName));
        };

        menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(toolbarButtons[5]), ModalCallbackFunction::create(callback));
    };

    // Initialise settings dialog for DAW and standalone
    if (pd.wrapperType == AudioPluginInstance::wrapperType_Standalone) {
        auto pluginHolder = StandalonePluginHolder::getInstance();
        settingsDialog.reset(new SettingsDialog(pd, &pluginHolder->deviceManager, pd.settingsTree, [this]() {
            pd.updateSearchPaths();
        }));
    } else {
        settingsDialog.reset(new SettingsDialog(pd, nullptr, pd.settingsTree, [this]() {
            pd.updateSearchPaths();
        }));
    }

    addChildComponent(settingsDialog.get());
    
    // Show settings
    toolbarButtons[6].setTooltip("Settings");
    toolbarButtons[6].onClick = [this]() {
        settingsDialog->setVisible(true);
        settingsDialog->setBounds(getLocalBounds().withSizeKeepingCentre(600, 400));
        settingsDialog->toFront(false);
        settingsDialog->resized();
    };

    // Hide sidebar
    hideButton.setTooltip("Hide Sidebar");
    hideButton.setLookAndFeel(&toolbarLook);
    hideButton.setClickingTogglesState(true);
    hideButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hideButton.setConnectedEdges(12);
    hideButton.onClick = [this]() {
        sidebarHidden = hideButton.getToggleState();
        hideButton.setButtonText(sidebarHidden ? CharPointer_UTF8("\xef\x81\x93") : CharPointer_UTF8("\xef\x81\x94"));

        toolbarButtons[8].setVisible(!sidebarHidden);
        toolbarButtons[9].setVisible(!sidebarHidden);

        repaint();
        resized();
    };

    // Sidebar selectors (inspector or console)
    toolbarButtons[8].setTooltip("Show Console");
    toolbarButtons[8].setClickingTogglesState(true);
    toolbarButtons[8].setRadioGroupId(101);
    
    toolbarButtons[8].setTooltip("Show Inspector");
    toolbarButtons[9].setClickingTogglesState(true);
    toolbarButtons[9].setRadioGroupId(101);

    //  Open console
    toolbarButtons[8].onClick = [this]() {
        console->setVisible(true);
        inspector.setVisible(false);
    };

    // Open inspector
    toolbarButtons[9].onClick = [this]() {
        console->setVisible(false);
        inspector.setVisible(true);
    };

    addAndMakeVisible(hideButton);

    // window size limits
    restrainer.setSizeLimits(600, 300, 4000, 4000);
    resizer.reset(new ResizableCornerComponent(this, &restrainer));
    addAndMakeVisible(resizer.get());

    setSize(pd.lastUIWidth, pd.lastUIHeight);
}

PlugDataPluginEditor::~PlugDataPluginEditor()
{
    setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
    hideButton.setLookAndFeel(nullptr);
    lockButton.setLookAndFeel(nullptr);
    connectionStyleButton.setLookAndFeel(nullptr);

    for (auto& button : toolbarButtons) {
        button.setLookAndFeel(nullptr);
    }
}

//==============================================================================
void PlugDataPluginEditor::paint(Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    auto baseColour = MainLook::firstBackground;
    auto highlightColour = MainLook::highlightColour;

    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);

    // Sidebar
    g.setColour(baseColour.darker(0.1));
    g.fillRect(getWidth() - sWidth, dragbarWidth, sWidth + 10, getHeight() - toolbarHeight);

    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight - 4);

    g.setColour(highlightColour);
    g.drawRoundedRectangle({ -4.0f, 39.0f, (float)getWidth() + 9, 20.0f }, 12.0, 4.0);

    // Make sure we cant see the bottom half of the rounded rectangle
    g.setColour(baseColour);
    g.fillRect(0, toolbarHeight - 4, getWidth(), toolbarHeight + 16);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbarHeight, getWidth(), statusbarHeight);

    // Draggable bar
    g.setColour(baseColour);
    g.fillRect(getWidth() - sWidth, dragbarWidth, statusbarHeight, getHeight() - (toolbarHeight - statusbarHeight));
}

void PlugDataPluginEditor::resized()
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, sidebarWidth);

    int sContentWidth = sWidth - dragbarWidth;

    int sbarY = toolbarHeight - 4;

    console->setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    console->toFront(false);

    inspector.setBounds(getWidth() - sContentWidth, sbarY + 2, sContentWidth, getHeight() - sbarY);
    inspector.toFront(false);

    tabbar.setBounds(0, sbarY, getWidth() - sWidth, getHeight() - sbarY - statusbarHeight);
    tabbar.toFront(false);

    bypassButton.setBounds(getWidth() - sWidth - 40, getHeight() - 27, 27, 27);

    int jumpPositions[2] = { 3, 5 };
    int idx = 0;
    int toolbarPosition = 0;
    for (int b = 0; b < 8; b++) {
        auto& button = toolbarButtons[b];
        int spacing = (25 * (idx >= jumpPositions[0])) + (25 * (idx >= jumpPositions[1])) + 10;
        button.setBounds(toolbarPosition + spacing, 0, 70, toolbarHeight);
        toolbarPosition += 70;
        idx++;
    }

    hideButton.setBounds(std::min(getWidth() - sWidth, getWidth() - 80), 0, 70, toolbarHeight);
    toolbarButtons[8].setBounds(std::min(getWidth() - sWidth + 90, getWidth() - 80), 0, 70, toolbarHeight);
    toolbarButtons[9].setBounds(std::min(getWidth() - sWidth + 160, getWidth() - 80), 0, 70, toolbarHeight);

    lockButton.setBounds(8, getHeight() - 27, 27, 27);
    connectionStyleButton.setBounds(38, getHeight() - 27, 27, 27);

    levelmeter.setBounds(getWidth() - sWidth - 150, getHeight() - 27, 100, 27);

    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    resizer->toFront(false);

    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();
}

void PlugDataPluginEditor::mouseDown(const MouseEvent& e)
{
    Rectangle<int> drag_bar(getWidth() - sidebarWidth, dragbarWidth, sidebarWidth, getHeight() - toolbarHeight);
    if (drag_bar.contains(e.getPosition()) && !sidebarHidden) {
        draggingSidebar = true;
        dragStartWidth = sidebarWidth;
    } else {
        draggingSidebar = false;
    }
}

void PlugDataPluginEditor::mouseDrag(const MouseEvent& e)
{
    if (draggingSidebar) {
        sidebarWidth = dragStartWidth - e.getDistanceFromDragStartX();
        repaint();
        resized();
    }
}

void PlugDataPluginEditor::mouseUp(const MouseEvent& e)
{
    draggingSidebar = false;
}

void PlugDataPluginEditor::openProject()
{

    auto openFunc = [this](const FileChooser& f) {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
            openFile(openedFile.getFullPathName());
        }
    };

    if (getMainCanvas()->changed()) {

        SaveDialog::show(this, [this, openFunc](int result) {
            if (result == 2) {
                saveProject([this, openFunc]() mutable {
                    openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
                });
            } else if (result != 0) {
                openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
            }
        });
    } else {
        openChooser.launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
    }
}


void PlugDataPluginEditor::openFile(String path) {
    tabbar.clearTabs();
    pd.loadPatch(path);
}

void PlugDataPluginEditor::saveProject(std::function<void()> nestedCallback)
{
    auto to_save = pd.getCanvasContent();

    saveChooser.launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, to_save, nestedCallback](const FileChooser& f) mutable {
        File result = saveChooser.getResult();

        FileOutputStream ostream(result);
        ostream.writeString(to_save);

        getCurrentCanvas()->title = result.getFileName();

        nestedCallback();
    });

    getCurrentCanvas()->hasChanged = false;
}

void PlugDataPluginEditor::timerCallback()
{
    auto* cnv = getCurrentCanvas();
    // cnv->patch.setCurrent();

    for (auto& box : cnv->boxes) {
        if (box->graphics && box->isShowing()) {
            box->graphics->updateValue();
        }
    }

    updateUndoState();

}


void PlugDataPluginEditor::updateUndoState()
{

    pd.setThis();

    toolbarButtons[5].setEnabled(!pd.locked);

    if (getCurrentCanvas() && getCurrentCanvas()->patch.getPointer() && !pd.locked) {
        getCurrentCanvas()->patch.setCurrent();

        getCurrentCanvas()->hasChanged = true;

        toolbarButtons[3].setEnabled(pd.canUndo);
        toolbarButtons[4].setEnabled(pd.canRedo);
    } else {
        toolbarButtons[3].setEnabled(false);
        toolbarButtons[4].setEnabled(false);
    }
}

Canvas* PlugDataPluginEditor::getCurrentCanvas()
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PlugDataPluginEditor::getMainCanvas()
{
    return mainCanvas;
}

Canvas* PlugDataPluginEditor::getCanvas(int idx)
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx))) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }

    return nullptr;
}

void PlugDataPluginEditor::addTab(Canvas* cnv)
{
    tabbar.addTab(cnv->title, findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);

    int tabIdx = tabbar.getNumTabs() - 1;
    tabbar.setCurrentTabIndex(tabIdx);

    if (tabbar.getNumTabs() > 1) {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    } else {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }

    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);

    auto* closeButton = new TextButton("x");

    closeButton->onClick = [this, tabButton]() mutable {
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for (int i = 0; i < tabbar.getNumTabs(); i++) {
            if (tabbar.getTabbedButtonBar().getTabButton(i) == tabButton) {
                idx = i;
                break;
            }
        }

        if (idx == -1)
            return;

        if (tabbar.getCurrentTabIndex() == idx) {
            tabbar.setCurrentTabIndex(0, false);
        }

        auto* cnv = getCanvas(idx);
        canvases.removeObject(cnv);
        tabbar.removeTab(idx);

        tabbar.setCurrentTabIndex(0, true);

        if (tabbar.getNumTabs() == 1) {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };

    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setColour(TextButton::textColourOnId, Colours::white);
    closeButton->setColour(TextButton::textColourOffId, Colours::white);
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);

    closeButton->setVisible(tabIdx != 0);
    closeButton->setSize(28, 28);

    tabbar.repaint();

    cnv->setVisible(true);
    // cnv->setBounds(0, 0, 1000, 700);
}
