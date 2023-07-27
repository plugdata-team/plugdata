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

#include "Utility/BouncingViewport.h"

#include "PluginEditor.h"
#include "PaletteItem.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/ZoomableDragAndDropContainer.h"
#include "Utility/HashUtils.h"

class ReorderButton : public TextButton {
public:
    ReorderButton()
        : TextButton() {}

    MouseCursor getMouseCursor() override
    {
        return MouseCursor::DraggingHandCursor;
    }
};

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

            colour = findColour(PlugDataColour::sidebarActiveTextColourId);
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
            auto offlineCnv = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            if (!offlineCnv->checkIfPatchIsValid(clipboardText)) {
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

            //pushViewportToBottom = true;
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
        //if (pushViewportToBottom) {
        //    viewport->setViewPositionProportionately(0.0f, 1.0f);
        //    pushViewportToBottom = false;
        //} else 
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
        if (std::abs(e.getDistanceFromDragStart()) < 5 && !isDragging || isDnD)
            return;

        isDragging = true;

        if (!draggedItem) {
            if (auto* reorderButton = dynamic_cast<ReorderButton*>(e.originalComponent)) {
                draggedItem = static_cast<PaletteItem*>(reorderButton->getParentComponent());
                draggedItem->toFront(false);
                mouseDownPos = draggedItem->getPosition();
                draggedItem->isRepositioning = true;
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
        isPaletteShowingMenu = false;

        if (draggedItem) {
            isDragging = false;
            draggedItem->isRepositioning = false;
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
    bool isDnD = false;
    bool isItemShowingMenu = false;
    bool isPaletteShowingMenu = false;

    bool shouldAnimate = false;

    int viewportPosHackY;
    //bool pushViewportToBottom = false;
    Point<int> accumulatedOffsetY;
};

class ObjectItem : public Component, public SettableTooltipClient {
public:
    ObjectItem(String const& text, String const& tooltip = String(), String const& patch = String())
        : titleText(text)
        , objectPatch(patch)
    {
        setTooltip(tooltip);

        // glyph reflection

        switch(hash(titleText)){
        // default objects
        case hash("GlyphEmpty"):
            glyph = Icons::GlyphEmpty;
            break;
        case hash("GlyphMessage"):
            glyph = Icons::GlyphMessage;
            break;
        case hash("GlyphFloatBox"):
            glyph = Icons::GlyphFloatBox;
            break;
        case hash("GlyphSymbolBox"):
            glyph = Icons::GlyphSymbolBox;
            break;
        case hash("GlyphListBox"):
            glyph = Icons::GlyphListBox;
            break;
        case hash("GlyphComment"):
            glyph = Icons::GlyphComment;
            break;
        // UI objects
        case hash("GlyphBang"):
            glyph = Icons::GlyphBang;
            break;
        case hash("GlyphToggle"):
            glyph = Icons::GlyphToggle;
            break;
        case hash("GlyphButton"):
            glyph = Icons::GlyphButton;
            break;
        case hash("GlyphKnob"):
            glyph = Icons::GlyphKnob;
            break;
        case hash("GlyphNumber"):
            glyph = Icons::GlyphNumber;
            break;
        case hash("GlyphHSlider"):
            glyph = Icons::GlyphHSlider;
            break;
        case hash("GlyphVSlider"):
            glyph = Icons::GlyphVSlider;
            break;
        case hash("GlyphHRadio"):
            glyph = Icons::GlyphHRadio;
            break;
        case hash("GlyphVRadio"):
            glyph = Icons::GlyphVRadio;
            break;
        case hash("GlyphCanvas"):
            glyph = Icons::GlyphCanvas;
            break;
        case hash("GlyphKeyboard"):
            glyph = Icons::GlyphKeyboard;
            break;
        case hash("GlyphVUMeter"):
            glyph = Icons::GlyphVUMeter;
            break;
        case hash("GlyphArray"):
            glyph = Icons::GlyphArray;
            break;
        case hash("GlyphGOP"):
            glyph = Icons::GlyphGOP;
            break;
        case hash("GlyphOscilloscope"):
            glyph = Icons::GlyphOscilloscope;
            break;
        case hash("GlyphFunction"):
            glyph = Icons::GlyphFunction;
            break;
        case hash("GlyphMessbox"):
            glyph = Icons::GlyphMessbox;
            break;
        case hash("GlyphBicoeff"):
            glyph = Icons::GlyphBicoeff;
            break;
        // general
        case hash("GlyphMetro"):
            glyph = Icons::GlyphMetro;
            break;
        case hash("GlyphCounter"):
            glyph = Icons::GlyphCounter;
            break;
        case hash("GlyphSelect"):
            glyph = Icons::GlyphSelect;
            break;
        case hash("GlyphRoute"):
            glyph = Icons::GlyphRoute;
            break;
        case hash("GlyphExpr"):
            glyph = Icons::GlyphExpr;
            break;
        case hash("GlyphLoadbang"):
            glyph = Icons::GlyphLoadbang;
            break;
        case hash("GlyphPack"):
            glyph = Icons::GlyphPack;
            break;
        case hash("GlyphUnpack"):
            glyph = Icons::GlyphUnpack;
            break;
        case hash("GlyphPrint"):
            glyph = Icons::GlyphPrint;
            break;
        case hash("GlyphNetsend"):
            glyph = Icons::GlyphNetsend;
            break;
        case hash("GlyphNetreceive"):
            glyph = Icons::GlyphNetreceive;
            break;
        case hash("GlyphTimer"):
            glyph = Icons::GlyphTimer;
            break;
        case hash("GlyphDelay"):
            glyph = Icons::GlyphDelay;
            break;

        // MIDI:
        case hash("GlyphMidiIn"):
            glyph = Icons::GlyphMidiIn;
            break;
        case hash("GlyphMidiOut"):
            glyph = Icons::GlyphMidiOut;
            break;
        case hash("GlyphNoteIn"):
            glyph = Icons::GlyphNoteIn;
            break;
        case hash("GlyphNoteOut"):
            glyph = Icons::GlyphNoteOut;
            break;
        case hash("GlyphCtlIn"):
            glyph = Icons::GlyphCtlIn;
            break;
        case hash("GlyphCtlOut"):
            glyph = Icons::GlyphCtlOut;
            break;
        case hash("GlyphPgmIn"):
            glyph = Icons::GlyphPgmIn;
            break;
        case hash("GlyphPgmOut"):
            glyph = Icons::GlyphPgmOut;
            break;
        case hash("GlyphSysexIn"):
            glyph = Icons::GlyphSysexIn;
            break;
        case hash("GlyphSysexOut"):
            glyph = Icons::GlyphSysexOut;
            break;
        case hash("GlyphMtof"):
            glyph = Icons::GlyphMtof;
            break;
        case hash("GlyphFtom"):
            glyph = Icons::GlyphFtom;
            break;
        // IO~
        case hash("GlyphAdc"):
            glyph = Icons::GlyphAdc;
            break;
        case hash("GlyphDac"):
            glyph = Icons::GlyphDac;
            break;
        case hash("GlyphOut"):
            glyph = Icons::GlyphOut;
            break;
        case hash("GlyphBlocksize"):
            glyph = Icons::GlyphBlocksize;
            break;
        case hash("GlyphSamplerate"):
            glyph = Icons::GlyphSamplerate;
            break;
        case hash("GlyphSetDsp"):
            glyph = Icons::GlyphSetDsp;
            break;
        // OSC~
        case hash("GlyphOsc"):
            glyph = Icons::GlyphOsc;
            break;
        case hash("GlyphPhasor"):
            glyph = Icons::GlyphPhasor;
            break;
        case hash("GlyphSaw"):
            glyph = Icons::GlyphSaw;
            break;
        case hash("GlyphSaw2"):
            glyph = Icons::GlyphSaw2;
            break;
        case hash("GlyphSquare"):
            glyph = Icons::GlyphSquare;
            break;
        case hash("GlyphTriangle"):
            glyph = Icons::GlyphTriangle;
            break;
        case hash("GlyphImp"):
            glyph = Icons::GlyphImp;
            break;
        case hash("GlyphImp2"):
            glyph = Icons::GlyphImp2;
            break;
        case hash("GlyphWavetable"):
            glyph = Icons::GlyphWavetable;
            break;
        case hash("GlyphPlaits"):
            glyph = Icons::GlyphPlaits;
            break;
        case hash("GlyphOscBL"):
            glyph = Icons::GlyphOscBL;
            break;
        case hash("GlyphSawBL"):
            glyph = Icons::GlyphSawBL;
            break;
        case hash("GlyphSawBL2"):
            glyph = Icons::GlyphSawBL2;
            break;
        case hash("GlyphSquareBL"):
            glyph = Icons::GlyphSquareBL;
            break;
        case hash("GlyphTriBL"):
            glyph = Icons::GlyphTriBL;
            break;
        case hash("GlyphImpBL"):
            glyph = Icons::GlyphImpBL;
            break;
        case hash("GlyphImpBL2"):
            glyph = Icons::GlyphImpBL2;
            break;
        case hash("GlyphWavetableBL"):
            glyph = Icons::GlyphWavetableBL;
            break;
        default:
            glyph = String();
        }
    }

    void paint(Graphics& g) override
    {
        if (isCategory()) {
            Fonts::drawStyledText(g, titleText, getLocalBounds(), findColour(PlugDataColour::panelTextColourId), FontStyle::Semibold, 15, Justification::centred);
        }
        else {
            auto standardColour = findColour(PlugDataColour::textObjectBackgroundColourId);
            auto highlight = findColour(PlugDataColour::toolbarHoverColourId);
            g.setColour(isHovering ? highlight : standardColour);
            g.fillRoundedRectangle(getLocalBounds().reduced(4).toFloat(), Corners::defaultCornerRadius);

            if (glyph.isEmpty()){
                Fonts::drawStyledText(g, titleText, getLocalBounds(), findColour(PlugDataColour::panelTextColourId), FontStyle::Regular, 18, Justification::centred);
            }
            else {
                Fonts::drawIcon(g, glyph, getLocalBounds(), findColour(PlugDataColour::panelTextColourId), 30);
            }
        }
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().reduced(4).contains(x, y);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        if (isCategory())
            return;
        
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        if (isCategory())
            return;

        isHovering = false;
        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (isCategory())
            return;

        auto dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        if (dragContainer->isDragAndDropActive())
            return;

        auto patchWithTheme = substituteThemeColours(objectPatch);

        if (dragImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToTempImage(patchWithTheme);
        }

        Array<var> palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(patchWithTheme));
        dragContainer->startDragging(palettePatchWithOffset, this, dragImage.image, true, nullptr, nullptr, true);
    }

    String substituteThemeColours(String patch)
    {
        auto colourToHex = [this](PlugDataColour colourEnum){
            auto colour = findColour(colourEnum);
            return String("#" + colour.toDisplayString(false));
        };

        auto colourToIEM = [this](PlugDataColour colourEnum){
            auto colour = findColour(colourEnum);
            return String(String(colour.getRed()) + " " + String(colour.getGreen()) + " " + String(colour.getBlue()));
        };

        String colouredObjects = patch;

        colouredObjects = colouredObjects.replace("@bgColour", colourToHex(PlugDataColour::guiObjectBackgroundColourId));
        colouredObjects = colouredObjects.replace("@fgColour", colourToHex(PlugDataColour::canvasTextColourId));
        // TODO: these both are the same, but should be different?
        colouredObjects = colouredObjects.replace("@arcColour", colourToHex(PlugDataColour::guiObjectInternalOutlineColour));
        colouredObjects = colouredObjects.replace("@canvasColour", colourToHex(PlugDataColour::guiObjectInternalOutlineColour));
        // TODO: these both are the same, but should be different?
        colouredObjects = colouredObjects.replace("@textColour", colourToHex(PlugDataColour::toolbarTextColourId));
        colouredObjects = colouredObjects.replace("@labelColour", colourToHex(PlugDataColour::toolbarTextColourId));

        colouredObjects = colouredObjects.replace("@iemBgColour", colourToIEM(PlugDataColour::guiObjectBackgroundColourId));
        colouredObjects = colouredObjects.replace("@iemFgColour", colourToIEM(PlugDataColour::canvasTextColourId));
        colouredObjects = colouredObjects.replace("@iemGridColour", colourToIEM(PlugDataColour::guiObjectInternalOutlineColour));

        return colouredObjects;
    }

    String getTitleText()
    {
        return titleText;
    }

    bool isCategory()
    {
        return objectPatch.isEmpty();
    }

private:
    String titleText;
    String objectPatch;
    String glyph;
    ImageWithOffset dragImage;
    bool isHovering = false;
};

class ObjectList : public Component {
public:
    ObjectList(PluginEditor* e, ValueTree tree)
        : editor(e)
        , objectTree(tree)
    {
        for (int i = 0; i < objectTree.getNumChildren(); i++) {
            auto objectCategory = objectTree.getChild(i);
            String categoryName = objectCategory.getProperty("Name");
            auto button = new ObjectItem(categoryName);
            objectButtons.add(button);
            addAndMakeVisible(button);

            for (int i = 0; i < objectCategory.getNumChildren(); i++) {
                auto objectItem = objectCategory.getChild(i);
                String name = objectItem.getProperty("Name");
                String patch = objectItem.getProperty("Patch");
                String tooltip = objectItem.getProperty("Tooltip");
                auto button = new ObjectItem(name, tooltip, patch);
                objectButtons.add(button);
                addAndMakeVisible(button);
            }
        }
    }

    void resized() override
    {

    }

    Array<ObjectItem*> objectButtons;

private:
    PluginEditor* editor;
    ValueTree objectTree;
};

class ObjectComponent : public Component {
public:
    ObjectComponent(PluginEditor* e, ValueTree tree)
        : editor(e)
        , objectTree(tree)
    {
        objectList = std::make_unique<ObjectList>(e, tree);

        viewport.setViewedComponent(objectList.get(), false);
        viewport.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto width = getBounds().getWidth();

        int column = 0;
        int row = 0;
        int size = 42;
        int maxColumns = width / size;
        int offset = 0;
        //float size = width / static_cast<float>(maxColumns);
        auto list = objectList->objectButtons;
        for (int i = 0; i < list.size(); i++) {
            auto button = list[i];
            if (button->isCategory()) {
                button->setBounds(0, offset, width, 25);
                offset += 25;
                column = 0;
            } else {
                button->setBounds(column * size, offset, size, size);
                column++;
                if (column >= maxColumns || ((i + 1) < list.size() && list[i + 1]->isCategory())) {
                    column = 0;
                    offset += size;
                }
            }
        }

        objectList->setBounds(bounds.withHeight(offset + size));
        viewport.setBounds(bounds);
    }

private:
    PluginEditor* editor;
    ValueTree objectTree;
    std::unique_ptr<ObjectList> objectList;
    BouncingViewport viewport;
};

class PaletteComponent : public Component {
public:
    PaletteComponent(PluginEditor* e, ValueTree tree)
        : editor(e)
        , paletteTree(tree)
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
    };

    ~PaletteComponent()
    {
        delete paletteDraggableList;
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
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
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
    PluginEditor* editor;
    ValueTree paletteTree;
    BouncingViewport viewport;

    Label nameLabel;
};

class PaletteSelector : public TextButton {

public:
    PaletteSelector(String textToShow, ValueTree palette)
        : palette(palette)
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

    void lookAndFeelChanged() override
    {
        setColour(TextButton::textColourOnId, findColour(TextButton::textColourOffId));
    }

    void setTextToShow(String const& text)
    {
        setButtonText(text);
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().toFloat().withTrimmedTop(0.5f));

        if (getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillRect(getLocalBounds().toFloat().withTrimmedTop(0.5f).removeFromRight(4));
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
        } else {
            palettesTree = ValueTree::fromXml(palettesFile.loadFileAsString());
        }

        // build the object selector
        objectTree = ValueTree("Objects");

        for (auto& [name, objectItem] : objectList) {

            ValueTree categoryTree = ValueTree("Category");
            categoryTree.setProperty("Name", name, nullptr);

            for (auto& [name, patch, tooltip] : objectItem) {
                ValueTree objectItem("Item");
                objectItem.setProperty("Name", name, nullptr);
                objectItem.setProperty("Tooltip", tooltip, nullptr);
                if (patch.isEmpty())
                    patch = "#X obj 0 0 " + name;
                objectItem.setProperty("Patch", patch, nullptr);
                categoryTree.appendChild(objectItem, nullptr);
            }

            objectTree.appendChild(categoryTree, nullptr);
        }

        objectView = std::make_unique<ObjectComponent>(editor, objectTree);

        addChildComponent(objectView.get());

        palettesTree.addListener(this);

        objectButton.getProperties().set("Style", "SmallIcon");
        objectButton.setToggleable(true);
        objectButton.onClick = [this]() {
            objectButton.setToggleState(!objectButton.getToggleState(), false);
            showObjects(objectButton.getToggleState());
        };

        addButton.getProperties().set("Style", "SmallIcon");
        addButton.onClick = [this, e]() {
            PopupMenu menu;
            menu.addItem(1, "New palette");

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

                        //palettesTree.appendChild(categoryTree, nullptr);
                        newPalette(categoryTree);
                    }
                });
            }

            menu.addSubMenu("Add default palette", defaultPalettesMenu);

            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(editor).withTargetComponent(&addButton), ModalCallbackFunction::create([this](int result) {
                if (result > 0) {
                    auto newUntitledPalette = ValueTree("Palette");
                    newUntitledPalette.setProperty("Name", var("Untitled palette"), nullptr);
                    newPalette(newUntitledPalette);
                }
            }));
        };

        paletteBar.setVisible(true);
        paletteViewport.setViewedComponent(&paletteBar, false);
        paletteViewport.setScrollBarsShown(true, true, true, true);
        paletteViewport.setScrollBarThickness(4);
        paletteViewport.setScrollBarPosition(false, false);

        addAndMakeVisible(paletteViewport);
        addAndMakeVisible(view.get());
        addAndMakeVisible(resizer);

        resizer.setAlwaysOnTop(true);

        paletteBar.addAndMakeVisible(objectButton);

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
        return (view.get() && view->isVisible()) || objectView->isVisible();
    }

private:
    void propertyChanged(String const& name, var const& value) override
    {
        if (name == "show_palettes") {
            setVisible(static_cast<bool>(value));
        }
    };

    bool hitTest(int x, int y) override
    {
        if (!isExpanded()) {
            return x < 26;
        }

        return true;
    }

    void resized() override
    {
        paletteViewport.setBounds(getLocalBounds());
        objectView->setBounds(getLocalBounds().withTrimmedLeft(26));

        int totalHeight = 0;
        for (auto* button : paletteSelectors) {
            totalHeight += Font(14).getStringWidth(button->getButtonText()) + 26;
        }

        totalHeight += 46;

        totalHeight = std::max(totalHeight, getHeight());

        paletteBar.setBounds(0, 0, 26, totalHeight);
        paletteViewport.setBounds(getLocalBounds().removeFromLeft(26));

        int offset = totalHeight > paletteViewport.getMaximumVisibleHeight() ? -4 : 0;

        auto& animator = Desktop::getInstance().getAnimator();

        totalHeight = 0;

        objectButton.toFront(false);
        objectButton.setBounds(Rectangle<int>(offset, totalHeight, 26, 26));

        totalHeight += 26;

        for (auto* button : paletteSelectors) {
            String buttonText = button->getButtonText();
            int height = Font(14).getStringWidth(buttonText) + 26;

            if (button != draggedTab) {
                auto bounds = Rectangle<int>(offset, totalHeight, 26, height);
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
        addButton.setBounds(Rectangle<int>(offset, totalHeight, 26, 26));

        if (view)
            view->setBounds(getLocalBounds().withTrimmedLeft(26));

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

    void showObjects(bool isShown)
    {
        // sending an empty tree to show palette will hide the palettes
        showPalette(ValueTree());

        objectView->setVisible(isShown);
        resizer.setVisible(isShown);

        resized();

        if (auto* parent = getParentComponent())
            parent->resized();
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
            //for (auto* paletteSelector : paletteSelectors) {
            //    if (paletteSelector->getTree() == paletteToShow)
            //        paletteSelector->setVisible(true);
            //        resizer.setVisible(true);
            //}
            showObjects(false);
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
        if (view || objectView->isVisible()) {
            g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
            g.fillRect(getLocalBounds().toFloat().withTrimmedTop(0.5f));
        }

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().toFloat().removeFromLeft(26).withTrimmedTop(0.5f));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawVerticalLine(25, 0, getHeight());

        if (view || objectView->isVisible()) {
            g.drawLine(getWidth(), 0, getWidth(), getHeight());
        }
    }

    void savePalettes()
    {
        palettesFile.replaceWithText(palettesTree.toXmlString());
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
        if (property == Identifier("Name")){
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
        button->onClick = [this, button, newPaletteTree](){
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
    File palettesFile = ProjectInfo::appDataDir.getChildFile("PaletteBar.xml");

    ValueTree objectTree;
    ValueTree palettesTree;

    std::unique_ptr<PaletteComponent> view = nullptr;

    std::unique_ptr<ObjectComponent> objectView;

    Point<int> mouseDownPos;
    SafePointer<PaletteSelector> draggedTab = nullptr;

    Viewport paletteViewport;
    Component paletteBar;

    TextButton objectButton = TextButton(Icons::Object);

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<PaletteSelector> paletteSelectors;

    bool shouldAnimate = false;

    static inline const String placeholderPatch = "#X obj 72 264 outlet~;\n"
                                                  "#X obj 72 156 inlet;\n";

    std::vector<std::pair<String, std::vector<std::tuple<String, String, String>>>> objectList = {
        { "Default",
            {
                { "GlyphEmpty", "#X obj 0 0", "(ctrl+1) Empty object" },
                { "GlyphMessage", "#X msg 0 0", "(ctrl+2) Message" },
                { "GlyphFloatBox", "#X floatatom 0 0", "(ctrl+3) Float box" },
                { "GlyphSymbolBox", "#X symbolatom 0 0", "Symbol box" },
                { "GlyphListBox", "#X listbox 0 0", "(ctrl+4) List box" },
                { "GlyphComment", "#X text 0 0 comment", "(ctrl+5) Comment" },
                { "GlyphArray", "#N canvas 0 0 450 250 (subpatch) 0;\n#X array array1 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(ctrl+shift+A) Array" },
                { "GlyphGOP", "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 226 1 graph;", "(ctrl+shift+G) Graph on parent" },
                }},
        { "UI",
            {
                { "GlyphNumber", "#X obj 0 0 nbx 4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 @bgColour @fgColour @textColour 0 256", "(ctrl+shift+N) Number box" },
                { "GlyphBang", "#X obj 0 0 bng 25 250 50 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour", "(ctrl+shift+B) Bang" },
                { "GlyphToggle", "#X obj 0 0 tgl 25 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+T) Toggle" },
                { "GlyphButton", "#X obj 0 0 button 25 25 @iemBgColour @iemFgColour 0", "Button" },
                { "GlyphKnob", "#X obj 0 0 knob 50 0 127 0 0 empty empty @bgColour @arcColour @fgColour 1 0 0 0 1 270 0 0;", "Knob" },
                { "GlyphVSlider", "#X obj 0 0 vsl 17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+V) Vertical slider" },
                { "GlyphHSlider", "#X obj 0 0 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+J) Horizontal slider" },
                { "GlyphVRadio", "#X obj 0 0 vradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(ctrl+shift+D) Vertical radio box" },
                { "GlyphHRadio", "#X obj 0 0 hradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(ctrl+shift+I) Horizontal radio box" },
                { "GlyphCanvas", "#X obj 0 0 cnv 15 100 60 empty empty empty 20 12 0 14 @canvasColour @labelColour 0", "(ctrl+shift+C) Canvas" },
                { "GlyphKeyboard", "#X obj 0 0 keyboard 16 80 4 2 0 0 empty empty", "Piano keyboard" },
                { "GlyphVUMeter", "#X obj 0 0 vu 20 120 empty empty -1 -8 0 10 #191919 @labelColour 1 0", "(ctrl+shift+U) VU meter" },
                { "GlyphOscilloscope", "#X obj 0 0 oscope~ 130 130 256 3 128 -1 1 0 0 0 0 @iemFgColour @iemBgColour @iemGridColour 0 empty", "Oscilloscope" },
                { "GlyphFunction", "#X obj 0 0 function 200 100 empty empty 0 1 @iemBgColour @iemFgColour 0 0 0 0 0 1000 0", "Function" },
                { "GlyphMessbox", "#X obj -0 0 messbox 180 60 @iemBgColour @iemFgColour 0 12", "Message box" },
                { "GlyphBicoeff", "#X obj 0 0 bicoeff 450 150 peaking", "Bicoeff generator" },
                }},
        { "General",
            {
                { "GlyphMetro", "#X obj 0 0 metro 500", "Metro" },
                { "GlyphCounter", "#X obj 0 0 counter 0 5", "Counter" },
                { "GlyphSelect", "#X obj 0 0 select", "Select" },
                { "GlyphRoute", "#X obj 0 0 route", "Route" },
                { "GlyphExpr", "#X obj 0 0 expr", "Expr" },
                { "GlyphLoadbang", "#X obj 0 0 loadbang", "Loadbang" },
                { "GlyphPack", "#X obj 0 0 pack", "Pack" },
                { "GlyphUnpack", "#X obj 0 0 unpack", "Unpack" },
                { "GlyphPrint", "#X obj 0 0 print", "Print" },
                { "GlyphNetreceive", "#X obj 0 0 netreceive", "Netreceive" },
                { "GlyphNetsend", "#X obj 0 0 netsend", "Netsend" },
                { "GlyphTimer", "#X obj 0 0 timer", "Timer" },
                { "GlyphDelay", "#X obj 0 0 delay 1 60 permin", "Delay" },
                }},
        { "MIDI",
            {
                { "GlyphMidiIn", "#X obj 0 0 midiin", "MIDI in" },
                { "GlyphMidiOut", "#X obj 0 0 midiout", "MIDI out" },
                { "GlyphNoteIn", "#X obj 0 0 notein", "Note in" },
                { "GlyphNoteOut", "#X obj 0 0 noteout", "Note out" },
                { "GlyphCtlIn", "#X obj 0 0 ctlin", "Control in" },
                { "GlyphCtlOut", "#X obj 0 0 ctlout", "Control out" },
                { "GlyphPgmIn", "#X obj 0 0 pgmin", "Program in" },
                { "GlyphPgmOut", "#X obj 0 0 pgmout", "Program out" },
                { "GlyphSysexIn", "#X obj 0 0 sysexin", "Sysex in" },
                { "GlyphSysexOut", "#X obj 0 0 sysexout", "Sysex out" },
                { "GlyphMtof", "#X obj 0 0 mtof", "MIDI to frequency" },
                { "GlyphFtom", "#X obj 0 0 ftom", "Frequency to MIDI" },
                }},
        { "IO~",
            {
                { "GlyphAdc", "#X obj 0 0 adc~", "Adc" },
                { "GlyphDac", "#X obj 0 0 dac~", "Dac" },
                { "GlyphOut", "#X obj 0 0 out~", "Out" },
                { "GlyphBlocksize", "#X obj 0 0 blocksize~", "Blocksize" },
                { "GlyphSamplerate", "#X obj 0 0 samplerate~", "Samplerate" },
                { "GlyphSetDsp", "#X obj 0 0 setdsp~", "Setdsp" },
                }},
        { "Oscillators~",
            {
                { "GlyphOsc", "#X obj 0 0 osc~ 440", "Osc" },
                { "GlyphPhasor", "#X obj 0 0 phasor~", "Phasor" },
                { "GlyphSaw", "#X obj 0 0 saw~ 440", "Saw" },
                { "GlyphSaw2", "#X obj 0 0 saw2~ 440~", "Saw 2" },
                { "GlyphSquare", "#X obj 0 0 square~", "Square" },
                { "GlyphTriangle", "#X obj 0 0 tri~ 440", "Triangle" },
                { "GlyphImp", "#X obj 0 0 imp~ 100", "Impulse" },
                { "GlyphImp2", "#X obj 0 0 imp2~ 100", "Impulse 2" },
                { "GlyphWavetable", "#X obj 0 0 wavetable~", "Wavetable" },
                { "GlyphPlaits", "#X obj 0 0 plaits~ -model 0 440 0.25 0.5 0.1", "Plaits" },

                // band limited
                { "GlyphOscBL", "#X obj 0 0 bl.osc~ 440", "Osc band limited" },
                { "GlyphSawBL", "#X obj 0 0 bl.saw~ 440~", "Saw band limited" },
                { "GlyphSawBL2", "#X obj 0 0 bl.saw2~", "Saw band limited 2" },
                { "GlyphSquareBL", "#X obj 0 0 bl.tri~ 440", "Square band limited" },
                { "GlyphTriBL", "#X obj 0 0 bl.tri~ 100", "Triangle band limited" },
                { "GlyphImpBL", "#X obj 0 0 bl.imp~ 100", "Impulse band limited" },
                { "GlyphImpBL2", "#X obj 0 0 bl.imp2~", "Impulse band limited 2" },
                { "GlyphWavetableBL", "#X obj 0 0 bl.wavetable~", "Wavetable band limited" },
                }},
        { "Math",
            {
                { "+", "", "Add"},
                { "-", "", "Subtract" },
                { "*", "", "Multiply" },
                { "/", "", "Divide" },
                { "%", "", "Remainder" },
                { "!-", "", "Reversed inlet subtraction" },
                { "!/", "", "Reversed inlet division" },
                { ">", "", "Greater than" },
                { "<", "", "Less than" },
                { ">=", "", "Greater or equal" },
                { "<=", "", "Less or equal" },
                { "==", "", "Equality" },
                { "!=", "", "Not equal" },
                }},
        { "Math~",
            {
                { "+~", "", "(signal) Add" },
                { "-~", "", "(signal) Subtract" },
                { "*~", "", "(signal) Multiply" },
                { "/~", "", "(signal) Divide" },
                { "%~", "", "(signal) Remainder" },
                { "!-~", "", "(signal) Reversed inlet subtraction" },
                { "!/~", "", "(signal) Reversed inlet division" },
                { ">~", "", "(signal) Greater than" },
                { "<~", "", "(signal) Less than" },
                { ">=~", "", "(signal) Greater or equal" },
                { "<=~", "", "(signal) Less or equal" },
                { "==~", "", "(signal) Equality" },
                { "!=~", "", "(signal) Not equal" },
                }},
    };

    std::map<String, std::map<String, String>> defaultPalettes = {
        { "Oscillators",
            { { "multi.osc", placeholderPatch },
                { "sawtooth", placeholderPatch },
                { "rectangle", placeholderPatch },
                { "triangle", placeholderPatch },
                { "sine", placeholderPatch },
                { "noise", placeholderPatch },
                { "phase ramp", placeholderPatch } } },
        { "Filters",
            { { "lowpass", placeholderPatch },
                { "svf", placeholderPatch },
                { "EQ", placeholderPatch } } },
        { "Effects",
            {
                { "sample delay", placeholderPatch },
                { "stereo delay", placeholderPatch },
                { "chorus", placeholderPatch },
                { "phaser", placeholderPatch },
                { "flanger", placeholderPatch },
                { "drive", placeholderPatch },
                { "bitcrusher", placeholderPatch },
                { "reverb", placeholderPatch },
            } },
        { "Sequencers",
            {
                { "bpm metronome", placeholderPatch },
                { "adsr", placeholderPatch },
                { "drum sequencer", placeholderPatch },
            } }

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
