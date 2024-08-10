/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/Dialogs.h"
#include "Iolet.h"
#include "Object.h"
#include "Sidebar/Sidebar.h"
#include "Pd/Instance.h"
#include "Pd/Patch.h"

#include "Components/BouncingViewport.h"

#include "PluginEditor.h"
#include "Sidebar/PaletteItem.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/ZoomableDragAndDropContainer.h"
#include "Components/Buttons.h"
#include "Components/ArrowPopupMenu.h"

class AddItemButton : public Component {

    bool mouseIsOver = false;

public:
    std::function<void()> onClick = []() {};

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(5, 2);
        auto textBounds = bounds;
        auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

        auto colour = findColour(PlugDataColour::sidebarTextColourId);
        if (mouseIsOver) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
        }

        Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
        Fonts::drawText(g, "Add from clipboard", textBounds, colour, 14);
    }

    bool hitTest(int x, int y) override
    {
        if (getLocalBounds().reduced(5, 2).contains(x, y)) {
            return true;
        }
        return false;
    }

    void mouseEnter(MouseEvent const& e) override
    {
        mouseIsOver = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        mouseIsOver = false;
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        onClick();
    }
};

class PaletteDraggableList : public Component
    , public ValueTree::Listener {
public:
    PaletteDraggableList(PluginEditor* e, ValueTree tree)
        : editor(e)
        , paletteTree(tree)
    {
        updateItems();

        paletteTree.addListener(this);

        setSize(1, items.size() * 40 + 40);

        pasteButton.onClick = [this]() {
            auto clipboardText = SystemClipboard::getTextFromClipboard();
            if (!OfflineObjectRenderer::checkIfPatchIsValid(clipboardText)) {
                /*
                // TODO: should we put an alert here? Needs to be themed however
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::NoIcon,
                                       "Clipboard contents not valid PD objects",
                                       "Pasted text: " + clipboardText.substring(0, 200).quoted());
                */
                return;
            }
            ValueTree itemTree("Item");

            String name;
            if (clipboardText.startsWith("#N canvas")) {
                auto lines = StringArray::fromLines(clipboardText);
                for (int i = lines.size() - 1; i >= 0; i--) {
                    if (lines[i].startsWith("#X restore")) {
                        auto tokens = StringArray::fromTokens(lines[i], true);
                        tokens.removeRange(0, 4);
                        name = tokens.joinIntoString(" ").trimCharactersAtEnd(";");
                    }
                }
            }

            // if plugdata didn't assign a name automatically from the copied patch, assign it untitled
            auto gainEditorFocus = false;
            if (name.isEmpty()) {
                name = "Untitled item";
                gainEditorFocus = true;
            }

            itemTree.setProperty("Name", name, nullptr);
            itemTree.setProperty("Patch", clipboardText, nullptr);
            paletteTree.appendChild(itemTree, nullptr);

            // make a new paletteItem,
            auto paletteItem = new PaletteItem(editor, this, itemTree);
            addAndMakeVisible(items.add(paletteItem));

            if (gainEditorFocus)
                MessageManager::callAsync([_paletteItem = SafePointer(paletteItem)]() {
                    if (_paletteItem)
                        _paletteItem->nameLabel.showEditor();
                });

            // pushViewportToBottom = true;
            resized();
        };

        addAndMakeVisible(pasteButton);
    }

    void resized() override
    {
        auto height = 40;
        auto itemsBounds = getLocalBounds().withHeight(height);
        auto totalHeight = 0;

        auto& animator = Desktop::getInstance().getAnimator();

        Rectangle<int> bounds;
        for (auto* item : items) {
            bounds = itemsBounds.withPosition(0, totalHeight);
            if (item != draggedItem) {
                if (shouldAnimate) {
                    animator.animateComponent(item, bounds, 1.0f, 200, false, 3.0f, 0.0f);
                } else {
                    animator.cancelAnimation(item, false);
                    item->setBounds(bounds);
                }
            }
            totalHeight += height;
        }
        bounds = itemsBounds.withPosition(0, totalHeight + 5).withHeight(30);
        pasteButton.setBounds(bounds.reduced(12, 0));
        // we set the bounds to the size of the component, but if we grow larger,
        // we want to make it larger so the viewport can scroll the component
        setBounds(getLocalBounds().withHeight(jmax(getHeight(), totalHeight + 35)));
        shouldAnimate = false;

        auto viewport = findParentComponentOfClass<BouncingViewport>();
        // if (pushViewportToBottom) {
        //     viewport->setViewPositionProportionately(0.0f, 1.0f);
        //     pushViewportToBottom = false;
        // } else
        if (viewport && viewport->getViewPositionY() != viewportPosHackY)
            viewport->setViewPosition(Point<int>(0, viewportPosHackY));
    }

    void updateItems()
    {
        if (isDragging)
            return;

        items.clear();

        for (auto item : paletteTree) {
            auto paletteItem = new PaletteItem(editor, this, item);
            addAndMakeVisible(items.add(paletteItem));
        }

        resized();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (std::abs(e.getDistanceFromDragStart()) < 5 && !isDragging)
            return;

        isDragging = true;

        if (!draggedItem) {
            if (auto* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
                draggedItem = static_cast<PaletteItem*>(reorderButton->getParentComponent());
                draggedItem->toFront(false);
                mouseDownPos = draggedItem->getPosition();
                draggedItem->reorderButton->setVisible(false);
                draggedItem->deleteButton.setVisible(false);
            }
        } else {
            // autoscroll the viewport when we are close. to. the. edge.
            auto viewport = findParentComponentOfClass<BouncingViewport>();
            if (viewport->autoScroll(0, viewport->getLocalPoint(nullptr, e.getScreenPosition()).getY(), 0, 5)) {
                beginDragAutoRepeat(20);
            }

            auto dragPos = mouseDownPos.translated(0, e.getDistanceFromDragStartY());
            auto autoScrollOffset = Point<int>(0, viewportPosHackY - viewport->getViewPositionY());
            accumulatedOffsetY += autoScrollOffset;
            draggedItem->setTopLeftPosition(dragPos - accumulatedOffsetY);
            viewportPosHackY -= autoScrollOffset.getY();

            int idx = items.indexOf(draggedItem);
            if (idx > 0 && draggedItem->getBounds().getCentreY() < items[idx - 1]->getBounds().getCentreY()) {
                items.swap(idx, idx - 1);
                paletteTree.moveChild(idx, idx - 1, nullptr);
                shouldAnimate = true;
                resized();
            } else if (idx < items.size() - 1 && draggedItem->getBounds().getCentreY() > items[idx + 1]->getBounds().getCentreY()) {
                items.swap(idx, idx + 1);
                paletteTree.moveChild(idx, idx + 1, nullptr);
                shouldAnimate = true;
                resized();
            }
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (draggedItem) {
            isDragging = false;
            draggedItem->deleteButton.setVisible(true);
            draggedItem = nullptr;
            shouldAnimate = true;
            resized();
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isItemShowingMenu)
            return;

        auto viewport = findParentComponentOfClass<BouncingViewport>();
        viewportPosHackY = viewport->getViewPositionY();
        accumulatedOffsetY = { 0, 0 };
    }

    AddItemButton pasteButton;

    PluginEditor* editor;
    ValueTree paletteTree;

    OwnedArray<PaletteItem> items;

    SafePointer<PaletteItem> draggedItem;
    Point<int> mouseDownPos;
    bool isDragging = false;
    bool isItemShowingMenu = false;

    bool shouldAnimate = false;

    int viewportPosHackY;
    // bool pushViewportToBottom = false;
    Point<int> accumulatedOffsetY;
};

class PaletteComponent : public Component {
public:
    PaletteComponent(PluginEditor* e, ValueTree tree)
        : paletteTree(tree)
        , editor(e)
    {
        paletteDraggableList = new PaletteDraggableList(e, tree);

        viewport.setViewedComponent(paletteDraggableList, false);
        viewport.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport);

        auto title = tree.getPropertyAsValue("Name", nullptr).toString();

        nameLabel.setText(title, dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.setJustificationType(Justification::centred);

        nameLabel.onEditorShow = [this]() {
            if (auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
                editor->setJustification(Justification::centred);
            }
        };

        nameLabel.onTextChange = [this, tree]() mutable {
            tree.setProperty("Name", nameLabel.getText(), nullptr);
        };

        addAndMakeVisible(nameLabel);
        lookAndFeelChanged();
    }

    ~PaletteComponent()
    {
        delete paletteDraggableList;
    }

    void lookAndFeelChanged() override
    {
        nameLabel.setFont(Fonts::getBoldFont());
    }

    void showAndGrabEditorFocus()
    {
        MessageManager::callAsync([_this = SafePointer(this)]() {
            if (_this)
                _this->nameLabel.showEditor();
        });
    }

    void paint(Graphics& g) override
    {
        // toolbar bar
        auto backgroundColour = findColour(PlugDataColour::toolbarBackgroundColourId);
        if (ProjectInfo::isStandalone && !editor->isActiveWindow()) {
            backgroundColour = backgroundColour.brighter(backgroundColour.getBrightness() / 2.5f);
        }

        g.setColour(backgroundColour);
        g.fillRect(getLocalBounds().toFloat().removeFromTop(30).withTrimmedTop(0.5f));

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 30.0f, getWidth(), 30.0f);
    }

    void resized() override
    {
        paletteDraggableList->setBounds(getLocalBounds().withTrimmedTop(32));
        viewport.setBounds(getLocalBounds().withTrimmedTop(32));

        nameLabel.setBounds(getLocalBounds().removeFromTop(32));
    }

    ValueTree getTree()
    {
        return paletteTree;
    }

private:
    PaletteDraggableList* paletteDraggableList;
    ValueTree paletteTree;
    PluginEditor* editor;
    BouncingViewport viewport;

    Label nameLabel;
};

class PaletteSelector : public TextButton {

public:
    PaletteSelector(String textToShow, ValueTree palette)
        : palette(palette)
    {
        setRadioGroupId(hash("palette"));
        setButtonText(textToShow);
        // setClickingTogglesState(true);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.mods.isRightButtonDown()) {
            PopupMenu menu;
            menu.addItem("Delete palette", rightClicked);
            menu.showMenuAsync(PopupMenu::Options());
        }

        TextButton::mouseDown(e);
    }

    void setTextToShow(String const& text)
    {
        setButtonText(text);
    }

    void lookAndFeelChanged() override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        if (getToggleState() || isMouseOver()) {
            g.setColour(findColour(PlugDataColour::toolbarHoverColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f, 4.0f), Corners::defaultCornerRadius);
        }
        g.saveState();

        auto midX = static_cast<float>(getWidth()) * 0.5f;
        auto midY = static_cast<float>(getHeight()) * 0.5f;

        auto transform = AffineTransform::rotation(-MathConstants<float>::halfPi, midX, midY);
        g.addTransform(transform);

        g.setFont(Fonts::getCurrentFont().withHeight(getWidth() * 0.5f));

        auto colour = findColour(getToggleState() ? TextButton::textColourOnId
                                                  : TextButton::textColourOffId)
                          .withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

        g.setColour(colour);
        g.drawText(getButtonText(), getLocalBounds().reduced(2).transformedBy(transform), Justification::centred, false);

        g.restoreState();
    }

    ValueTree getTree()
    {
        return palette;
    }

    std::function<void()> rightClicked = []() {};

private:
    ValueTree palette;
};

class Palettes : public Component
    , public SettingsFileListener
    , public ValueTree::Listener {
public:
    explicit Palettes(PluginEditor* e)
        : editor(e)
        , resizer(this)
    {
        if (!palettesFile.exists()) {
            palettesFile.create();
            initialisePalettesFile();
        } else {
            auto paletteFileContent = palettesFile.loadFileAsString();
            if (paletteFileContent.isEmpty()) {
                initialisePalettesFile();
            } else {
                palettesTree = ValueTree::fromXml(paletteFileContent);

                for (auto paletteCategory : palettesTree) {
                    for (auto [name, palette] : defaultPalettes) {
                        if (name != static_cast<String>(paletteCategory.getProperty("Name").toString()))
                            continue;

                        for (auto& [paletteName, patch] : palette) {
                            if (!paletteCategory.getChildWithProperty("Name", paletteName).isValid()) {
                                ValueTree paletteTree("Item");
                                paletteTree.setProperty("Name", paletteName, nullptr);
                                paletteTree.setProperty("Patch", patch, nullptr);
                                paletteCategory.appendChild(paletteTree, nullptr);
                            }
                        }
                    }
                }
            }
        }

        palettesTree.addListener(this);

        addButton.onClick = [this]() {
            auto menu = new PopupMenu();
            menu->addItem(1, "New palette");

            PopupMenu defaultPalettesMenu;

            for (auto& [name, palette] : defaultPalettes) {
                defaultPalettesMenu.addItem(name, [this, name = name, palette = palette]() {
                    auto existingTree = palettesTree.getChildWithProperty("Name", name);
                    if (existingTree.isValid()) {
                        showPalette(existingTree);
                    } else {
                        ValueTree categoryTree = ValueTree("Category");
                        categoryTree.setProperty("Name", name, nullptr);

                        for (auto& [paletteName, patch] : palette) {
                            ValueTree paletteTree("Item");
                            paletteTree.setProperty("Name", paletteName, nullptr);
                            paletteTree.setProperty("Patch", patch, nullptr);
                            categoryTree.appendChild(paletteTree, nullptr);
                        }

                        // palettesTree.appendChild(categoryTree, nullptr);
                        newPalette(categoryTree);
                    }
                });
            }

            menu->addSubMenu("Add default palette", defaultPalettesMenu);

            auto* parent = ProjectInfo::canUseSemiTransparentWindows() ? editor->calloutArea.get() : nullptr;

            
            ArrowPopupMenu::showMenuAsync(menu, PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&addButton).withParentComponent(parent), [this, menu](int result) {
                if (result > 0) {
                    auto newUntitledPalette = ValueTree("Palette");
                    newUntitledPalette.setProperty("Name", var("Untitled palette"), nullptr);
                    newPalette(newUntitledPalette);
                }

                MessageManager::callAsync([menu, this]() {
                    editor->calloutArea->removeFromDesktop();
                    delete menu;
                });
            }, ArrowPopupMenu::ArrowDirection::LeftRight);

            if (ProjectInfo::canUseSemiTransparentWindows()) {
                editor->calloutArea->addToDesktop(ComponentPeer::windowIsTemporary);
            }
        };

        paletteBar.setVisible(true);
        paletteViewport.setViewedComponent(&paletteBar, false);
        paletteViewport.setScrollBarsShown(true, false, false, false);
        paletteViewport.setScrollBarThickness(4);
        paletteViewport.setScrollBarPosition(false, false);

        addAndMakeVisible(paletteViewport);
        addAndMakeVisible(view.get());
        addAndMakeVisible(resizer);

        resizer.setAlwaysOnTop(true);

        paletteBar.addAndMakeVisible(addButton);

        setSize(300, 0);

        generatePalettes();

        showPalettes = SettingsFile::getInstance()->getProperty<bool>("show_palettes");
        setVisible(showPalettes);

        showPalette(ValueTree());
    }

    ~Palettes() override
    {
        savePalettes();
    }

    bool isExpanded()
    {
        return (view.get() && view->isVisible());
    }

    void initialisePalettesFile()
    {
        palettesTree = ValueTree("Palettes");

        for (auto [name, palette] : defaultPalettes) {

            ValueTree categoryTree = ValueTree("Category");
            categoryTree.setProperty("Name", name, nullptr);

            for (auto& [paletteName, patch] : palette) {
                ValueTree paletteTree("Item");
                paletteTree.setProperty("Name", paletteName, nullptr);
                paletteTree.setProperty("Patch", patch, nullptr);
                categoryTree.appendChild(paletteTree, nullptr);
            }

            palettesTree.appendChild(categoryTree, nullptr);
        }

        savePalettes();
    }

private:
    void propertyChanged(String const& name, var const& value) override
    {
        if (name == "show_palettes") {
            setVisible(static_cast<bool>(value));
        }
        if (name == "centre_sidepanel_buttons") {
            resized();
        }
    }

    bool hitTest(int x, int y) override
    {
        if (!isExpanded()) {
            return x < 30;
        }

        return true;
    }

    void lookAndFeelChanged() override
    {
        updateGeometry();
    }

    void resized() override
    {
        updateGeometry();
    }

    void updateGeometry()
    {
        int totalHeight = 0;
        for (auto* button : paletteSelectors) {
            totalHeight += CachedStringWidth<14>::calculateStringWidth(button->getButtonText()) + 30;
        }

        totalHeight += 46;

        Rectangle<int> selectorBounds;
        if (totalHeight > getHeight() || !SettingsFile::getInstance()->getProperty<bool>("centre_sidepanel_buttons")) {
            selectorBounds = getLocalBounds().removeFromLeft(30);
        } else {
            selectorBounds = getLocalBounds().removeFromLeft(30).withSizeKeepingCentre(30, totalHeight);
        }

        paletteBar.setBounds(0, 0, 30, std::max(totalHeight, getHeight()));

        paletteViewport.setBounds(getLocalBounds().removeFromLeft(30));

        int offset = totalHeight > paletteViewport.getMaximumVisibleHeight() ? -4 : 0;

        auto& animator = Desktop::getInstance().getAnimator();

        totalHeight = selectorBounds.getY();

        for (auto* button : paletteSelectors) {
            String buttonText = button->getButtonText();
            int height = Fonts::getCurrentFont().withHeight(14).getStringWidth(buttonText) + 30;

            if (button != draggedTab) {
                auto bounds = Rectangle<int>(offset, totalHeight, 30, height);
                if (shouldAnimate) {
                    animator.animateComponent(button, bounds, 1.0f, 200, false, 3.0f, 0.0f);
                } else {
                    animator.cancelAnimation(button, false);
                    button->setBounds(bounds);
                }
            }

            totalHeight += height;
        }

        shouldAnimate = false;

        addButton.toFront(false);
        addButton.setBounds(Rectangle<int>(offset, totalHeight, 30, 30));

        if (view)
            view->setBounds(getLocalBounds().withTrimmedLeft(30));

        resizer.setBounds(getWidth() - 5, 0, 5, getHeight());

        repaint();

        paletteBar.addMouseListener(this, true);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (draggedTab) {
            draggedTab = nullptr;
            shouldAnimate = true;
            resized();
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.getDistanceFromDragStart() < 5)
            return;

        if (!draggedTab) {
            if (auto* paletteSelector = dynamic_cast<PaletteSelector*>(e.originalComponent)) {
                draggedTab = paletteSelector;
                draggedTab->toFront(false);
                mouseDownPos = draggedTab->getPosition();
            }
        } else {
            draggedTab->setTopLeftPosition(mouseDownPos.translated(0, e.getDistanceFromDragStartY()));

            int idx = paletteSelectors.indexOf(draggedTab);
            if (idx > 0 && draggedTab->getBounds().getCentreY() < paletteSelectors[idx - 1]->getBounds().getCentreY()) {
                paletteSelectors.swap(idx, idx - 1);
                palettesTree.moveChild(idx, idx - 1, nullptr);
                shouldAnimate = true;
                resized();
            } else if (idx < paletteSelectors.size() - 1 && draggedTab->getBounds().getCentreY() > paletteSelectors[idx + 1]->getBounds().getCentreY()) {
                paletteSelectors.swap(idx, idx + 1);
                palettesTree.moveChild(idx, idx + 1, nullptr);
                shouldAnimate = true;
                resized();
            }
        }
    }

    void showPalette(ValueTree paletteToShow)
    {
        if (!paletteToShow.isValid()) {
            for (auto* button : paletteSelectors) {
                button->setToggleState(false, dontSendNotification);
            }

            resizer.setVisible(false);
            view.reset(nullptr);
        } else {
            // for (auto* paletteSelector : paletteSelectors) {
            //     if (paletteSelector->getTree() == paletteToShow)
            //         paletteSelector->setVisible(true);
            //         resizer.setVisible(true);
            // }
            view = std::make_unique<PaletteComponent>(editor, paletteToShow);

            // if the user hasn't changed the default title of this palette
            // put the editor into editor mode (for now)
            if (paletteToShow.getPropertyAsValue("Name", nullptr).toString().compare("Untitled palette") == 0) {
                view->showAndGrabEditorFocus();
            }
            addAndMakeVisible(view.get());
            resizer.setVisible(true);
        }

        resized();

        if (auto* parent = getParentComponent())
            parent->resized();
    }

    void paint(Graphics& g) override
    {
        if (view) {
            g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
            g.fillRect(getLocalBounds().toFloat().withTrimmedTop(29.5f));
            g.fillRect(getLocalBounds().toFloat().removeFromLeft(30).withTrimmedTop(29.5f));
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        if (view) {
            auto hasTabbar = editor->getCurrentCanvas() != nullptr;
            auto lineHeight = hasTabbar ? 30.f : 0.0f;
            g.drawLine(0, lineHeight, getWidth(), lineHeight);
            g.drawLine(getWidth() - 0.5f, 29.5f, getWidth() - 0.5f, getHeight());

            g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
            g.drawLine(29.5f, 29.5f, 29.5f, getHeight());
        } else {
            g.drawLine(29.5f, 29.5f, 29.5f, getHeight());
        }
    }

    void savePalettes()
    {
        auto paletteContent = palettesTree.toXmlString();
        if (paletteContent.isNotEmpty()) {
            palettesFile.replaceWithText(paletteContent);
        }
    }

    void generatePalettes()
    {
        for (auto palette : palettesTree) {
            newPalette(palette, true);
        }
        resized();
    }

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
    {
        savePalettes();
        if (property == Identifier("Name")) {
            for (auto paletteSelector : paletteSelectors) {
                if (paletteSelector->getTree() == treeWhosePropertyHasChanged) {
                    paletteSelector->setTextToShow(treeWhosePropertyHasChanged.getPropertyAsValue("Name", nullptr).toString());
                    resized();
                }
            }
        }
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override
    {
        savePalettes();
    }

    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        savePalettes();
    }

    void valueTreeChildOrderChanged(ValueTree&, int, int) override
    {
        savePalettes();
    }

    void valueTreeParentChanged(ValueTree&) override
    {
        savePalettes();
    }

    void newPalette(ValueTree newPaletteTree, bool construct = false)
    {
        palettesTree.appendChild(newPaletteTree, nullptr);
        auto title = newPaletteTree.getPropertyAsValue("Name", nullptr).toString();
        auto* button = paletteSelectors.add(new PaletteSelector(title, newPaletteTree));
        button->onClick = [this, button, newPaletteTree]() {
            if (button->getToggleState()) {
                showPalette(ValueTree());
            } else {
                button->setToggleState(true, dontSendNotification);
                savePalettes();
                showPalette(newPaletteTree);
            }
        };

        button->rightClicked = [this, newPaletteTree]() {
            for (int i = 0; i < paletteSelectors.size(); i++) {
                auto* paletteSelector = paletteSelectors[i];
                if (paletteSelector->getTree() == newPaletteTree) {
                    paletteSelector->setVisible(false);
                    if (i > 0) {
                        showPalette(paletteSelectors[i - 1]->getTree());
                        paletteSelectors[i - 1]->setToggleState(true, dontSendNotification);
                    } else if (i == 0 && paletteSelectors.size() > 1) {
                        showPalette(paletteSelectors[i + 1]->getTree());
                        paletteSelectors[i + 1]->setToggleState(true, dontSendNotification);
                    } else
                        showPalette(ValueTree());
                    paletteSelectors.removeObject(paletteSelector);
                }
            }
            palettesTree.removeChild(newPaletteTree, nullptr);
            resized();
        };
        paletteBar.addAndMakeVisible(button);

        if (!construct) {
            paletteSelectors.getLast()->triggerClick();
            resized();
        }
    }

    PluginEditor* editor;

    File palettesFile = ProjectInfo::appDataDir.getChildFile(".palettes_test_4");
    //    File palettesFile = ProjectInfo::appDataDir.getChildFile(".palettes"); // TODO: move palette location once we have finished all the default palettes

    ValueTree objectTree;
    ValueTree palettesTree;

    std::unique_ptr<PaletteComponent> view = nullptr;

    Point<int> mouseDownPos;
    SafePointer<PaletteSelector> draggedTab = nullptr;

    Viewport paletteViewport;
    Component paletteBar;

    MainToolbarButton addButton = MainToolbarButton(Icons::Add);

    OwnedArray<PaletteSelector> paletteSelectors;

    bool shouldAnimate = false;

    std::map<String, std::map<String, String>> defaultPalettes = {
        { "Oscillators",
            {
                { "vco", "#X obj 0 0 palette/vco.m~" },
                { "lfo", "#X obj 0 0 palette/lfo.m~" },
                { "plaits", "#X obj 0 0 palette/plaits.m~" },
                { "6 operator FM", "#X obj 0 0 palette/pm6.m~" },
                { "signal generator", "#X obj 0 0 palette/sig.m~" },
                { "noise osc", "#X obj 0 0 palette/noiseosc.m~" },
                { "gendyn osc", "#X obj 0 0 palette/gendyn.m~" },
            } },
        { "Filters",
            {
                { "vcf", "#X obj 0 0 palette/vcf.m~" },
                { "svf", "#X obj 0 0 palette/svf.m~" },
                { "EQ", "#X obj 0 0 palette/eq.m~" },
            } },
        { "Effects",
            {
                { "delay", "#X obj 0 0 palette/delay.m~" },
                { "chorus", "#X obj 0 0 palette/chorus.m~" },
                { "phaser", "#X obj 0 0 palette/phaser.m~" },
                { "flanger", "#X obj 0 0 palette/flanger.m~" },
                { "drive", "#X obj 0 0 palette/drive.m~" },
                { "bitcrusher", "#X obj 0 0 palette/bitcrusher.m~" },
                { "reverb", "#X obj 0 0 palette/plate.rev.m~" },
                { "gain", "#X obj 0 0 palette/gain.m~" },
                { "ringmod", "#X obj 0 0 palette/rm.m~" },
                { "drive", "#X obj 0 0 palette/drive.m~" },
            } },
        { "Sequencers",
            {
                { "bpm metronome", "#X obj 0 0 palette/metronome.m~" },
                { "adsr", "#X obj 0 0 palette/adsr.m~" },
                { "drum sequencer", "#X obj 0 0 palette/drumseq.m~" },
                { "note sequencer", "#X obj 0 0 palette/noteseq.m~" },
                { "8-step sequencer", "#X obj 0 0 palette/seq8.m~" },
                { "presets", "#X obj 0 0 palette/presets.m" },
            } },
        { "Instruments",
            {
                { "drums", "#X obj 0 0 palette/drums.m~" },
                { "piano", "#X obj 0 0 palette/piano.m~" },
                { "e-piano", "#X obj 0 0 palette/epiano.m~" },
                { "bass", "#X obj 0 0 palette/bass.m~" },
                { "guitar", "#X obj 0 0 palette/guitar.m~" },
                { "strings", "#X obj 0 0 palette/strings.m~" },
                { "brass", "#X obj 0 0 palette/brass.m~" },
                { "organ", "#X obj 0 0 palette/organ.m~" },
                { "vca", "#X obj 0 0 palette/vca.m~" },
                { "pluck", "#X obj 0 0 palette/pluck.m~" },
                { "brane", "#X obj 0 0 palette/brane.m~" },
            } },
    };

    bool showPalettes = false;

    class ResizerComponent : public Component {
    public:
        explicit ResizerComponent(Component* toResize)
            : target(toResize)
        {
        }

    private:
        void mouseDown(MouseEvent const& e) override
        {
            dragStartWidth = target->getWidth();
        }

        void mouseDrag(MouseEvent const& e) override
        {
            int newWidth = dragStartWidth + e.getDistanceFromDragStartX();
            newWidth = std::clamp(newWidth, 100, std::max(target->getParentWidth() / 2, 150));

            target->setBounds(0, target->getY(), newWidth, target->getHeight());
            target->getParentComponent()->resized();
        }

        void mouseMove(MouseEvent const& e) override
        {
            bool resizeCursor = e.getEventRelativeTo(target).getPosition().getX() > target->getWidth() - 5;
            e.originalComponent->setMouseCursor(resizeCursor ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
        }

        void mouseExit(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
        }

        int dragStartWidth = 0;
        Component* target;
    };

    ResizerComponent resizer;
};
