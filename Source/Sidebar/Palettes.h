/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/Dialogs.h"
#include "Object.h"
#include "Sidebar/Sidebar.h"
#include "Pd/Instance.h"
#include "Pd/Patch.h"

#include "Components/BouncingViewport.h"

#include "PluginEditor.h"
#include "Sidebar/PaletteItem.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/ZoomableDragAndDropContainer.h"
#include "Utility/CachedStringWidth.h"
#include "Utility/RateReducer.h"
#include "Components/Buttons.h"
#include "Components/ArrowPopupMenu.h"

class AddItemButton final : public Component {

    bool mouseIsOver = false;

public:
    std::function<void()> onClick = [] { };

    void paint(Graphics& g) override
    {
        auto const bounds = getLocalBounds().reduced(5, 2);
        auto textBounds = bounds;
        auto const iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

        auto const colour = findColour(PlugDataColour::sidebarTextColourId);
        if (mouseIsOver) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
        }

        Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
        Fonts::drawText(g, "Add from clipboard", textBounds, colour, 14);
    }

    bool hitTest(int const x, int const y) override
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

class PaletteDraggableList final : public Component
    , public ValueTree::Listener {
public:
    PaletteDraggableList(PluginEditor* e, ValueTree tree)
        : editor(e)
        , paletteTree(tree)
    {
        updateItems();

        paletteTree.addListener(this);

        setSize(1, items.size() * 40 + 40);

        pasteButton.onClick = [this] {
            auto const clipboardText = SystemClipboard::getTextFromClipboard();
            if (!OfflineObjectRenderer::checkIfPatchIsValid(clipboardText)) {
                Dialogs::showMultiChoiceDialog(&editor->openedDialog, editor, "Clipboard contents not valid PD patch", [](int){}, {"Dismiss"});
                return;
            }
            ValueTree itemTree("Item");

            String name;
            if (clipboardText.startsWith("#N canvas")) {
                auto const lines = StringArray::fromLines(clipboardText);
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
            auto const paletteItem = new PaletteItem(editor, this, itemTree);
            addAndMakeVisible(items.add(paletteItem));

            if (gainEditorFocus)
                MessageManager::callAsync([_paletteItem = SafePointer(paletteItem)] {
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
        constexpr auto height = 40;
        auto const itemsBounds = getLocalBounds().withHeight(height);
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

        auto const viewport = findParentComponentOfClass<BouncingViewport>();
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

        for (auto const item : paletteTree) {
            auto const paletteItem = new PaletteItem(editor, this, item);
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
            if (auto const* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
                draggedItem = static_cast<PaletteItem*>(reorderButton->getParentComponent());
                draggedItem->toFront(false);
                mouseDownPos = draggedItem->getPosition();
                draggedItem->reorderButton->setVisible(false);
                draggedItem->deleteButton.setVisible(false);
            }
        } else {
            // autoscroll the viewport when we are close. to. the. edge.
            auto const viewport = findParentComponentOfClass<BouncingViewport>();
            if (viewport->autoScroll(0, viewport->getLocalPoint(nullptr, e.getScreenPosition()).getY(), 0, 5)) {
                beginDragAutoRepeat(20);
            }

            auto const dragPos = mouseDownPos.translated(0, e.getDistanceFromDragStartY());
            auto const autoScrollOffset = Point<int>(0, viewportPosHackY - viewport->getViewPositionY());
            accumulatedOffsetY += autoScrollOffset;
            draggedItem->setTopLeftPosition(dragPos - accumulatedOffsetY);
            viewportPosHackY -= autoScrollOffset.getY();

            int const idx = items.indexOf(draggedItem);
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
        if (isItemShowingMenu || !e.mods.isLeftButtonDown())
            return;

        auto const viewport = findParentComponentOfClass<BouncingViewport>();
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

class PaletteComponent final : public Component {
public:
    PaletteComponent(PluginEditor* e, ValueTree tree)
        : paletteTree(tree)
        , editor(e)
    {
        paletteDraggableList = new PaletteDraggableList(e, tree);

        viewport.setViewedComponent(paletteDraggableList, false);
        viewport.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport);

        auto const title = tree.getPropertyAsValue("Name", nullptr).toString();

        nameLabel.setText(title, dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.setJustificationType(Justification::centred);

        nameLabel.onEditorShow = [this] {
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

    ~PaletteComponent() override
    {
        delete paletteDraggableList;
    }

    void lookAndFeelChanged() override
    {
        nameLabel.setFont(Fonts::getBoldFont());
    }

    void showAndGrabEditorFocus()
    {
        MessageManager::callAsync([_this = SafePointer(this)] {
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

class PaletteSelector final : public TextButton {

public:
    PaletteSelector(String const& textToShow, ValueTree palette)
        : palette(palette)
    {
        setRadioGroupId(hash("palette"));
        setButtonText(textToShow);
        setSize(30, CachedStringWidth<14>::calculateStringWidth(textToShow) + 30);
        // setClickingTogglesState(true);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (e.mods.isRightButtonDown()) {
            PopupMenu menu;
            menu.addItem("Export palette", exportClicked);
            menu.addItem("Delete palette", deleteClicked);
            menu.showMenuAsync(PopupMenu::Options());
        }

        TextButton::mouseDown(e);
    }

    void setTextToShow(String const& text)
    {
        setButtonText(text);
        setSize(30, CachedStringWidth<14>::calculateStringWidth(text) + 30);
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

        auto const midX = static_cast<float>(getWidth()) * 0.5f;
        auto const midY = static_cast<float>(getHeight()) * 0.5f;

        auto const transform = AffineTransform::rotation(-MathConstants<float>::halfPi, midX, midY);
        g.addTransform(transform);

        g.setFont(Fonts::getCurrentFont().withHeight(getWidth() * 0.5f));

        auto const colour = findColour(getToggleState() ? TextButton::textColourOnId
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

    std::function<void()> exportClicked = [] { };
    std::function<void()> deleteClicked = [] { };

private:
    ValueTree palette;
};

class Palettes final : public Component
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
            auto const paletteFileContent = palettesFile.loadFileAsString();
            if (paletteFileContent.isEmpty()) {
                initialisePalettesFile();
            } else {
                palettesTree = ValueTree::fromXml(paletteFileContent);

                for (auto paletteCategory : palettesTree) {
                    for (auto [name, palette] : defaultPalettes) {
                        if (name != paletteCategory.getProperty("Name").toString())
                            continue;

                        for (auto& [paletteName, patch] : palette) {
                            auto exitingChildWithName = paletteCategory.getChildWithProperty("Name", paletteName);
                            if (!exitingChildWithName.isValid()) {
                                ValueTree paletteTree("Item");
                                paletteTree.setProperty("Name", paletteName, nullptr);
                                paletteTree.setProperty("Patch", patch, nullptr);
                                paletteCategory.appendChild(paletteTree, nullptr);
                            } else {
                                // TODO: temporary for version transition. Remove after v1 release
                                auto p = exitingChildWithName.getProperty("Patch").toString();
                                p = p.replace("palette/", "else/");
                                exitingChildWithName.setProperty("Patch", p, nullptr);
                            }
                        }
                    }
                }
            }
        }

        palettesTree.addListener(this);

        addButton.onClick = [this] {
            auto menu = new PopupMenu();
            menu->addItem(1, "New palette");
            menu->addItem(2, "Import palette");

            PopupMenu defaultPalettesMenu;

            for (auto& [name, palette] : defaultPalettes) {
                defaultPalettesMenu.addItem(name, [this, name = name, palette = palette] {
                    auto const existingTree = palettesTree.getChildWithProperty("Name", name);
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

            auto* parent = ProjectInfo::canUseSemiTransparentWindows() ? editor->getCalloutAreaComponent() : nullptr;

            ArrowPopupMenu::showMenuAsync(menu, PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withTargetComponent(&addButton).withParentComponent(parent), [this, menu](int const result) {
                if (result == 1) {
                    auto newUntitledPalette = ValueTree("Palette");
                    newUntitledPalette.setProperty("Name", var("Untitled palette"), nullptr);
                    newPalette(newUntitledPalette);
                } else if (result == 2) {
                    Dialogs::showOpenDialog([this](URL const& url) {
                        auto const result = url.getLocalFile();
                        if (!result.exists())
                            return;

                        auto const paletteXml = result.loadFileAsString();
                        auto importedPalette = ValueTree::fromXml(paletteXml);
                        importedPalette.setProperty("Name", var(result.getFileNameWithoutExtension()), nullptr);
                        newPalette(importedPalette);
                    },
                        true, false, "*.pdpalette", "PaletteLocation", getTopLevelComponent());
                }

                MessageManager::callAsync([menu, this] {
                    editor->showCalloutArea(false);
                    delete menu;
                }); }, ArrowPopupMenu::ArrowDirection::LeftRight);

            if (ProjectInfo::canUseSemiTransparentWindows()) {
                editor->showCalloutArea(true);
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

    bool isExpanded() const
    {
        return view.get() && view->isVisible();
    }

    void initialisePalettesFile()
    {
        palettesTree = ValueTree("Palettes");

        for (auto const& [name, palette] : defaultPalettes) {

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
    void settingsChanged(String const& name, var const& value) override
    {
        if (name == "show_palettes") {
            auto const settingsTree = SettingsFile::getInstance()->getValueTree();
            // Check if we have recently opened files
            if (settingsTree.getChildWithName("RecentlyOpened").getNumChildren() || editor->getCurrentCanvas()) {
                if (isVisible() != static_cast<bool>(value)) {
                    setVisible(value);

                    // Update the editor and nvg surface in case we are rendering through image
                    // So that the editor can correctly set the size of the canvas area
                    editor->resized();
                    editor->nvgSurface.resized();
                }
            }
        }
        if (name == "centre_sidepanel_buttons") {
            resized();
        }
    }

    bool hitTest(int const x, int y) override
    {
        if (!isExpanded()) {
            return x < 30;
        }

        return true;
    }

    void lookAndFeelChanged() override
    {
        resized();
    }

    void resized() override
    {
        int totalHeight = 0;
        for (auto const* button : paletteSelectors) {
            totalHeight += button->getHeight();
        }

        totalHeight += 46;

        Rectangle<int> selectorBounds;
        if (totalHeight > getHeight() || !SettingsFile::getInstance()->getProperty<bool>("centre_sidepanel_buttons")) {
            selectorBounds = getLocalBounds().removeFromLeft(30).withTrimmedTop(34);
        } else {
            selectorBounds = getLocalBounds().removeFromLeft(30).withSizeKeepingCentre(30, totalHeight);
        }

        paletteBar.setBounds(0, 0, 30, std::max(totalHeight, getHeight()));

        paletteViewport.setBounds(getLocalBounds().removeFromLeft(30));

        int const offset = totalHeight > paletteViewport.getMaximumVisibleHeight() ? -4 : 0;

        auto& animator = Desktop::getInstance().getAnimator();

        totalHeight = selectorBounds.getY();

        for (auto* button : paletteSelectors) {
            String buttonText = button->getButtonText();
            int const height = button->getHeight();
            
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

            int const idx = paletteSelectors.indexOf(draggedTab);
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
            auto const hasTabbar = editor->getCurrentCanvas() != nullptr;
            auto const lineHeight = hasTabbar ? 30.f : 0.0f;
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
        auto const paletteContent = palettesTree.toXmlString();
        if (paletteContent.isNotEmpty()) {
            palettesFile.replaceWithText(paletteContent);
        }
    }

    void generatePalettes()
    {
        for (auto const palette : palettesTree) {
            newPalette(palette, true);
        }
        resized();
    }

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
    {
        savePalettes();
        if (property == Identifier("Name")) {
            for (auto const paletteSelector : paletteSelectors) {
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

    void newPalette(ValueTree newPaletteTree, bool const construct = false)
    {
        palettesTree.appendChild(newPaletteTree, nullptr);
        auto const title = newPaletteTree.getPropertyAsValue("Name", nullptr).toString();
        auto* button = paletteSelectors.add(new PaletteSelector(title, newPaletteTree));
        button->onClick = [this, button, newPaletteTree] {
            if (button->getToggleState()) {
                showPalette(ValueTree());
            } else {
                button->setToggleState(true, dontSendNotification);
                savePalettes();
                showPalette(newPaletteTree);
            }
        };

        button->exportClicked = [this, newPaletteTree] {
            auto paletteContent = newPaletteTree.toXmlString();

            Dialogs::showSaveDialog([paletteContent](URL const& url) {
                auto result = url.getLocalFile();
                if (result.getParentDirectory().exists()) {
                    result = result.withFileExtension(".pdpalette");
                    result.replaceWithText(paletteContent);
                }
            },
                "*.pdpalette", "PaletteLocation", getTopLevelComponent());
        };

        button->deleteClicked = [this, newPaletteTree] {
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

    File palettesFile = ProjectInfo::appDataDir.getChildFile(".palettes_test_7");
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

    UnorderedMap<String, UnorderedMap<String, String>> defaultPalettes = {
        { "Generators",
            {
                { "vco", "#X obj 0 0 vco.m~" },
                { "lfo", "#X obj 0 0 lfo.m~" },
                { "plaits", "#X obj 0 0 plaits.m~" },
                { "6 operator FM", "#X obj 0 0 pm6.m~" },
                { "signal generator", "#X obj 0 0 sig.m~" },
                { "gendyn osc", "#X obj 0 0 gendyn.m~" },
                { "pluck", "#X obj 0 0 pluck.m~" },
                { "brane", "#X obj 0 0 brane.m~" },
                { "sfont", "#X obj 0 0 sfont.m~" },
                { "8-step sequencer", "#X obj 0 0 seq8.m~" },
                { "adsr", "#X obj 0 0 adsr.m~" },
            } },
        { "Filters",
            {
                { "vcf", "#X obj 0 0 vcf.m~" },
            } },
        { "Effects",
            {
                { "delay", "#X obj 0 0 delay.m~" },
                { "chorus", "#X obj 0 0 chorus.m~" },
                { "phaser", "#X obj 0 0 phaser.m~" },
                { "flanger", "#X obj 0 0 flanger.m~" },
                { "drive", "#X obj 0 0 drive.m~" },
                { "bitcrusher", "#X obj 0 0 crusher.m~" },
                { "reverb", "#X obj 0 0 plate.rev.m~" },
                { "vca", "#X obj 0 0 vca.m~" },
                { "ringmod", "#X obj 0 0 rm.m~" },
                { "drive", "#X obj 0 0 drive.m~" },
            } },
        { "Extra",
            {
                { "presets", "#X obj 0 0 presets.m" },
            } }
    };

    bool showPalettes = false;

    class ResizerComponent final : public Component {
    public:
        explicit ResizerComponent(Component* toResize)
            : target(toResize)
        {
        }

    private:
        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;
            dragStartWidth = target->getWidth();
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if(rateReducer.tooFast()) return;
            
            int newWidth = dragStartWidth + e.getDistanceFromDragStartX();
            newWidth = std::clamp(newWidth, 100, std::max(target->getParentWidth() / 2, 150));

            target->setBounds(0, target->getY(), newWidth, target->getHeight());
            target->getParentComponent()->resized();
        }

        void mouseMove(MouseEvent const& e) override
        {
            bool const resizeCursor = e.getEventRelativeTo(target).getPosition().getX() > target->getWidth() - 5;
            e.originalComponent->setMouseCursor(resizeCursor ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
        }

        void mouseExit(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
        }

        RateReducer rateReducer = RateReducer(45);
        int dragStartWidth = 0;
        Component* target;
    };

    ResizerComponent resizer;
};
