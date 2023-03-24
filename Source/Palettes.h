/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <JuceHeader.h>
#include "Canvas.h"
#include "Connection.h"
#include "Iolet.h"
#include "Object.h"
#include "Pd/Instance.h"

class PaletteComboBox : public ComboBox
    , public LookAndFeel_V4
    , public Label::Listener {

    class ComboLabel : public Label {
        void textEditorFocusLost(TextEditor& ed) override
        {
            if (!ed.getText().isEmpty()) {
                textEditorTextChanged(ed);
            } else {
                ed.grabKeyboardFocus();
            }
        }
    };

public:
    PaletteComboBox(LookAndFeel* lnf)
    {
        setLookAndFeel(this);
        setEditableText(true);

        LookAndFeel::setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        LookAndFeel::setColour(ComboBox::backgroundColourId, lnf->findColour(ComboBox::backgroundColourId));
        LookAndFeel::setColour(ComboBox::textColourId, lnf->findColour(ComboBox::textColourId));
        LookAndFeel::setColour(ComboBox::textColourId, lnf->findColour(ComboBox::textColourId));
        LookAndFeel::setColour(ComboBox::buttonColourId, lnf->findColour(ComboBox::buttonColourId));
        LookAndFeel::setColour(ComboBox::arrowColourId, lnf->findColour(ComboBox::arrowColourId));
        LookAndFeel::setColour(ComboBox::focusedOutlineColourId, lnf->findColour(ComboBox::focusedOutlineColourId));

        LookAndFeel::setColour(PopupMenu::backgroundColourId, lnf->findColour(PlugDataColour::popupMenuBackgroundColourId).withAlpha(0.99f));
        LookAndFeel::setColour(PopupMenu::textColourId, lnf->findColour(PlugDataColour::popupMenuTextColourId));
        LookAndFeel::setColour(PopupMenu::highlightedBackgroundColourId, lnf->findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
        LookAndFeel::setColour(PopupMenu::highlightedTextColourId, lnf->findColour(PlugDataColour::popupMenuActiveTextColourId));

        LookAndFeel::setColour(Label::textColourId, lnf->findColour(PlugDataColour::sidebarTextColourId));
        LookAndFeel::setColour(Label::textWhenEditingColourId, lnf->findColour(PlugDataColour::sidebarTextColourId));
        // TODO: set label text colour id
    }

    ~PaletteComboBox()
    {
        setLookAndFeel(nullptr);
    }

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override
    {

        auto background = LookAndFeel::findColour(PopupMenu::backgroundColourId);

        if (Desktop::canUseSemiTransparentWindows()) {
            Path shadowPath;
            shadowPath.addRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, height).reduced(10.0f), Corners::defaultCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.2f), 10, { 0, 2 });

            // Add a bit of alpha to disable the opaque flag

            g.setColour(background);

            auto bounds = Rectangle<float>(0, 0, width, height).reduced(7);
            g.fillRoundedRectangle(bounds, Corners::defaultCornerRadius);

            g.setColour(LookAndFeel::findColour(ComboBox::outlineColourId));
            g.drawRoundedRectangle(bounds, Corners::defaultCornerRadius, 1.0f);
        } else {
            auto bounds = Rectangle<float>(0, 0, width, height);

            g.setColour(background);
            g.fillRect(bounds);

            g.setColour(LookAndFeel::findColour(ComboBox::outlineColourId));
            g.drawRect(bounds, 1.0f);
        }
    }

    void drawPopupMenuItem(Graphics& g, Rectangle<int> const& area,
        bool const isSeparator, bool const isActive,
        bool const isHighlighted, bool const isTicked,
        bool const hasSubMenu, String const& text,
        String const& shortcutKeyText,
        Drawable const* icon, Colour const* const textColourToUse) override
    {
        int margin = Desktop::canUseSemiTransparentWindows() ? 9 : 2;

        auto r = area.reduced(margin, 1);

        auto colour = LookAndFeel::findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
        if (isHighlighted && isActive) {
            g.setColour(LookAndFeel::findColour(PopupMenu::highlightedBackgroundColourId));
            g.fillRoundedRectangle(r.toFloat().reduced(4, 0), 4.0f);
            colour = LookAndFeel::findColour(PopupMenu::highlightedTextColourId);
        }

        g.setColour(colour);

        r.reduce(jmin(5, area.getWidth() / 20), 0);
        r.removeFromLeft(8.0f);
        
        auto font = getPopupMenuFont();

        auto maxFontHeight = (float)r.getHeight() / 1.3f;

        if (font.getHeight() > maxFontHeight)
            font.setHeight(maxFontHeight);

        g.setFont(font);

        r.removeFromRight(3);
        Fonts::drawFittedText(g, text, r, colour);
    }

    void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override
    {
        auto cornerSize = 3.0f;
        Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(object.findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

        Rectangle<int> arrowZone(width - 22, 9, 14, height - 18);
        Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 2.0f);
        path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        g.setColour(object.findColour(ComboBox::arrowColourId).withAlpha((object.isEnabled() ? 0.9f : 0.2f)));

        g.strokePath(path, PathStrokeType(2.0f));
    }

    int getPopupMenuBorderSize() override
    {
        if (Desktop::canUseSemiTransparentWindows()) {
            return 10;
        } else {
            return 2;
        }
    };

    Label* createComboBoxTextBox(ComboBox& /*comboBox*/) override
    {
        auto* label = new ComboLabel();
        label->addListener(this);
        label->setEditable(true);
        label->setJustificationType(Justification::centred);
        label->setBorderSize(BorderSize<int>(1, 16, 1, 5));
        return label;
    }

    void labelTextChanged(Label* labelThatHasChanged) override
    {
        onTextChange(labelThatHasChanged->getText());
    }

    void editorShown(Label* label, TextEditor& editor) override
    {
        editor.setJustification(Justification::centred);
    }

    std::function<void(String)> onTextChange = [](String) {};

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaletteComboBox)
};

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

            int minX = 9999999;
            int minY = 9999999;

            int canvasDepth = 0;

            auto isObject = [](StringArray& tokens) {
                return tokens[0] == "#X" && tokens[1] != "connect" && tokens[2].containsOnly("0123456789") && tokens[3].containsOnly("0123456789");
            };

            auto isStartingCanvas = [](StringArray& tokens) {
                return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("0123456789") && tokens[3].containsOnly("0123456789") && tokens[4].containsOnly("0123456789") && tokens[5].containsOnly("0123456789");
            };

            auto isEndingCanvas = [](StringArray& tokens) {
                return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("0123456789") && tokens[3].containsOnly("0123456789");
            };

            for (auto& line : StringArray::fromLines(clipboardContent)) {
                auto tokens = StringArray::fromTokens(line, true);

                if (isStartingCanvas(tokens)) {
                    canvasDepth++;
                }

                if (canvasDepth == 0 && isObject(tokens)) {
                    minX = std::min(minX, tokens[2].getIntValue());
                    minY = std::min(minY, tokens[3].getIntValue());
                }

                if (canvasDepth == 1 && isEndingCanvas(tokens)) {
                    minX = std::min(minX, tokens[2].getIntValue());
                    minY = std::min(minY, tokens[3].getIntValue());
                    canvasDepth--;
                }
            }

            auto toPaste = StringArray::fromLines(clipboardContent);
            for (auto& line : toPaste) {

                auto tokens = StringArray::fromTokens(line, true);
                if (isStartingCanvas(tokens)) {
                    canvasDepth++;
                }

                if (canvasDepth == 0 && isObject(tokens)) {
                    tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                    tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));

                    line = tokens.joinIntoString(" ");
                }

                if (canvasDepth != 0 && isEndingCanvas(tokens)) {
                    tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                    tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));

                    line = tokens.joinIntoString(" ");

                    canvasDepth--;
                }
            }

            auto result = toPaste.joinIntoString("\n");
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
        , patchSelector(&e->getLookAndFeel())
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
            // TODO: make sure gui objects don't respond to mouse clicks!
        };

        addAndMakeVisible(dragModeButton);
        
        deleteButton.setButtonText(Icons::Trash);
        deleteButton.getProperties().set("Style", "LargeIcon");
        deleteButton.onClick = [this]() {
            
        };
        addAndMakeVisible(deleteButton);
        
        addAndMakeVisible(patchSelector);
        patchSelector.setJustificationType(Justification::centred);
        patchSelector.onChange = [this]() {
            setOpenedPatch(patchSelector.getSelectedItemIndex());
        };

        patchSelector.onTextChange = [this](String newText) {
            paletteTree.setProperty("Name", newText, nullptr);
            updatePalettes();
        };

        patchSelector.toBehind(&editModeButton);
    }

    void savePalette()
    {
        if (cnv) {
            paletteTree.setProperty("Patch", cnv->patch.getCanvasContent(), nullptr);

            if (paletteTree.isValid()) {
                int lastMode = editModeButton.getToggleState();
                lastMode += lockModeButton.getToggleState() * 2;
                lastMode += dragModeButton.getToggleState() * 3;
                paletteTree.setProperty("Mode", lastMode, nullptr);
            }
        }
    }

    void updatePatches(StringArray comboOptions)
    {
        patchSelector.clear();
        patchSelector.addItemList(comboOptions, 1);
    }

    void showPalette(ValueTree palette)
    {

        if (paletteTree.isValid()) {
            int lastMode = editModeButton.getToggleState();
            lastMode += lockModeButton.getToggleState() * 2;
            lastMode += dragModeButton.getToggleState() * 3;
            paletteTree.setProperty("Mode", lastMode, nullptr);
        }

        paletteTree = palette;

        auto patchText = paletteTree.getProperty("Patch").toString();

        if (patchText.isEmpty())
            patchText = pd::Instance::defaultPatch;

        auto patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(patchText);

        auto* newPatch = pd->openPatch(patchFile); // Don't delete old patch until canvas is replaced!
        cnv = std::make_unique<Canvas>(editor, *newPatch, nullptr, true);
        patch.reset(newPatch);
        viewport.reset(cnv->viewport);

        cnv->paletteDragMode.referTo(dragModeButton.getToggleStateValue());

        viewport->setScrollBarsShown(true, false, true, false);

        addAndMakeVisible(*viewport);

        auto name = paletteTree.getProperty("Name").toString();
        if (name.isNotEmpty()) {
            patchSelector.setText(name, dontSendNotification);
        } else {
            patchSelector.setText("", dontSendNotification);
            patchSelector.showEditor();
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

        // TODO: fix this!
        // patchSelector.setEditableText(true);

        cnv->addMouseListener(this, true);
        cnv->lookAndFeelChanged();
        cnv->setColour(PlugDataColour::canvasBackgroundColourId, Colours::transparentBlack);
        cnv->setColour(PlugDataColour::canvasDotsColourId, Colours::transparentBlack);

        resized();
        repaint();
        cnv->repaint();
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
        cnv->addMouseListener(dragger.get(), true);
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
            editModeButton.setToggleState(!static_cast<bool>(locked.getValue()), dontSendNotification);
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
        patchSelector.setBounds(b.removeFromTop(panelHeight));
        
        auto secondPanel = b.removeFromTop(panelHeight).expanded(0, 3);
        auto stateButtonsBounds = secondPanel.withX((getWidth() / 2) - (panelHeight * 1.5f)).withWidth(panelHeight);

        dragModeButton.setBounds(stateButtonsBounds.translated((panelHeight - 1) * 2, 0));
        lockModeButton.setBounds(stateButtonsBounds.translated(panelHeight - 1, 0));
        editModeButton.setBounds(stateButtonsBounds);
        
        deleteButton.setBounds(secondPanel.removeFromRight(panelHeight));
        
        if (cnv) {
            cnv->viewport->getPositioner()->applyNewBounds(b);
            cnv->checkBounds();
        }
    }

    Canvas* getCanvas()
    {
        return cnv.get();
    }

    std::function<void()> updatePalettes = []() {};
    std::function<void(int)> setOpenedPatch = [](int) {};

private:
    Value locked;
    pd::Instance* pd;
    std::unique_ptr<pd::Patch> patch;
    ValueTree paletteTree;
    PluginEditor* editor;

    std::unique_ptr<DraggedComponentGroup> dragger = nullptr;
    std::unique_ptr<Canvas> cnv;
    std::unique_ptr<Viewport> viewport;

    PaletteComboBox patchSelector;

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

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::toolbarBackgroundColourId));

        if (getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillRect(getLocalBounds().removeFromBottom(4));
        }

        getLookAndFeel().drawButtonText(g, *this, isMouseOver(), getToggleState());
    }
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
                paletteTree.setProperty("Width", 300, nullptr);
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

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(editor).withTargetComponent(&addButton), ModalCallbackFunction::create([this](int result) {
                if (result > 0) {
                    newPalette(result - 1);
                }
            }));
        };

        updatePalettes();
        addAndMakeVisible(paletteBar);
        addAndMakeVisible(view);
        addAndMakeVisible(resizer);

        resizer.setAlwaysOnTop(true);

        paletteBar.addAndMakeVisible(addButton);

        view.setOpenedPatch = [this](int toOpen) {
            auto palette = palettesTree.getChild(toOpen);

            paletteSelectors[toOpen]->setToggleState(true, dontSendNotification);
            setViewHidden(false);
            savePalettes();
            view.showPalette(palette);
        };

        view.updatePalettes = [this]() {
            updatePalettes();
        };

        setViewHidden(true);
        setSize(300, 0);

        showPalettes = SettingsFile::getInstance()->getProperty<bool>("show_palettes");
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

    void setDefaultVisibility()
    {
        setVisible(showPalettes);
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
        auto barBounds = getLocalBounds().removeFromBottom(26).withWidth(getHeight() + 26);

        auto buttonArea = barBounds.withZeroOrigin();
        for (auto* button : paletteSelectors) {
            String buttonText = button->getButtonText();
            int width = Font(14).getStringWidth(buttonText);
            button->setBounds(buttonArea.removeFromRight(width + 26));
        }

        addButton.setBounds(buttonArea.removeFromRight(26));

        paletteBar.setBounds(barBounds);
        paletteBar.setTransform(AffineTransform::rotation(-MathConstants<float>::halfPi, 26, getHeight()));

        view.setBounds(getLocalBounds().withTrimmedLeft(26));

        resizer.setBounds(getWidth() - 5, 0, 5, getHeight());

        repaint();
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
        paletteSelectors.clear();

        for (auto palette : palettesTree) {
            auto name = palette.getProperty("Name").toString();
            auto button = paletteSelectors.add(new PaletteSelector(name));
            button->onClick = [this, name, button]() {
                if (button->getToggleState()) {
                    button->setToggleState(false, dontSendNotification);
                    setViewHidden(true);
                } else {
                    button->setToggleState(true, dontSendNotification);
                    setViewHidden(false);
                    savePalettes();
                    view.showPalette(palettesTree.getChildWithProperty("Name", name));
                }
            };
            paletteBar.addAndMakeVisible(*button);
        }

        StringArray patches;
        for (auto* selector : paletteSelectors) {
            patches.add(selector->getButtonText());
        }
        view.updatePatches(patches);
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
        paletteTree.setProperty("Width", 300, nullptr);
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
