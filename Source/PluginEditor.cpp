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
#include "Edge.h"
#include "x_libpd_mod_utils.h"

PlugDataPluginEditor::PlugDataPluginEditor(PlugDataAudioProcessor& p) : AudioProcessorEditor(&p), pd(p), statusbar(p), sidebar(&p)
{
    toolbarButtons = {new TextButton(Icons::New), new TextButton(Icons::Open), new TextButton(Icons::Save), new TextButton(Icons::SaveAs), new TextButton(Icons::Undo), new TextButton(Icons::Redo), new TextButton(Icons::Add), new TextButton(Icons::Settings), new TextButton(Icons::Hide)};

    addKeyListener(&statusbar);

    pd.locked.addListener(this);
    pd.zoomScale.addListener(this);
    pd.settingsTree.getPropertyAsValue("LastChooserPath", nullptr).addListener(this);

    setWantsKeyboardFocus(true);

    tabbar.setColour(TabbedButtonBar::frontOutlineColourId, findColour(ComboBox::backgroundColourId));
    tabbar.setColour(TabbedButtonBar::tabOutlineColourId, findColour(ComboBox::backgroundColourId));
    tabbar.setColour(TabbedComponent::outlineColourId, findColour(ComboBox::backgroundColourId));

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

        cnv->checkBounds();

        updateValues();
    };

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
        auto createFunc = [this]()
        {
            tabbar.clearTabs();
            canvases.clear();
            pd.patches.clear();

            pd.loadPatch(pd::Instance::defaultPatch);
            pd.getPatch().setTitle("Untitled Patcher");
        };

        if (pd.isDirty())
        {
            Dialogs::showSaveDialog(this,
                                    [this, createFunc](int result)
                                    {
                                        if (result == 2)
                                        {
                                            saveProject([createFunc]() mutable { createFunc(); });
                                        }
                                        else if (result == 1)
                                        {
                                            createFunc();
                                        }
                                    });
        }
        else
        {
            createFunc();
        }
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
        // By initialising after the first click we prevent it possibly hanging because audio hasn't started yet
        if (!settingsDialog)
        {
            // Initialise settings dialog for DAW and standalone
            if (pd.wrapperType == AudioPluginInstance::wrapperType_Standalone)
            {
                auto pluginHolder = StandalonePluginHolder::getInstance();

                settingsDialog = Dialogs::createSettingsDialog(pd, &pluginHolder->deviceManager, pd.settingsTree, [this]() { pd.updateSearchPaths(); });
            }
            else
            {
                settingsDialog = Dialogs::createSettingsDialog(pd, nullptr, pd.settingsTree, [this]() { pd.updateSearchPaths(); });
            }
        }

        addChildComponent(settingsDialog.get());

        settingsDialog->setVisible(true);
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
        sidebar.showSidebar(!toolbarButton(Hide)->getToggleState());
        toolbarButton(Hide)->setButtonText(toolbarButton(Hide)->getToggleState() ? Icons::Show : Icons::Hide);

        repaint();
        resized();
    };

    addAndMakeVisible(toolbarButton(Hide));

    // window size limits
    restrainer.setSizeLimits(700, 300, 4000, 4000);
    resizer = std::make_unique<ResizableCornerComponent>(this, &restrainer);
    addAndMakeVisible(resizer.get());

    setSize(pd.lastUIWidth, pd.lastUIHeight);

    saveChooser = std::make_unique<FileChooser>("Select a save file", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");
    openChooser = std::make_unique<FileChooser>("Choose file to open", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");

    tabbar.toFront(false);
}
PlugDataPluginEditor::~PlugDataPluginEditor()
{
    removeKeyListener(&statusbar);
    pd.locked.removeListener(this);
    pd.zoomScale.removeListener(this);
    pd.settingsTree.getPropertyAsValue("LastChooserPath", nullptr).removeListener(this);
}

void PlugDataPluginEditor::showNewObjectMenu()
{
    std::function<void(String)> callback = [this](String result)
    {
        auto* cnv = getCurrentCanvas();

        if (result == "array")
        {
            Dialogs::showArrayDialog(this,
                                     [this](int result, const String& name, const String& size)
                                     {
                                         if (result)
                                         {
                                             auto* cnv = getCurrentCanvas();
                                             auto* box = new Box(cnv, "graph " + name + " " + size, cnv->lastMousePos);
                                             cnv->boxes.add(box);
                                         }
                                     });
        }
        else
        {
            cnv->boxes.add(new Box(cnv, result, cnv->viewport->getViewArea().getCentre()));
        }
    };

    Dialogs::showObjectMenu(this, toolbarButton(Add), callback);
}

void PlugDataPluginEditor::paint(Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    auto baseColour = findColour(ComboBox::backgroundColourId);
    auto highlightColour = findColour(Slider::thumbColourId);

    // Toolbar background
    g.setColour(baseColour);
    g.fillRect(0, 0, getWidth(), toolbarHeight - 4);

    g.setColour(highlightColour);
    g.drawRoundedRectangle({-4.0f, toolbarHeight - 6.0f, static_cast<float>(getWidth() + 9), 20.0f}, 12.0, 4.0);

    // Make sure we cant see the bottom half of the rounded rectangle
    g.setColour(baseColour);
    g.fillRect(0, toolbarHeight - 4, getWidth(), toolbarHeight + 16);

    // Statusbar background
    g.setColour(baseColour);
    g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight());
}

void PlugDataPluginEditor::resized()
{
    int sbarY = toolbarHeight - 4;

    tabbar.setBounds(0, sbarY, getWidth() - sidebar.getWidth(), getHeight() - sbarY - statusbar.getHeight());

    sidebar.setBounds(getWidth() - sidebar.getWidth(), toolbarHeight, sidebar.getWidth(), getHeight() - toolbarHeight);

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

    toolbarButton(Hide)->setBounds(std::min(getWidth() - sidebar.getWidth(), getWidth() - 80), 0, 70, toolbarHeight);

    resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
    resizer->toFront(false);

    pd.lastUIWidth = getWidth();
    pd.lastUIHeight = getHeight();
}

bool PlugDataPluginEditor::keyPressed(const KeyPress& key)
{
    auto* cnv = getCurrentCanvas();

    if (pd.locked == true) return false;

    // Key shortcuts for creating objects
    if (key == KeyPress('n', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('c', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "comment", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('b', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "bng", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('m', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "msg", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('i', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "nbx", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('f', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "floatatom", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('t', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "tgl", cnv->lastMousePos));
        return true;
    }
    if (key == KeyPress('s', ModifierKeys::noModifiers, 0))
    {
        cnv->boxes.add(new Box(cnv, "vsl", cnv->lastMousePos));
        return true;
    }

    if (key.getKeyCode() == KeyPress::backspaceKey)
    {
        cnv->removeSelection();
        return true;
    }

    if (key == KeyPress('a', ModifierKeys::commandModifier, 0))
    {
        for (auto* child : cnv->boxes)
        {
            cnv->setSelected(child, true);
        }
        for (auto* child : cnv->connections)
        {
            child->isSelected = true;
            child->repaint();
        }
        return true;
    }
    // cmd-c
    if (key == KeyPress('c', ModifierKeys::commandModifier, 0))
    {
        cnv->copySelection();
        return true;
    }
    // cmd-v
    if (key == KeyPress('v', ModifierKeys::commandModifier, 0))
    {
        cnv->pasteSelection();
        return true;
    }
    // cmd-x
    if (key == KeyPress('x', ModifierKeys::commandModifier, 0))
    {
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    // cmd-d
    if (key == KeyPress('d', ModifierKeys::commandModifier, 0))
    {
        cnv->duplicateSelection();
        return true;
    }

    // cmd-shift-z
    if (key == KeyPress('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0))
    {
        cnv->redo();
        return true;
    }
    // cmd-z
    if (key == KeyPress('z', ModifierKeys::commandModifier, 0))
    {
        cnv->undo();
        return true;
    }

    return false;
}

void PlugDataPluginEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
   auto mods = ModifierKeys::getCurrentModifiers();
    
    if (e.mods.isCommandDown()) {
       mouseMagnify (e, 1.0f / (1.0f - wheel.deltaY));
    }
}


void PlugDataPluginEditor::mouseMagnify(const MouseEvent& e, float scrollFactor)
{
   auto* cnv = getCurrentCanvas();
   auto* viewport = getCurrentCanvas()->viewport;
    
   auto event = e.getEventRelativeTo(viewport);
    
   //Point<int> anchor (cnv->getLocalPoint (viewport, Point<int> (event.x, event.y)));
    
   auto pos = cnv->getLocalPoint(this, e.getPosition());

   statusbar.zoom(scrollFactor);
   valueChanged(pd.zoomScale); // trigger change to make the anchoring work
    
    auto pos2 = cnv->getLocalPoint(this, e.getPosition());
    
    std::cout << "diffx:" << (pos.x - pos2.x) << "diffy:" << (pos.y - pos2.y) << std::endl;
    
    viewport->setViewPosition(viewport->getViewPositionX() + (pos.x - pos2.x), viewport->getViewPositionY() + (pos.y - pos2.y));
}

void PlugDataPluginEditor::openProject()
{
    auto openFunc = [this](const FileChooser& f)
    {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd"))
        {
            pd.settingsTree.setProperty("LastChooserPath", openedFile.getParentDirectory().getFullPathName(), nullptr);

            tabbar.clearTabs();
            pd.loadPatch(openedFile);
        }
    };

    if (pd.isDirty())
    {
        Dialogs::showSaveDialog(this,
                                [this, openFunc](int result)
                                {
                                    if (result == 2)
                                    {
                                        saveProject([this, openFunc]() mutable { openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc); });
                                    }
                                    else if (result != 0)
                                    {
                                        openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
                                    }
                                });
    }
    else
    {
        openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
    }
}

void PlugDataPluginEditor::saveProjectAs(const std::function<void()>& nestedCallback)
{
    saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
                             [this, nestedCallback](const FileChooser& f) mutable
                             {
                                 File result = saveChooser->getResult();

                                 if (result.getFullPathName().isNotEmpty())
                                 {
                                     pd.settingsTree.setProperty("LastChooserPath", result.getParentDirectory().getFullPathName(), nullptr);

                                     result.deleteFile();

                                     pd.savePatch(result);
                                 }

                                 nestedCallback();
                             });
}

void PlugDataPluginEditor::saveProject(const std::function<void()>& nestedCallback)
{
    if (pd.getCurrentFile().existsAsFile())
    {
        getCurrentCanvas()->pd->savePatch();
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

    updateUndoState();
}

// Called by the hook in pd_inter, on pd's thread!
void PlugDataPluginEditor::updateUndoState()
{
    MessageManager::callAsync([this]() { toolbarButton(Add)->setEnabled(pd.locked == false); });

    if (getCurrentCanvas() && getCurrentCanvas()->patch.getPointer() && pd.locked == false)
    {
        auto* cnv = getCurrentCanvas()->patch.getPointer();

        pd.enqueueFunction(
            [this, cnv]()
            {
                // getCurrentCanvas()->patch.setCurrent();
                //  Check undo state on pd's thread
                bool canUndo = libpd_can_undo(cnv);
                bool canRedo = libpd_can_redo(cnv);

                // Apply to buttons on message thread
                MessageManager::callAsync(
                    [this, canUndo, canRedo]() mutable
                    {
                        toolbarButton(Undo)->setEnabled(canUndo);
                        toolbarButton(Redo)->setEnabled(canRedo);
                    });
            });
    }
    else
    {
        toolbarButton(Undo)->setEnabled(false);
        toolbarButton(Redo)->setEnabled(false);
    }
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
    pd.patches.addIfNotAlreadyThere(cnv->patch);

    tabbar.addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);

    int tabIdx = tabbar.getNumTabs() - 1;
    tabbar.setCurrentTabIndex(tabIdx);

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

        if (tabbar.getCurrentTabIndex() == idx)
        {
            tabbar.setCurrentTabIndex(0, false);
        }

        auto* cnv = getCanvas(idx);

        pd.patches.removeFirstMatchingValue(cnv->patch);

        if (deleteWhenClosed)
        {
            cnv->patch.close();
        }
        canvases.removeObject(cnv);
        tabbar.removeTab(idx);

        tabbar.setCurrentTabIndex(0, true);

        if (tabbar.getNumTabs() == 1)
        {
            tabbar.getTabbedButtonBar().setVisible(false);
            tabbar.setTabBarDepth(1);
        }
    };

    closeButton->setName("tab:close");
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
}

void PlugDataPluginEditor::valueChanged(Value& v)
{
    // Update undo state when locking/unlocking
    if (v.refersToSameSourceAs(pd.locked))
    {
        updateUndoState();
    }
    // Update zoom
    else if (v.refersToSameSourceAs(pd.zoomScale))
    {
        transform = AffineTransform().scaled(static_cast<float>(v.getValue()));
        for (auto& canvas : canvases)
        {
            if (!canvas->isGraph)
            {
                canvas->setTransform(transform);
            }
        }
        getCurrentCanvas()->checkBounds();
    }
    // Update last filechooser path
    else
    {
        saveChooser = std::make_unique<FileChooser>("Select a save file", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");
        openChooser = std::make_unique<FileChooser>("Choose file to open", File(pd.settingsTree.getProperty("LastChooserPath")), "*.pd");
    }
}
