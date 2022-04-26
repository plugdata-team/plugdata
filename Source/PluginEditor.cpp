/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "PluginEditor.h"

#include "LookAndFeel.h"

#include <memory>

#include "Canvas.h"
#include "Connection.h"
#include "Dialogs.h"
#include "x_libpd_mod_utils.h"

PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p) : AudioProcessorEditor(&p), pd(p), statusbar(p), sidebar(&p)
{
    toolbarButtons = {new TextButton(Icons::New),  new TextButton(Icons::Open), new TextButton(Icons::Save),     new TextButton(Icons::SaveAs), new TextButton(Icons::Undo),
                      new TextButton(Icons::Redo), new TextButton(Icons::Add),  new TextButton(Icons::Settings), new TextButton(Icons::Hide),   new TextButton(Icons::Pin)};

#if PLUGDATA_ROUNDED
    setResizable(true, false);
#else
    setResizable(true, true);
#endif
    
    tooltipWindow->setOpaque(false);
    
    addKeyListener(&statusbar);
    addKeyListener(getKeyMappings());

    pd.locked.addListener(this);
    pd.zoomScale.addListener(this);

    pd.settingsTree.addListener(this);

    setWantsKeyboardFocus(true);
    registerAllCommandsForTarget(this);
    
    for(auto& seperator : seperators) {
        addChildComponent(&seperator);
    }


    auto keymap = p.settingsTree.getChildWithName("Keymap");
    if (keymap.isValid())
    {
        auto xmlStr = keymap.getProperty("keyxml").toString();
        auto elt = XmlDocument(xmlStr).getDocumentElement();

        if (elt)
        {
            getKeyMappings()->restoreFromXml(*elt);
        }
    }
    else
    {
        p.settingsTree.appendChild(ValueTree("Keymap"), nullptr);
    }

    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, findColour(PlugDataColour::toolbarColourId));
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, findColour(PlugDataColour::toolbarColourId));
    tabbar.setColour(TabbedComponent::outlineColourId, findColour(PlugDataColour::toolbarColourId));

    addAndMakeVisible(statusbar);

    tabbar.onTabChange = [this](int idx)
    {
        if (idx == -1) return;

        // update GraphOnParent when changing tabs
        for (auto* box : getCurrentCanvas()->boxes)
        {
            if (box->graphics && box->graphics->getGui().getType() == pd::Type::GraphOnParent)
            {
                auto* cnv = box->graphics->getCanvas();
                if (cnv) cnv->synchronise();
            }
            if (box->graphics && (box->graphics->getGui().getType() == pd::Type::Subpatch || box->graphics->getGui().getType() == pd::Type::GraphOnParent))
            {
                box->updatePorts();
            }
        }

        auto* cnv = getCurrentCanvas();
        if (cnv->patch.getPointer())
        {
            cnv->patch.setCurrent();
        }

        cnv->synchronise();
        updateValues();
    };

    tabbar.setOutline(0);
    addAndMakeVisible(tabbar);
    addAndMakeVisible(sidebar);

    for (auto* button : toolbarButtons)
    {
        button->setName("toolbar:button");
        button->setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // New button
    toolbarButtons[0]->setTooltip("New Project");
    toolbarButtons[0]->onClick = [this]()
    {
        auto* patch = pd.loadPatch(pd::Instance::defaultPatch);
        patch->setTitle("Untitled Patcher");

    };

    // Open button
    toolbarButton(Open)->setTooltip("Open Project");
    toolbarButton(Open)->onClick = [this]() { openProject(); };

    // Save button
    toolbarButton(Save)->setTooltip("Save Project");
    toolbarButton(Save)->onClick = [this]() { saveProject(); };

    // Save Ad button
    toolbarButton(SaveAs)->setTooltip("Save Project as");
    toolbarButton(SaveAs)->onClick = [this]() { saveProjectAs(); };

    //  Undo button
    toolbarButton(Undo)->setTooltip("Undo");
    toolbarButton(Undo)->onClick = [this]() { getCurrentCanvas()->undo(); };

    // Redo button
    toolbarButton(Redo)->setTooltip("Redo");
    toolbarButton(Redo)->onClick = [this]() { getCurrentCanvas()->redo(); };

    // New object button
    toolbarButton(Add)->setTooltip("Create Object");
    toolbarButton(Add)->onClick = [this]() { showNewObjectMenu(); };

    // Show settings
    toolbarButton(Settings)->setTooltip("Settings");
    toolbarButton(Settings)->onClick = [this]()
    {
        
#ifdef PLUGDATA_STANDALONE
        // Initialise settings dialog for DAW and standalone
        auto pluginHolder = StandalonePluginHolder::getInstance();

        settingsDialog = Dialogs::createSettingsDialog(pd, &pluginHolder->deviceManager, pd.settingsTree);
#else
        settingsDialog = Dialogs::createSettingsDialog(pd, nullptr, pd.settingsTree);
#endif
        // Add on top of everything
        // To make sure it is above the top-level close button
        getTopLevelComponent()->addAndMakeVisible(settingsDialog);
        settingsDialog->setBounds(getLocalBounds().withSizeKeepingCentre(650, 500));
        settingsDialog->toFront(false);
        settingsDialog->resized();
    };

    // Hide sidebar
    toolbarButton(Hide)->setTooltip("Hide Sidebar");
    toolbarButton(Hide)->setName("toolbar:hide");
    toolbarButton(Hide)->setClickingTogglesState(true);
    toolbarButton(Hide)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Hide)->setConnectedEdges(12);
    toolbarButton(Hide)->onClick = [this]()
    {
        bool show = !toolbarButton(Hide)->getToggleState();
        sidebar.showSidebar(show);
        toolbarButton(Hide)->setButtonText(show ? Icons::Hide : Icons::Show);

        toolbarButton(Pin)->setVisible(show);

        repaint();
        resized();
    };

    // Hide pin sidebar panel
    toolbarButton(Pin)->setTooltip("Pin Panel");
    toolbarButton(Pin)->setName("toolbar:pin");
    toolbarButton(Pin)->setClickingTogglesState(true);
    toolbarButton(Pin)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Pin)->setConnectedEdges(12);
    toolbarButton(Pin)->onClick = [this]() { sidebar.pinSidebar(toolbarButton(Pin)->getToggleState());
        
    };

    addAndMakeVisible(toolbarButton(Hide));
    
    setSize(pd.lastUIWidth, pd.lastUIHeight);

    tabbar.toFront(false);
    sidebar.toFront(false);


}
PlugDataPluginEditor::~PlugDataPluginEditor()
{
    auto keymap = pd.settingsTree.getChildWithName("Keymap");
    if (keymap.isValid())
    {
        keymap.setProperty("keyxml", getKeyMappings()->createXml(true)->toString(), nullptr);
    }
    
    if(settingsDialog) delete settingsDialog;
    
    pd.settingsTree.removeListener(this);

    removeKeyListener(&statusbar);
    pd.locked.removeListener(this);
    pd.zoomScale.removeListener(this);
}

void PlugDataPluginEditor::showNewObjectMenu()
{


    Dialogs::showObjectMenu(this, toolbarButton(Add));
}

void PlugDataPluginEditor::paint(Graphics& g)
{
    auto baseColour = findColour(PlugDataColour::toolbarColourId);
    
    // TODO: fix this by never having gaps around the canvas!!
    g.setColour(findColour(PlugDataColour::canvasColourId));
    g.fillRect(getLocalBounds().reduced(0, 10));
    
#if PLUGDATA_ROUNDED
    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 10, getWidth(), toolbarHeight - 9);
    g.fillRoundedRectangle(0, 0, getWidth(), toolbarHeight, 6.0f);
    
    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight() - 10);
    g.fillRoundedRectangle(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight(), 6.0f);
#else
    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight());
#endif
}

void PlugDataPluginEditor::paintOverChildren(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0, toolbarHeight + 1, static_cast<float>(getWidth()), toolbarHeight + 1);
    g.drawLine(0.0f, getHeight() - statusbar.getHeight(), static_cast<float>(getWidth()), getHeight() - statusbar.getHeight());
}
    
void PlugDataPluginEditor::resized()
{
    tabbar.setBounds(0, toolbarHeight, (getWidth() - sidebar.getWidth()) + 1, getHeight() - toolbarHeight - statusbar.getHeight());

    sidebar.setBounds(getWidth() - sidebar.getWidth(), toolbarHeight + 1, sidebar.getWidth(), getHeight() - toolbarHeight);

    statusbar.setBounds(0, getHeight() - statusbar.getHeight(), getWidth() - sidebar.getWidth(), statusbar.getHeight());
    

    FlexBox fb;
    fb.flexWrap = FlexBox::Wrap::noWrap;
    fb.justifyContent = FlexBox::JustifyContent::flexStart;
    fb.alignContent = FlexBox::AlignContent::flexStart;
    fb.flexDirection = FlexBox::Direction::row;

    for (int b = 0; b < 9; b++)
    {
        auto* button = toolbarButtons[b];

        auto item = FlexItem(*button).withMinWidth(50.0f).withMinHeight(8.0f).withMaxWidth(80.0f);
        item.flexGrow = 1.0f;
        item.flexShrink = 1.0f;

        if (b == 4 || b == 6)
        {
            auto separator = FlexItem(seperators[b == 4]).withMinWidth(1.0f).withMaxWidth(12.0f);
            separator.flexGrow = 1.0f;
            separator.flexShrink = 1.0f;
            fb.items.add(separator);
        }

        fb.items.add(item);
    }

    Rectangle<float> toolbarBounds = {5.0f, 0.0f, getWidth() - sidebar.getWidth() + 60.0f, static_cast<float>(toolbarHeight)};
    if (toolbarButton(Hide)->getToggleState()) toolbarBounds.setWidth(getWidth() - 50.0f);

    fb.performLayout(toolbarBounds);

    // hide when they fall off the screen
    for (int b = 0; b < 8; b++)
    {
        toolbarButtons[b]->setVisible((toolbarButtons[b]->getBounds().getCentreX()) < getWidth() - sidebar.getWidth());
    }
#ifdef PLUGDATA_STANDALONE
    int offset = 150;
#else
    int offset = 80;
#endif
    
    int pinPosition = getWidth() - std::max(sidebar.getWidth() - 40, offset);
    int hidePosition = toolbarButton(Hide)->getToggleState() ? pinPosition : pinPosition - 70;
    
    toolbarButton(Hide)->setBounds(hidePosition, 0, 70, toolbarHeight);
    toolbarButton(Pin)->setBounds(pinPosition, 0, 70, toolbarHeight);

    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();

    if (auto* cnv = getCurrentCanvas())
    {
        cnv->checkBounds();
    }
}

void PlugDataPluginEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
    }
}

void PlugDataPluginEditor::mouseMagnify(const MouseEvent& e, float scrollFactor)
{
    auto* cnv = getCurrentCanvas();
    auto* viewport = getCurrentCanvas()->viewport;

    auto event = e.getEventRelativeTo(viewport);

    auto oldMousePos = cnv->getLocalPoint(this, e.getPosition());

    statusbar.zoom(scrollFactor);
    valueChanged(pd.zoomScale);  // trigger change to make the anchoring work

    auto newMousePos = cnv->getLocalPoint(this, e.getPosition());

    viewport->setViewPosition(viewport->getViewPositionX() + (oldMousePos.x - newMousePos.x), viewport->getViewPositionY() + (oldMousePos.y - newMousePos.y));
}

#ifdef PLUGDATA_STANDALONE
void PlugDataPluginEditor::mouseDown(const MouseEvent& e)
{
    if(e.getPosition().getY() < toolbarHeight)
    {
        auto* window = getTopLevelComponent();
        windowDragger.startDraggingComponent(window, e.getEventRelativeTo(window));
    }
}

void PlugDataPluginEditor::mouseDrag(const MouseEvent& e)
{
    auto* window = getTopLevelComponent();
    windowDragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
}
#endif

void PlugDataPluginEditor::openProject()
{
    auto openFunc = [this](const FileChooser& f)
    {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd"))
        {
            pd.settingsTree.setProperty("LastChooserPath", openedFile.getParentDirectory().getFullPathName(), nullptr);

            pd.loadPatch(openedFile);
        }
    };

    openChooser = std::make_unique<FileChooser>("Choose file to open", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");
    
    openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
}

void PlugDataPluginEditor::saveProjectAs(const std::function<void()>& nestedCallback)
{
    saveChooser = std::make_unique<FileChooser>("Select a save file", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");
    
    saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
                             [this, nestedCallback](const FileChooser& f) mutable
                             {
                                 File result = saveChooser->getResult();

                                 if (result.getFullPathName().isNotEmpty())
                                 {
                                     pd.settingsTree.setProperty("LastChooserPath", result.getParentDirectory().getFullPathName(), nullptr);

                                     result.deleteFile();
                                     
                                     getCurrentCanvas()->patch.savePatch(result);
                                 }

                                 nestedCallback();
                             });
}

void PlugDataPluginEditor::saveProject(const std::function<void()>& nestedCallback)
{
    if (getCurrentCanvas()->patch.getCurrentFile().existsAsFile())
    {
        getCurrentCanvas()->patch.savePatch();
        nestedCallback();
    }
    else
    {
        saveProjectAs(nestedCallback);
    }
}

void PlugDataPluginEditor::updateValues()
{
    auto* cnv = getCurrentCanvas();

    if (!cnv) return;

    for (auto& box : cnv->boxes)
    {
        if (box->graphics && box->isShowing())
        {
            box->graphics->updateValue();
        }
    }

    updateCommandStatus();
}

Canvas* PlugDataPluginEditor::getCurrentCanvas()
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent()))
    {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()))
        {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PlugDataPluginEditor::getCanvas(int idx)
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx)))
    {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent()))
        {
            return cnv;
        }
    }

    return nullptr;
}

void PlugDataPluginEditor::addTab(Canvas* cnv, bool deleteWhenClosed)
{
    tabbar.addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);

    int tabIdx = tabbar.getNumTabs() - 1;
    
    tabbar.setCurrentTabIndex(tabIdx);
    tabbar.setTabBackgroundColour(tabIdx, Colours::transparentBlack);
    
    if (tabbar.getNumTabs() > 1)
    {
        tabbar.getTabbedButtonBar().setVisible(true);
        tabbar.setTabBarDepth(30);
    }
    else
    {
        tabbar.getTabbedButtonBar().setVisible(false);
        tabbar.setTabBarDepth(1);
    }

    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);

    auto* closeButton = new TextButton(Icons::Clear);

    closeButton->onClick = [this, tabButton, deleteWhenClosed]() mutable
    {
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for (int i = 0; i < tabbar.getNumTabs(); i++)
        {
            if (tabbar.getTabbedButtonBar().getTabButton(i) == tabButton)
            {
                idx = i;
                break;
            }
        }

        if (idx == -1) return;
        
        auto deleteFunc = [this, deleteWhenClosed, idx]() mutable
        {
            auto* cnv = getCanvas(idx);
            auto* patch = &cnv->patch;
    
            if (deleteWhenClosed)
            {
                patch->close();
            }
            
            canvases.removeObject(cnv);
            tabbar.removeTab(idx);
            pd.patches.removeObject(patch);
            
            int numTabs = tabbar.getNumTabs();
            tabbar.setCurrentTabIndex(numTabs - 1, true);

            if (numTabs == 1)
            {
                tabbar.getTabbedButtonBar().setVisible(false);
                tabbar.setTabBarDepth(1);
            }
        };

        MessageManager::callAsync([this, deleteFunc, idx]() mutable {
            auto* cnv = getCanvas(idx);
            if (cnv->patch.isDirty())
            {
                Dialogs::showSaveDialog(this, cnv->patch.getTitle(),
                                        [this, deleteFunc](int result) mutable
                                        {
                                            if (result == 2)
                                            {
                                                saveProject([deleteFunc]() mutable { deleteFunc(); });
                                            }
                                            else if (result == 1)
                                            {
                                                deleteFunc();
                                            }
                                        });
            }
            else
            {
                deleteFunc();
            }

            
        });
    };

    closeButton->setName("tab:close");
    closeButton->setColour(TextButton::buttonColourId, Colour());
    closeButton->setColour(TextButton::buttonOnColourId, Colour());
    closeButton->setColour(ComboBox::outlineColourId, Colour());
    closeButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeButton, TabBarButton::beforeText);
    closeButton->setSize(28, 28);

    tabbar.repaint();

    cnv->setVisible(true);
}

void PlugDataPluginEditor::valueChanged(Value& v)
{
    // Update undo state when locking/unlocking
    if (v.refersToSameSourceAs(pd.locked))
    {
        toolbarButton(Add)->setEnabled(pd.locked == var(false));
        updateCommandStatus();
    }
    // Update zoom
    else if (v.refersToSameSourceAs(pd.zoomScale))
    {
        transform = AffineTransform().scaled(static_cast<float>(v.getValue()));
        for (auto& canvas : canvases)
        {
            if (!canvas->isGraph)
            {
                canvas->hideSuggestions();
                canvas->setTransform(transform);
            }
        }
        getCurrentCanvas()->checkBounds();
    }
}

void PlugDataPluginEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    // Save settings to file whenever valuetree state changes
    pd.saveSettings();
}

void PlugDataPluginEditor::updateCommandStatus()
{
    // TODO: Fix threading issue!!
    if (auto* cnv = getCurrentCanvas())
    {
        auto* patchPtr = cnv->patch.getPointer();
        if(!patchPtr) return;
        
        // First on pd's thread, get undo status
        pd.enqueueFunction([this, cnv, patchPtr]() mutable {
            canUndo = libpd_can_undo(patchPtr);
            canRedo = libpd_can_redo(patchPtr);
            
            // Set button enablement on message thread
            MessageManager::callAsync([this](){
                toolbarButton(Redo)->setEnabled(canRedo && pd.locked == var(false));
                toolbarButton(Undo)->setEnabled(canUndo && pd.locked == var(false));
                
                // Application commands need to be updated when undo state changes
                commandStatusChanged();
            });
            
        });
    }
}

ApplicationCommandTarget* PlugDataPluginEditor::getNextCommandTarget()
{
    return this;
}

void PlugDataPluginEditor::getAllCommands(Array<CommandID>& commands)
{
    // Add all command IDs
    for (int n = NewProject; n < NumItems; n++)
    {
        commands.add(n);
    }
}

void PlugDataPluginEditor::getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result)
{
    bool hasBoxSelection = false;
    bool hasSelection = false;
    
    if (auto* cnv = getCurrentCanvas())
    {
        auto selectedBoxes = cnv->getSelectionOfType<Box>();
        auto selectedConnections = cnv->getSelectionOfType<Connection>();

        hasBoxSelection = !selectedBoxes.isEmpty();
        hasSelection = hasBoxSelection || !selectedConnections.isEmpty();
    }

    switch (commandID)
    {
        case CommandIDs::NewProject:
        {
            result.setInfo("New Project", "Create a new project", "General", 0);
            break;
        }
        case CommandIDs::OpenProject:
        {
            result.setInfo("Open Project", "Open a new project", "General", 0);
            break;
        }
        case CommandIDs::SaveProject:
        {
            result.setInfo("Save Project", "Save project at current location", "General", 0);
            result.addDefaultKeypress(83, ModifierKeys::commandModifier);
            break;
        }
        case CommandIDs::SaveProjectAs:
        {
            result.setInfo("Save Project As", "Save project in chosen location", "General", 0);
            result.addDefaultKeypress(83, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        }
        case CommandIDs::Undo:
        {
            result.setInfo("Undo", "Undo action", "General", 0);
            result.addDefaultKeypress(90, ModifierKeys::commandModifier);
            result.setActive(canUndo);

            break;
        }

        case CommandIDs::Redo:
        {
            result.setInfo("Redo", "Redo action", "General", 0);
            result.addDefaultKeypress(90, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(canRedo);
            break;
        }
        case CommandIDs::Lock:
        {
            result.setInfo("Lock", "Lock patch", "Edit", 0);
            result.addDefaultKeypress(69, ModifierKeys::commandModifier);
            break;
        }
        case CommandIDs::ConnectionPathfind:
        {
            result.setInfo("Tidy connection", "Find best path for connection", "Edit", 0);
            result.addDefaultKeypress(89, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            result.setActive(statusbar.connectionStyle == var(true));
            break;
        }
        case CommandIDs::ConnectionStyle:
        {
            result.setInfo("Connection style", "Set connection style", "Edit", 0);

            break;
        }
        case CommandIDs::ZoomIn:
        {
            result.setInfo("Zoom in", "Zoom in", "Edit", 0);
            result.addDefaultKeypress(61, ModifierKeys::commandModifier);
            break;
        }
        case CommandIDs::ZoomOut:
        {
            result.setInfo("Zoom out", "Zoom out", "Edit", 0);
            result.addDefaultKeypress(45, ModifierKeys::commandModifier);
            break;
        }
        case CommandIDs::ZoomNormal:
        {
            result.setInfo("Zoom 100%", "Revert zoom to 100%", "Edit", 0);
            result.addDefaultKeypress(33, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
            break;
        }
        case CommandIDs::Copy:
        {
            result.setInfo("Copy", "Copy", "Edit", 0);
            result.addDefaultKeypress(67, ModifierKeys::commandModifier);
            result.setActive(pd.locked == var(false) && hasBoxSelection);
            break;
        }
        case CommandIDs::Paste:
        {
            result.setInfo("Paste", "Paste", "Edit", 0);
            result.addDefaultKeypress(86, ModifierKeys::commandModifier);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::Cut:
        {
            result.setInfo("Cut", "Cut selection", "Edit", 0);
            result.addDefaultKeypress(88, ModifierKeys::commandModifier);
            result.setActive(pd.locked == var(false) && hasSelection);
            break;
        }
        case CommandIDs::Delete:
        {
            result.setInfo("Delete", "Delete selection", "Edit", 0);
            result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false) && hasSelection);
            break;
        }
        case CommandIDs::Duplicate:
        {
            result.setInfo("Duplicate", "Duplicate selection", "Edit", 0);
            result.addDefaultKeypress(68, ModifierKeys::commandModifier);
            result.setActive(pd.locked == var(false) && hasBoxSelection);
            break;
        }
        case CommandIDs::SelectAll:
        {
            result.setInfo("Select all", "Select all objects and connections", "Edit", 0);
            result.addDefaultKeypress(65, ModifierKeys::commandModifier);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::ShowBrowser:
        {
            result.setInfo("Show Browser", "Open documentation browser panel", "Edit", 0);
            result.addDefaultKeypress(66, ModifierKeys::commandModifier);
            result.setActive(true);
            break;
        }
            
            
        case CommandIDs::NewObject:
        {
            result.setInfo("New Object", "Create new object", "Objects", 0);
            result.addDefaultKeypress(78, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewComment:
        {
            result.setInfo("New Comment", "Create new comment", "Objects", 0);
            result.addDefaultKeypress(67, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewBang:
        {
            result.setInfo("New Bang", "Create new bang", "Objects", 0);
            result.addDefaultKeypress(66, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewMessage:
        {
            result.setInfo("New Message", "Create new message", "Objects", 0);
            result.addDefaultKeypress(77, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewToggle:
        {
            result.setInfo("New Toggle", "Create new toggle", "Objects", 0);
            result.addDefaultKeypress(84, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewNumbox:
        {
            result.setInfo("New Number", "Create new number box", "Objects", 0);
            result.addDefaultKeypress(73, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewFloatAtom:
        {
            result.setInfo("New Floatatom", "Create new floatatom", "Objects", 0);
            result.addDefaultKeypress(70, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewSymbolAtom:
        {
            result.setInfo("New Symbolatom", "Create new symbolatom", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewListAtom:
        {
            result.setInfo("New Listatom", "Create new listatom", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVerticalSlider:
        {
            result.setInfo("New Vertical Slider", "Create new vertical slider", "Objects", 0);
            result.addDefaultKeypress(83, ModifierKeys::noModifiers);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewHorizontalSlider:
        {
            result.setInfo("New Horizontal Slider", "Create new horizontal slider", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVerticalRadio:
        {
            result.setInfo("New Vertical Radio", "Create new vertical radio", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewHorizontalRadio:
        {
            result.setInfo("New Horizontal Radio", "Create new horizontal radio", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewArray:
        {
            result.setInfo("New Array", "Create new array", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewGraphOnParent:
        {
            result.setInfo("New GraphOnParent", "Create new graph on parent", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewCanvas:
        {
            result.setInfo("New Canvas", "Create new canvas object", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewKeyboard:
        {
            result.setInfo("New Keyboard", "Create new keyboard", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        case CommandIDs::NewVUMeter:
        {
            result.setInfo("New VUMeter", "Create new VU meter", "Objects", 0);
            result.setActive(pd.locked == var(false));
            break;
        }
        default:
            break;
    }
}

bool PlugDataPluginEditor::perform(const InvocationInfo& info)
{
    auto* cnv = getCurrentCanvas();

    auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->getMouseXYRelative() - Point<int>(Box::margin, Box::margin));

    switch (info.commandID)
    {
        case CommandIDs::NewProject:
        {
            // could be nicer
            toolbarButtons[0]->triggerClick();
            break;
        }
        case CommandIDs::OpenProject:
        {
            openProject();
            break;
        }
        case CommandIDs::SaveProject:
        {
            saveProject();
            break;
        }
        case CommandIDs::SaveProjectAs:
        {
            saveProjectAs();
            break;
        }
        case CommandIDs::Copy:
        {
            cnv->copySelection();
            return true;
        }
        case CommandIDs::Paste:
        {
            cnv->pasteSelection();
            return true;
        }
        case CommandIDs::Cut:
        {
            cnv->copySelection();
            cnv->removeSelection();
            return true;
        }
        case CommandIDs::Delete:
        {
            cnv->removeSelection();
            return true;
        }
        case CommandIDs::Duplicate:
        {
            cnv->duplicateSelection();
            return true;
        }
        case CommandIDs::SelectAll:
        {
            for (auto* box : cnv->boxes)
            {
                cnv->setSelected(box, true);
            }
            for (auto* con : cnv->connections)
            {
                cnv->setSelected(con, true);
            }
            return true;
        }
        case CommandIDs::ShowBrowser:
        {
            sidebar.showBrowser(!sidebar.isShowingBrowser());
            statusbar.browserButton->setToggleState(sidebar.isShowingBrowser(), dontSendNotification);
        }
        case CommandIDs::Lock:
        {
            statusbar.lockButton->triggerClick();
            return true;
        }
        case CommandIDs::ConnectionPathfind:
        {
            auto* cnv = getCurrentCanvas();

            for (auto* con : cnv->connections)
            {
                if (cnv->isSelected(con))
                {
                    con->findPath();
                    con->updatePath();
                }
            }

            return true;
        }
        case CommandIDs::ZoomIn:
        {
            statusbar.zoom(true);
            return true;
        }
        case CommandIDs::ZoomOut:
        {
            statusbar.zoom(false);
            return true;
        }
        case CommandIDs::ZoomNormal:
        {
            statusbar.defaultZoom();
            return true;
        }
        case CommandIDs::Undo:
        {
            cnv->undo();
            return true;
        }
        case CommandIDs::Redo:
        {
            cnv->redo();
            return true;
        }
        case CommandIDs::NewArray:
        {
            Dialogs::showArrayDialog(this,
                                     [this](int result, const String& name, const String& size)
                                     {
                                         if (result)
                                         {
                                             auto* cnv = getCurrentCanvas();
                                             auto* box = new Box(cnv, "graph " + name + " " + size, cnv->viewport->getViewArea().getCentre());
                                             cnv->boxes.add(box);
                                         }
                                     });
            return true;
        }

            
        default:
        {
            const std::vector<std::string> objectNames = {"", "comment", "bng", "msg", "tgl", "nbx", "vsl", "hsl", "vradio", "hradio", "floatatom", "symbolatom", "listatom", "array", "graph", "cnv", "keyboard", "vu"};
            
            jassert(objectNames.size() == CommandIDs::NumItems - CommandIDs::NewObject);
            
            int idx = static_cast<int>(info.commandID) - CommandIDs::NewObject;
            if(isPositiveAndBelow(idx, objectNames.size())) {
                cnv->boxes.add(new Box(cnv, objectNames[idx], lastPosition));
                return true;
            }
            
            return false;
        }
    }
    
    return false;
}
