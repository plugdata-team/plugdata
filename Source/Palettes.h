/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <JuceHeader.h>
#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/Dialogs.h"
#include "Iolet.h"
#include "Object.h"
#include "Sidebar/Sidebar.h"
#include "Pd/Instance.h"
#include "Pd/Patch.h"

class PluginEditor;
class PaletteView : public Component
    , public Value::Listener {
    class DraggedComponentGroup : public Component {
    public:
        DraggedComponentGroup(Canvas* canvas, Object* target, Point<int> mouseDownPosition)
            : cnv(canvas)
            , draggedObject(target)
        {
            // Find top level object if we're dragging a graph
            while (auto* nextObject = target->findParentComponentOfClass<Object>()) {
                target = nextObject;
            }

            auto [clipboard, components] = getDraggedArea(target);

            Rectangle<int> totalBounds;
            for (auto* component : components) {
                totalBounds = totalBounds.getUnion(component->getBounds());
            }

            imageToDraw = getObjectsSnapshot(components, totalBounds);

            clipboardContent = clipboard;

            addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses);
            setBounds(cnv->localAreaToGlobal(totalBounds));
            setVisible(true);
            setAlwaysOnTop(true);

            cnv->addMouseListener(this, true);
        }

        ~DraggedComponentGroup()
        {
            cnv->removeMouseListener(this);
        }

        Image getObjectsSnapshot(Array<Component*> components, Rectangle<int> totalBounds)
        {

            std::sort(components.begin(), components.end(), [this](auto const* a, auto const* b) {
                return cnv->getIndexOfChildComponent(a) > cnv->getIndexOfChildComponent(b);
            });

            Image image(Image::ARGB, totalBounds.getWidth(), totalBounds.getHeight(), true);
            Graphics g(image);

            for (auto* component : components) {
                auto b = component->getBounds();

                g.drawImageAt(component->createComponentSnapshot(component->getLocalBounds()), b.getX() - totalBounds.getX(), b.getY() - totalBounds.getY());
            }

            image.multiplyAllAlphas(0.75f);

            return image;
        }

        void pasteObjects(Canvas* target)
        {
            auto position = target->getLocalPoint(nullptr, getScreenPosition()) + Point<int>(Object::margin, Object::margin) - target->canvasOrigin;

            auto result = pd::Patch::translatePatchAsString(clipboardContent, position);

            libpd_paste(target->patch.getPointer(), result.toRawUTF8());
            target->synchronise();
        }

    private:
        std::pair<String, Array<Component*>> getDraggedArea(Object* target)
        {
            cnv->patch.deselectAll();

            std::pair<Array<Object*>, Array<Connection*>> dragged;
            getConnectedObjects(target, dragged);
            auto& [objects, connections] = dragged;

            for (auto* object : objects) {
                cnv->patch.selectObject(object->getPointer());
            }

            int size;
            char const* text = libpd_copy(cnv->patch.getPointer(), &size);
            String clipboard = String::fromUTF8(text, size);

            cnv->patch.deselectAll();

            Array<Component*> components;
            components.addArray(objects);
            components.addArray(connections);

            return { clipboard, components };
        }

        void getConnectedObjects(Object* target, std::pair<Array<Object*>, Array<Connection*>>& result)
        {
            auto& [objects, connections] = result;

            objects.addIfNotAlreadyThere(target);

            for (auto* connection : target->getConnections()) {

                connections.addIfNotAlreadyThere(connection);

                if (target->iolets.contains(connection->inlet) && !objects.contains(connection->outlet->object)) {
                    getConnectedObjects(connection->outlet->object, result);
                } else if (!objects.contains(connection->inlet->object)) {
                    getConnectedObjects(connection->inlet->object, result);
                }
            }
        }

        void paint(Graphics& g) override
        {
            g.drawImageAt(imageToDraw, 0, 0);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            auto relativeEvent = e.getEventRelativeTo(this);

            if (!isDragging) {
                dragger.startDraggingComponent(this, relativeEvent);
                isDragging = true;
            }

            dragger.dragComponent(this, relativeEvent, nullptr);
        }

        void mouseUp(MouseEvent const& e) override
        {
            isDragging = false;
        }

        bool isDragging = false;
        Image imageToDraw;

        Canvas* cnv;
        Object* draggedObject;
        String clipboardContent;
        ComponentDragger dragger;
    };

public:
    PaletteView(PluginEditor* e)
        : editor(e)
        , pd(e->pd)
    {

        editModeButton.setButtonText(Icons::Edit);
        editModeButton.getProperties().set("Style", "LargeIcon");
        editModeButton.setClickingTogglesState(true);
        editModeButton.setRadioGroupId(2222);
        editModeButton.setConnectedEdges(Button::ConnectedOnRight);
        editModeButton.onClick = [this]() {
            cnv->locked = false;
        };
        addAndMakeVisible(editModeButton);

        lockModeButton.setButtonText(Icons::Lock);
        lockModeButton.getProperties().set("Style", "LargeIcon");
        lockModeButton.setClickingTogglesState(true);
        lockModeButton.setRadioGroupId(2222);
        lockModeButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
        lockModeButton.onClick = [this]() {
            cnv->locked = true;
        };
        addAndMakeVisible(lockModeButton);

        dragModeButton.setButtonText(Icons::DragCopyMode);
        dragModeButton.getProperties().set("Style", "LargeIcon");
        dragModeButton.setClickingTogglesState(true);
        dragModeButton.setRadioGroupId(2222);
        dragModeButton.setConnectedEdges(Button::ConnectedOnLeft);
        dragModeButton.onClick = [this]() {
            cnv->locked = true;

            // Make sure that iolets get repainted
            for (auto* object : cnv->objects) {
                for (auto* iolet : object->iolets)
                    reinterpret_cast<Component*>(iolet)->repaint();
            }
        };

        addAndMakeVisible(dragModeButton);

        deleteButton.setButtonText(Icons::Trash);
        deleteButton.getProperties().set("Style", "LargeIcon");
        deleteButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&editor->openedDialog, editor, "Are you sure you want to delete this palette?", [this](bool result) {
                if (result) {
                    deletePalette();
                }
            });
        };
        addAndMakeVisible(deleteButton);

        addAndMakeVisible(patchTitle);
        patchTitle.setEditable(true);
        patchTitle.setColour(Label::outlineWhenEditingColourId, Colours::transparentBlack);
        patchTitle.setJustificationType(Justification::centred);
        patchTitle.onEditorShow = [this]() {
            if (auto* editor = patchTitle.getCurrentTextEditor()) {
                editor->setJustification(Justification::centred);
            }
        };
        patchTitle.onTextChange = [this]() {
            paletteTree.setProperty("Name", patchTitle.getText(), nullptr);
            updatePalettes();
        };

        patchTitle.toBehind(&editModeButton);
    }

    void savePalette()
    {
        if (cnv && paletteTree.isValid()) {
            paletteTree.setProperty("Patch", cnv->patch.getCanvasContent(), nullptr);

            if (paletteTree.isValid()) {
                int lastMode = editModeButton.getToggleState();
                lastMode += lockModeButton.getToggleState() * 2;
                lastMode += dragModeButton.getToggleState() * 3;
                paletteTree.setProperty("Mode", lastMode, nullptr);
            }
        }
    }

    void deletePalette()
    {
        auto parentTree = paletteTree.getParent();
        if (parentTree.isValid()) {
            parentTree.removeChild(paletteTree, nullptr);
            updatePalettes();
        }
    }

    void showPalette(ValueTree palette)
    {
        if (paletteTree.isValid() && paletteTree != palette) {
            int lastMode = editModeButton.getToggleState();
            lastMode += lockModeButton.getToggleState() * 2;
            lastMode += dragModeButton.getToggleState() * 3;
            paletteTree.setProperty("Mode", lastMode, nullptr);
        }

        if (paletteTree == palette && cnv) {
            return;
        }

        paletteTree = palette;

        auto patchText = paletteTree.getProperty("Patch").toString();

        if (patchText.isEmpty())
            patchText = pd::Instance::defaultPatch;

        // Make sure there aren't any properties still open in sidebar
        editor->sidebar->hideParameters();

        auto patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(patchText);

        auto newPatch = pd->openPatch(patchFile); // Don't delete old patch until canvas is replaced!
        cnv = std::make_unique<Canvas>(editor, *newPatch, nullptr, true);
        patch = newPatch;
        viewport.reset(cnv->viewport);

        viewport->setScrollBarsShown(true, false, true, false);

        cnv->paletteDragMode.referTo(dragModeButton.getToggleStateValue());

        addAndMakeVisible(*viewport);

        auto name = paletteTree.getProperty("Name").toString();
        if (name.isNotEmpty()) {
            patchTitle.setText(name, dontSendNotification);
        } else {
            patchTitle.setText("", dontSendNotification);
            patchTitle.showEditor();
        }

        if (paletteTree.hasProperty("Mode")) {
            int mode = paletteTree.getProperty("Mode");
            if (mode == 1) {
                editModeButton.triggerClick();
            } else if (mode == 2) {
                lockModeButton.triggerClick();
            } else if (mode == 3) {
                dragModeButton.triggerClick();
            }
        } else {
            dragModeButton.triggerClick();
        }

        cnv->addMouseListener(this, true);
        cnv->lookAndFeelChanged();
        cnv->setColour(PlugDataColour::canvasBackgroundColourId, Colours::transparentBlack);
        cnv->setColour(PlugDataColour::canvasDotsColourId, Colours::transparentBlack);

        resized();
        repaint();
        cnv->repaint();

        cnv->synchronise();
        cnv->jumpToOrigin();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (dragger || !dragModeButton.getToggleState() || !cnv)
            return;

        auto relativeEvent = e.getEventRelativeTo(cnv.get());

        auto* object = dynamic_cast<Object*>(e.originalComponent);

        if (!object) {
            object = e.originalComponent->findParentComponentOfClass<Object>();
            if (!object) {

                if (auto* cnv = dynamic_cast<Canvas*>(e.originalComponent)) {
                    // TODO: is the order correct? we should prioritise top-level objects!
                    for (auto* obj : cnv->objects) {
                        if (obj->getBounds().contains(relativeEvent.getPosition())) {
                            object = obj;
                            break;
                        }
                    }
                }

                if (!object)
                    return;
            }
        }

        dragger = std::make_unique<DraggedComponentGroup>(cnv.get(), object, relativeEvent.getMouseDownPosition());
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!dragModeButton.getToggleState() || !cnv)
            return;
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!dragger)
            return;

        if (getLocalBounds().contains(e.getEventRelativeTo(this).getPosition())) {
            dragger.reset(nullptr);
            return;
        }

        cnv->removeMouseListener(dragger.get());

        Canvas* targetCanvas = nullptr;
        if (auto* leftCnv = editor->splitView.getLeftTabbar()->getCurrentCanvas()) {
            if (leftCnv->getScreenBounds().contains(e.getScreenPosition())) {
                targetCanvas = leftCnv;
            }
        }
        if (auto* rightCnv = editor->splitView.getRightTabbar()->getCurrentCanvas()) {
            if (rightCnv->getScreenBounds().contains(e.getScreenPosition())) {
                targetCanvas = rightCnv;
            }
        }
        if (targetCanvas)
            dragger->pasteObjects(targetCanvas);

        dragger.reset(nullptr);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(locked)) {
            editModeButton.setToggleState(!getValue<bool>(locked), dontSendNotification);
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(52));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawHorizontalLine(51, 0, getWidth());
    }

    void resized() override
    {
        int panelHeight = 26;

        auto b = getLocalBounds();
        patchTitle.setBounds(b.removeFromTop(panelHeight));

        auto secondPanel = b.removeFromTop(panelHeight).expanded(0, 3);
        auto stateButtonsBounds = secondPanel.withX((getWidth() / 2) - (panelHeight * 1.5f)).withWidth(panelHeight);

        dragModeButton.setBounds(stateButtonsBounds.translated((panelHeight - 1) * 2, 0));
        lockModeButton.setBounds(stateButtonsBounds.translated(panelHeight - 1, 0));
        editModeButton.setBounds(stateButtonsBounds);

        deleteButton.setBounds(secondPanel.removeFromRight(panelHeight + 6).expanded(2, 3));

        if (cnv) {
            cnv->viewport->getPositioner()->applyNewBounds(b);
        }
    }

    Canvas* getCanvas()
    {
        return cnv.get();
    }

    std::function<void()> updatePalettes = []() {};
    std::function<void(ValueTree)> onPaletteChange = [](ValueTree) {};

private:
    Value locked;
    pd::Instance* pd;
    pd::Patch::Ptr patch;
    ValueTree paletteTree;
    PluginEditor* editor;

    std::unique_ptr<DraggedComponentGroup> dragger = nullptr;
    std::unique_ptr<Canvas> cnv;
    std::unique_ptr<Viewport> viewport;

    Label patchTitle;

    TextButton editModeButton;
    TextButton lockModeButton;
    TextButton dragModeButton;

    TextButton deleteButton;

    std::function<StringArray()> getComboOptions = []() { return StringArray(); };
};

class PaletteSelector : public TextButton {

public:
    PaletteSelector(String textToShow)
    {
        setRadioGroupId(1011);
        setButtonText(textToShow);
        setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
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

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::toolbarBackgroundColourId));

        if (getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillRect(getLocalBounds().removeFromRight(4));
        }

        g.saveState();

        auto midX = static_cast<float>(getWidth()) * 0.5f;
        auto midY = static_cast<float>(getHeight()) * 0.5f;

        auto transform = AffineTransform::rotation(-MathConstants<float>::halfPi, midX, midY);
        g.addTransform(transform);

        Font font(getWidth() / 1.7f);

        g.setFont(font);
        auto colour = findColour(getToggleState() ? TextButton::textColourOnId
                                                  : TextButton::textColourOffId)
                          .withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);

        g.setColour(colour);
        g.drawText(getButtonText(), getLocalBounds().reduced(2).transformedBy(transform), Justification::centred, false);

        g.restoreState();
    }

    std::function<void()> rightClicked = []() {};
};

class Palettes : public Component
    , public SettingsFileListener {
public:
    Palettes(PluginEditor* editor)
        : view(editor)
        , resizer(this)
    {

        if (!palettesFile.exists()) {
            palettesFile.create();

            palettesTree = ValueTree("Palettes");

            for (auto [name, patch] : defaultPalettes) {
                ValueTree paletteTree = ValueTree("Palette");
                paletteTree.setProperty("Name", name, nullptr);
                paletteTree.setProperty("Patch", patch, nullptr);
                palettesTree.appendChild(paletteTree, nullptr);
            }

            savePalettes();
        } else {
            palettesTree = ValueTree::fromXml(palettesFile.loadFileAsString());
        }

        addButton.getProperties().set("Style", "SmallIcon");
        addButton.onClick = [this, editor]() {
            PopupMenu menu;
            menu.addItem(1, "New palette");
            menu.addItem(2, "New palette from clipboard");

            PopupMenu defaultPalettesMenu;

            for (auto& [name, patch] : defaultPalettes) {
                defaultPalettesMenu.addItem(name, [this, name = name, patch = patch]() {
                    auto existingTree = palettesTree.getChildWithProperty("Name", name);
                    if (existingTree.isValid()) {
                        view.showPalette(existingTree);
                    } else {
                        ValueTree paletteTree = ValueTree("Palette");
                        paletteTree.setProperty("Name", name, nullptr);
                        paletteTree.setProperty("Patch", patch, nullptr);
                        palettesTree.appendChild(paletteTree, nullptr);

                        updatePalettes();

                        paletteSelectors.getLast()->triggerClick();

                        savePalettes();
                    }
                });
            }

            menu.addSubMenu("Add default palette", defaultPalettesMenu);

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(editor).withTargetComponent(&addButton), ModalCallbackFunction::create([this](int result) {
                if (result > 0) {
                    newPalette(result - 1);
                }
            }));
        };

        updatePalettes();

        paletteBar.setVisible(true);
        paletteViewport.setViewedComponent(&paletteBar, false);
        paletteViewport.setScrollBarsShown(true, true, true, true);
        paletteViewport.setScrollBarThickness(4);
        paletteViewport.setScrollBarPosition(false, false);

        addAndMakeVisible(paletteViewport);
        addAndMakeVisible(view);
        addAndMakeVisible(resizer);

        resizer.setAlwaysOnTop(true);

        paletteBar.addAndMakeVisible(addButton);

        view.onPaletteChange = [this](ValueTree currentPalette) {
            // TODO: clean this up!
            int idx = 0;
            bool found = false;
            for (auto palette : palettesTree) {
                if (palette == currentPalette) {
                    found = true;
                    break;
                }

                idx++;
            }
            if (!found)
                return;

            paletteSelectors[idx]->setToggleState(true, dontSendNotification);
            setViewHidden(false);
            savePalettes();
        };

        view.updatePalettes = [this]() {
            updatePalettes();
        };

        setViewHidden(true);
        setSize(300, 0);

        showPalettes = SettingsFile::getInstance()->getProperty<bool>("show_palettes");
        setVisible(showPalettes);
    }

    ~Palettes()
    {
        savePalettes();
    }

    bool isExpanded()
    {
        return view.isVisible();
    }

    Canvas* getCurrentCanvas()
    {
        return view.getCanvas();
    }

    bool hasFocus()
    {
        auto* cnv = getCurrentCanvas();
        return cnv && (cnv->isShowingMenu || hasKeyboardFocus(true));
    }

private:
    void propertyChanged(String name, var value) override
    {
        if (name == "show_palettes") {
            setVisible(static_cast<bool>(value));
        }
    };

    bool hitTest(int x, int y) override
    {
        if (!view.isVisible()) {
            return x < 26;
        }

        return true;
    }

    void resized() override
    {
        int totalHeight = 0;
        for (auto* button : paletteSelectors) {
            totalHeight += Font(14).getStringWidth(button->getButtonText()) + 26;
        }

        totalHeight += 46;

        totalHeight = std::max(totalHeight, getHeight());

        paletteBar.setBounds(0, 0, 26, totalHeight);
        paletteViewport.setBounds(getLocalBounds().removeFromLeft(26));

        int offset = totalHeight > paletteViewport.getMaximumVisibleHeight() ? -4 : 0;

        totalHeight = 0;
        for (auto* button : paletteSelectors) {
            String buttonText = button->getButtonText();
            int height = Font(14).getStringWidth(buttonText) + 26;

            if (button != draggedTab) {
                button->setBounds(Rectangle<int>(offset, totalHeight, 26, height));
            }

            totalHeight += height;
        }

        addButton.toFront(false);
        addButton.setBounds(Rectangle<int>(offset, totalHeight, 26, 26));
        totalHeight += 25;

        view.setBounds(getLocalBounds().withTrimmedLeft(26));

        resizer.setBounds(getWidth() - 5, 0, 5, getHeight());

        repaint();

        paletteBar.addMouseListener(this, true);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (draggedTab) {
            draggedTab = nullptr;
            savePalettes();
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
                resized();
            } else if (idx < paletteSelectors.size() - 1 && draggedTab->getBounds().getCentreY() > paletteSelectors[idx + 1]->getBounds().getCentreY()) {
                paletteSelectors.swap(idx, idx + 1);
                palettesTree.moveChild(idx, idx + 1, nullptr);
                resized();
            }
        }
    }

    void setViewHidden(bool hidden)
    {
        if (hidden) {
            for (auto* button : paletteSelectors) {
                button->setToggleState(false, dontSendNotification);
            }
        }

        view.setVisible(!hidden);
        resizer.setVisible(!hidden);
        if (auto* parent = getParentComponent())
            parent->resized();
    }

    void paint(Graphics& g) override
    {
        if (view.isVisible()) {
            g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        }

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromLeft(26));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawVerticalLine(25, 0, getHeight());

        if (view.isVisible()) {
            g.drawVerticalLine(getWidth() - 1, 0, getHeight());
        }
    }

    void savePalettes()
    {
        view.savePalette();
        palettesFile.replaceWithText(palettesTree.toXmlString());
    }

    void updatePalettes()
    {
        int lastIdx = -1;
        for (int i = 0; i < paletteSelectors.size(); i++) {
            if (paletteSelectors[i]->getToggleState())
                lastIdx = i;
        }

        paletteSelectors.clear();

        for (auto palette : palettesTree) {
            auto name = palette.getProperty("Name").toString();
            auto button = paletteSelectors.add(new PaletteSelector(name));
            button->onClick = [this, name, button]() {
                if (draggedTab == button)
                    return;

                if (button->getToggleState()) {
                    button->setToggleState(false, dontSendNotification);
                    view.showPalette(ValueTree());
                    setViewHidden(true);
                } else {
                    button->setToggleState(true, dontSendNotification);
                    setViewHidden(false);
                    savePalettes();
                    view.showPalette(palettesTree.getChildWithProperty("Name", name));
                }
            };

            button->rightClicked = [this, palette]() {
                palettesTree.removeChild(palette, nullptr);
                updatePalettes();
            };
            paletteBar.addAndMakeVisible(*button);
        }

        if (isPositiveAndBelow(lastIdx, paletteSelectors.size())) {
            paletteSelectors[lastIdx]->setToggleState(true, dontSendNotification);
        } else if (view.isVisible()) {
            if (!paletteSelectors.size()) {
                setViewHidden(true);
            } else {
                paletteSelectors[std::max(0, lastIdx - 1)]->triggerClick();
            }
        }

        savePalettes();

        resized();
    }

    void newPalette(bool fromClipboard)
    {
        auto patchPrefix = "#N canvas 827 239 527 327 12;\n";
        auto patch = fromClipboard ? patchPrefix + SystemClipboard::getTextFromClipboard() : "";

        ValueTree paletteTree = ValueTree("Palette");
        paletteTree.setProperty("Name", "", nullptr);
        paletteTree.setProperty("Patch", patch, nullptr);
        palettesTree.appendChild(paletteTree, nullptr);

        updatePalettes();

        paletteSelectors.getLast()->triggerClick();

        savePalettes();
    }

    int dragStartWidth = 0;
    bool resizing = false;

    File palettesFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Palettes.xml");

    ValueTree palettesTree;

    PaletteView view;

    Point<int> mouseDownPos;
    PaletteSelector* draggedTab = nullptr;

    Viewport paletteViewport;
    Component paletteBar;

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<PaletteSelector> paletteSelectors;

    static inline const String oscillatorsPatch = "#N canvas 827 239 527 327 12;\n"
                                                  "#X obj 110 125 osc~ 440;\n"
                                                  "#X obj 110 170 phasor~;\n"
                                                  "#X obj 110 214 saw~ 440;\n"
                                                  "#X obj 110 250 saw2~ 440;\n"
                                                  "#X obj 110 285 square~ 440;\n"
                                                  "#X obj 110 320 triangle~ 440;\n";

    static inline const String filtersPatch = "#N canvas 827 239 527 327 12;\n"
                                              "#X obj 110 97 lop~ 100;\n"
                                              "#X obj 110 140 vcf~;\n"
                                              "#X obj 110 184 lores~ 800 0.1;\n"
                                              "#X obj 110 222 svf~ 800 0.1;\n"
                                              "#X obj 110 258 bob~;\n";

    std::map<String, String> defaultPalettes = {
        { "Oscillators", oscillatorsPatch },
        { "Filters", filtersPatch }
    };

    bool showPalettes = false;

    class ResizerComponent : public Component {
    public:
        ResizerComponent(Component* toResize)
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
