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

class PluginEditor;
class PaletteView : public Component, public Value::Listener
{
    class DraggedComponentGroup : public Component
    {
    public:
        DraggedComponentGroup(Canvas* canvas, Object* target, Point<int> mouseDownPosition) : cnv(canvas), draggedObject(target)
        {
            auto [clipboard, components] = getDraggedArea(target);
            
            
            Rectangle<int> totalBounds;
            for(auto* component : components)
            {
                totalBounds = totalBounds.getUnion(component->getBounds());
            }
            
            imageToDraw = getObjectsSnapshot(components, totalBounds);
            
            clipboardContent = clipboard;
            
            addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses);
            setBounds(cnv->localAreaToGlobal(totalBounds));
            setVisible(true);
        }
        
        Image getObjectsSnapshot(Array<Component*> components, Rectangle<int> totalBounds) {
            
            std::sort(components.begin(), components.end(), [this](const auto* a, const auto* b){
                return cnv->getIndexOfChildComponent(a) > cnv->getIndexOfChildComponent(b);
            });

            Image image (Image::ARGB, totalBounds.getWidth(), totalBounds.getHeight(), true);
            Graphics g (image);
            
            for(auto* component : components)
            {
                auto b = component->getBounds();
                
                g.drawImageAt(component->createComponentSnapshot(component->getLocalBounds()), b.getX() - totalBounds.getX(), b.getY() - totalBounds.getY());
            }
            
            image.multiplyAllAlphas(0.75f);
            
            return image;
        }
        
        void pasteObjects(Canvas* target) {
            
            auto position = target->getLocalPoint(nullptr, getScreenPosition()) + Point<int>(Object::margin, Object::margin);
            
            int minX = 9999999;
            int minY = 9999999;
            
            auto isObject = [](StringArray& tokens){
                return tokens[0] == "#X" &&
                tokens[1] != "connect" &&
                tokens[2].containsOnly("0123456789") &&
                tokens[3].containsOnly("0123456789");
            };
            
            for(auto& line : StringArray::fromLines(clipboardContent))
            {
                auto tokens = StringArray::fromTokens(line, true);
                if(isObject(tokens)) {
                    minX = std::min(minX, tokens[2].getIntValue());
                    minY = std::min(minY, tokens[3].getIntValue());
                }
            }
            
            auto toPaste = StringArray::fromLines(clipboardContent);
            for(auto& line : toPaste)
            {
                auto tokens = StringArray::fromTokens(line, true);
                if(isObject(tokens)) {
                    tokens.set(2, String(tokens[2].getIntValue() - minX + position.x));
                    tokens.set(3, String(tokens[3].getIntValue() - minY + position.y));
                    
                    line = tokens.joinIntoString(" ");
                }
            }
            
            auto result = toPaste.joinIntoString("\n");
            libpd_paste(target->patch.getPointer(), result.toRawUTF8());
            target->synchronise();
        }
        

    private:
        
        std::pair<String, Array<Component*>> getDraggedArea(Object* target)
        {
            std::pair<Array<Object*>, Array<Connection*>> dragged;
            getConnectedObjects(target, dragged);
            auto& [objects, connections] = dragged;
            
            for(auto* object : objects)
            {
                cnv->patch.selectObject(object->getPointer());
            }
            
            int size;
            const char* text = libpd_copy(cnv->patch.getPointer(), &size);
            String clipboard = String::fromUTF8(text, size);
            
            cnv->patch.deselectAll();
            
            Array<Component*> components;
            components.addArray(objects);
            components.addArray(connections);
            
            return {clipboard, components};
        }
        
        void getConnectedObjects(Object* target, std::pair<Array<Object*>, Array<Connection*>>& result)
        {
            auto& [objects, connections] = result;
            
            objects.addIfNotAlreadyThere(target);
            
            for(auto* connection : target->getConnections()) {
                
                connections.addIfNotAlreadyThere(connection);
                
                if(target->iolets.contains(connection->inlet) && !objects.contains(connection->outlet->object))
                {
                    getConnectedObjects(connection->outlet->object, result);
                }
                else if(!objects.contains(connection->inlet->object)){
                    getConnectedObjects(connection->inlet->object, result);
                }
            }
        }
        
        void paint(Graphics& g) override
        {
            g.drawImageAt(imageToDraw, 0, 0);
        }
    
        void mouseDrag(const MouseEvent& e) override
        {
            auto relativeEvent = e.getEventRelativeTo(this);
            
            if(!isDragging) {
                dragger.startDraggingComponent (this, relativeEvent);
                isDragging = true;
            }
            
            dragger.dragComponent (this, relativeEvent, nullptr);
        }
        
        void mouseUp(const MouseEvent& e) override
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
    PaletteView(PluginEditor* e) : editor(e), pd(e->pd) {
        
        editModeButton.setButtonText(Icons::Edit);
        editModeButton.getProperties().set("Style", "SmallIcon");
        editModeButton.setClickingTogglesState(true);
        editModeButton.setRadioGroupId(2222);
        editModeButton.onClick = [this](){
            cnv->locked = false;
        };
        addAndMakeVisible(editModeButton);
        
        lockModeButton.setButtonText(Icons::Lock);
        lockModeButton.getProperties().set("Style", "SmallIcon");
        lockModeButton.setClickingTogglesState(true);
        lockModeButton.setRadioGroupId(2222);
        lockModeButton.onClick = [this](){
            cnv->locked = true;
        };
        addAndMakeVisible(lockModeButton);
        
        patchSelector.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        
        dragModeButton.setButtonText(Icons::Presentation);
        dragModeButton.getProperties().set("Style", "SmallIcon");
        dragModeButton.setClickingTogglesState(true);
        dragModeButton.setRadioGroupId(2222);
        dragModeButton.onClick = [this](){
            cnv->locked = true;
            // TODO: make sure gui objects don't respond to mouse clicks!
        };
        
        addAndMakeVisible(dragModeButton);
        
        addAndMakeVisible(patchSelector);
        patchSelector.setJustificationType(Justification::centred);
        
        patchSelector.toBehind(&editModeButton);
    }
    
    void showPalette(ValueTree palette)
    {
        paletteTree = palette;
        
        auto patchText = paletteTree.getProperty("Patch").toString();
        
        if (patchText.isEmpty())
            patchText = pd::Instance::defaultPatch;
        
        auto patchFile = File::createTempFile(".pd");
        patchFile.replaceWithText(patchText);
        
        patch.reset(pd->openPatch(patchFile));
        cnv = std::make_unique<Canvas>(editor, *patch, nullptr);
        addAndMakeVisible(*cnv->viewport);
        
        auto name = paletteTree.getProperty("Name").toString();
        if(name.isNotEmpty()) {
            patchSelector.setText(name, dontSendNotification);
        }
        else {
            patchSelector.setText("", dontSendNotification);
            patchSelector.showEditor();
        }

        patchSelector.setEditableText(true);
        
        patchSelector.onChange = [this](){
            paletteTree.setProperty("Name", patchSelector.getText(), nullptr);
            updatePalettes();
        };

        cnv->addMouseListener(this, true);
        cnv->lookAndFeelChanged();
        cnv->setColour(PlugDataColour::canvasBackgroundColourId, Colours::transparentBlack);
        cnv->setColour(PlugDataColour::canvasDotsColourId, Colours::transparentBlack);
        
        resized();
        repaint();
        cnv->repaint();
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        if(dragger || !dragModeButton.getToggleState() || !cnv) return;
        
        auto relativeEvent = e.getEventRelativeTo(cnv.get());
        
        auto* object = dynamic_cast<Object*>(e.originalComponent);
        if(!object)
        {
            object = e.originalComponent->findParentComponentOfClass<Object>();
            if(!object) return;
        }
        
        dragger = std::make_unique<DraggedComponentGroup>(cnv.get(), object, relativeEvent.getMouseDownPosition());
        cnv->addMouseListener(dragger.get(), true);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        if(!dragModeButton.getToggleState() || !cnv) return;
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if(!dragger) return;
        
        cnv->removeMouseListener(dragger.get());
                
        Canvas* targetCanvas = nullptr;
        if(auto* leftCnv = editor->splitView.getLeftTabbar()->getCurrentCanvas())
        {
            if(leftCnv->getScreenBounds().contains(e.getScreenPosition()))
            {
                targetCanvas = leftCnv;
            }
        }
        if(auto* rightCnv = editor->splitView.getRightTabbar()->getCurrentCanvas())
        {
            if(rightCnv->getScreenBounds().contains(e.getScreenPosition()))
            {
                targetCanvas = rightCnv;
            }
        }
        if(targetCanvas) dragger->pasteObjects(targetCanvas);

        dragger.reset(nullptr);
    }
    
    void valueChanged(Value& v) override
    {
        if(v.refersToSameSourceAs(locked))
        {
            editModeButton.setToggleState(!static_cast<bool>(locked.getValue()), dontSendNotification);
        }
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(26));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawHorizontalLine(25, 0, getWidth());
    }
    
    
    void resized() override
    {
        auto b = getLocalBounds();
        auto topPanel = b.removeFromTop(26);
        
        editModeButton.setBounds(topPanel.removeFromRight(26));
        lockModeButton.setBounds(topPanel.removeFromRight(26));
        dragModeButton.setBounds(topPanel.removeFromRight(26));
        patchSelector.setBounds(topPanel);
        
        if(cnv)
        {
            cnv->viewport->getPositioner()->applyNewBounds(b);
            cnv->checkBounds();
        }
    }
    
    
    Canvas* getCanvas()
    {
        return cnv.get();
    }
    
    std::function<void()> updatePalettes = [](){};
    

private:

    Value locked;
    pd::Instance* pd;
    std::unique_ptr<pd::Patch> patch;
    ValueTree paletteTree;
    PluginEditor* editor;
    
    std::unique_ptr<DraggedComponentGroup> dragger = nullptr;
    std::unique_ptr<Canvas> cnv;
    
    ComboBox patchSelector;
    
    TextButton editModeButton;
    TextButton lockModeButton;
    TextButton dragModeButton;
};

class PaletteSelector : public TextButton
{
    
public:
    PaletteSelector(String textToShow) {
        setRadioGroupId(1011);
        setButtonText(textToShow);
        //setClickingTogglesState(true);
    }
    
    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::toolbarBackgroundColourId));
        
        if(getToggleState()) {
            g.setColour(findColour(PlugDataColour::toolbarActiveColourId));
            g.fillRect(getLocalBounds().removeFromBottom(4));
        }
        
        getLookAndFeel().drawButtonText (g, *this, isMouseOver(), getToggleState());
    }
    
};

class Palettes : public Component
{
public:
    Palettes(PluginEditor* editor) : view(editor), resizer(this) {
        
        setAlwaysOnTop(true);
        
        if(!palettesFile.exists())
        {
            palettesFile.create();
            
            palettesTree = ValueTree("Palettes");
            
            for(auto [name, patch] : defaultPalettes)
            {
                ValueTree paletteTree = ValueTree("Palette");
                paletteTree.setProperty("Name", name, nullptr);
                paletteTree.setProperty("Patch", patch, nullptr);
                paletteTree.setProperty("Width", 300, nullptr);
                palettesTree.appendChild(paletteTree, nullptr);
            }
            
            savePalettes();
        }
        else {
            palettesTree = ValueTree::fromXml(palettesFile.loadFileAsString());
        }
        
        addButton.getProperties().set("Style", "SmallIcon");
        addButton.onClick = [this, editor](){
            
            PopupMenu menu;
            menu.addItem(1, "New palette");
            menu.addItem(2, "New palette from clipboard");
            
            menu.showMenuAsync(PopupMenu::Options().withMinimumWidth(100).withMaximumNumColumns(1).withParentComponent(editor).withTargetComponent(&addButton), ModalCallbackFunction::create([this](int result){
                if(result > 0) {
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
        
        view.updatePalettes = [this](){
            updatePalettes();
        };
        
        setViewHidden(true);
        setSize(300, 0);
    }
    
    ~Palettes()
    {
        savePalettes();
    }
    
    bool isExpanded() {
        return view.isVisible();
    }

    Canvas* getCurrentCanvas()
    {
        return view.getCanvas();
    }
    
    
private:
    
    bool hitTest(int x, int y) override
    {
        if(!view.isVisible()) {
            return x < 26;
        }
        
        return true;
    }
    
    
    void resized() override
    {
        auto barBounds = getLocalBounds().removeFromBottom(26).withWidth(getHeight() + 26);
        
        auto buttonArea = barBounds.withZeroOrigin();
        for(auto* button : paletteSelectors)
        {
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
    
    void setViewHidden(bool hidden) {
        if(hidden) {
            for(auto* button : paletteSelectors) {
                button->setToggleState(false, dontSendNotification);
            }
        }
        
        view.setVisible(!hidden);
        resizer.setVisible(!hidden);
        if(auto* parent = getParentComponent()) parent->resized();
    }
    
    void paint(Graphics& g) override
    {
        if(view.isVisible()) {
            g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        }
        
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromLeft(26));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawVerticalLine(25, 0, getHeight());
        
        if(view.isVisible()) {
            g.drawVerticalLine(getWidth() - 1, 0, getHeight());
        }
    }
    
    void savePalettes()
    {
        palettesFile.replaceWithText(palettesTree.toXmlString());
    }
    
    void updatePalettes()
    {
        paletteSelectors.clear();
        
        for(auto palette : palettesTree)
        {
            auto name = palette.getProperty("Name").toString();
            auto button = paletteSelectors.add(new PaletteSelector(name));
            button->onClick = [this, name, button](){
                
                if(button->getToggleState()) {
                    button->setToggleState(false, dontSendNotification);
                    setViewHidden(true);
                }
                else {
                    button->setToggleState(true, dontSendNotification);
                    setViewHidden(false);
                    view.showPalette(palettesTree.getChildWithProperty("Name", name));
                }
            };
            paletteBar.addAndMakeVisible(*button);
        }
        
        resized();
    }

    void newPalette(bool fromClipboard) {
        
        auto patchPrefix = "#N canvas 827 239 527 327 12;\n";
        auto patch = fromClipboard ? patchPrefix  + SystemClipboard::getTextFromClipboard() : "";
        
        ValueTree paletteTree = ValueTree("Palette");
        paletteTree.setProperty("Name", "", nullptr);
        paletteTree.setProperty("Patch", patch, nullptr);
        paletteTree.setProperty("Width", 300, nullptr);
        palettesTree.appendChild(paletteTree, nullptr);
        
        updatePalettes();
        
        paletteSelectors.getLast()->setToggleState(true, sendNotification);
    }
    
    int dragStartWidth = 0;
    bool resizing = false;
    
    File palettesFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Palettes.xml");
    
    ValueTree palettesTree;
    
    PaletteView view;
    
    Component paletteBar;
    
    TextButton addButton = TextButton(Icons::Add);
    
    OwnedArray<PaletteSelector> paletteSelectors;
    
    static inline const String oscillatorsPatch =
    "#N canvas 827 239 527 327 12;\n"
    "#X obj 110 125 osc~ 440;\n"
    "#X obj 110 170 phasor~;\n"
    "#X obj 110 214 saw~ 440;\n"
    "#X obj 110 250 saw2~ 440;\n"
    "#X obj 110 285 square~ 440;\n"
    "#X obj 110 320 triangle~ 440;\n";
    
    static inline const String filtersPatch =
    "#N canvas 827 239 527 327 12;\n"
    "#X obj 110 97 lop~ 100;\n"
    "#X obj 110 140 vcf~;\n"
    "#X obj 110 184 lores~ 800 0.1;\n"
    "#X obj 110 222 svf~ 800 0.1;\n"
    "#X obj 110 258 bob~;\n";
    
    
    std::map<String, String> defaultPalettes = {
        {"Oscillators", oscillatorsPatch},
        {"Filters", filtersPatch}
    };
    
    class ResizerComponent : public Component
    {
    public:
        ResizerComponent(Component* toResize) : target(toResize)
        {}
    private:
        void mouseDown(MouseEvent const& e) override
        {
            dragStartWidth = target->getWidth();
        }

        void mouseDrag(MouseEvent const& e) override
        {
            int newWidth = dragStartWidth + e.getDistanceFromDragStartX();
            newWidth = std::clamp(newWidth, 100, std::max(target->getParentWidth() / 2, 150));

            std::cout << newWidth << std::endl;
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
