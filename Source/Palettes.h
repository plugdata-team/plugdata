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

class DraggedPaletteItem : public Component {
    
public:
    std::function<void(Point<int>)> onMouseUp = [](Point<int>){};
    
    DraggedPaletteItem(Component* targetItem) : target(targetItem)
    {
        target->addMouseListener(this, false);
        imageToDraw = target->createComponentSnapshot(target->getLocalBounds());
        
        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses);
        setBounds(target->getScreenBounds());
        setVisible(true);
        setAlwaysOnTop(true);
    }
    
    ~DraggedPaletteItem() override
    {
        target->removeMouseListener(this);
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        isDragging = false;
        onMouseUp(e.getScreenPosition());
    }
    
private:
    
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

    bool isDragging = false;
    Image imageToDraw;
    Component* target;
    ComponentDragger dragger;
};

class PaletteItem : public Component
{
public:
    PaletteItem(PluginEditor* e, ValueTree itemTree) : editor(e)
    {
        paletteName = itemTree.getProperty("Name");
        palettePatch = itemTree.getProperty("Patch");
        
        nameLabel.setText(paletteName, dontSendNotification);
        nameLabel.setInterceptsMouseClicks(false, false);
        nameLabel.onTextChange = [this, itemTree]() mutable {
            paletteName = nameLabel.getText();
            itemTree.setProperty("Name", paletteName, nullptr);
        };
        
        nameLabel.onEditorShow = [this](){
            if(auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
                editor->setJustification(Justification::centred);
            }
        };
        
        nameLabel.setJustificationType(Justification::centred);
        //nameLabel.addMouseListener(this, false);
        
        addAndMakeVisible(nameLabel);
    }
    
    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(16.0f, 4.0f).toFloat();
        
        auto ioletRadius = 6.5f;
        auto cornerRadius = 5.0f;
        
        auto [inlets, outlets] = countIolets(palettePatch);
        
        int x = bounds.getX() + 8;
        
        auto lineBounds = bounds.reduced(ioletRadius / 2);
        
        Path p;
        p.startNewSubPath(x, lineBounds.getY());
        
        g.saveState();
        g.reduceClipRegion(lineBounds.getSmallestIntegerContainer());
        
        for (int i = 0; i < inlets.size(); i++) {
            auto inletBounds = Rectangle<float>();
            int const total = inlets.size();
            float const yPosition = bounds.getY();

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? bounds.getCentreX() - ioletRadius / 2.0f : bounds.getX();
                inletBounds = Rectangle<float>(xPosition + 24, yPosition, ioletRadius, ioletRadius);

            } else if (total > 1) {
                float const ratio = (bounds.getWidth() - ioletRadius - 48) / static_cast<float>(total - 1);
                inletBounds = Rectangle<float>((bounds.getX() + ratio * i) + 24, yPosition, ioletRadius, ioletRadius);
            }
        
            auto const fromRadians = MathConstants<float>::pi * 1.5f;
            auto const toRadians = MathConstants<float>::pi * 0.5f;

            p.addCentredArc(inletBounds.getCentreX(), inletBounds.getCentreY(), ioletRadius, ioletRadius, 0.0f, fromRadians, toRadians, false);
            
            g.setColour(inlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.drawEllipse(inletBounds.withSizeKeepingCentre(ioletRadius + 5, ioletRadius + 5), 1.5f);
        }
        
        p.lineTo(lineBounds.getTopRight().translated(-cornerRadius, 0));
        
        p.quadraticTo(lineBounds.getTopRight(), lineBounds.getTopRight().translated(0, cornerRadius));
        
        p.lineTo(lineBounds.getBottomRight().translated(0, -cornerRadius));
        
        p.quadraticTo(lineBounds.getBottomRight(), lineBounds.getBottomRight().translated(-cornerRadius, 0));
        
        for (int i = outlets.size() - 1; i >= 0; i--) {
            auto outletBounds = Rectangle<float>();
            int const total = outlets.size();
            float const yPosition = bounds.getBottom() - ioletRadius;

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? bounds.getCentreX() - ioletRadius / 2.0f : bounds.getX();
                outletBounds = Rectangle<float>(xPosition + 24, yPosition, ioletRadius, ioletRadius);

            } else if (total > 1) {
                float const ratio = (bounds.getWidth() - ioletRadius - 48) / static_cast<float>(total - 1);
                outletBounds = Rectangle<float>((bounds.getX() + ratio * i) + 24, yPosition, ioletRadius, ioletRadius);
            }
        
            auto const fromRadians = MathConstants<float>::pi * 1.5f;
            auto const toRadians = MathConstants<float>::pi * 0.5f;

            p.addCentredArc(outletBounds.getCentreX(), outletBounds.getCentreY(), ioletRadius, ioletRadius, MathConstants<float>::pi, fromRadians, toRadians, false);
            
            g.setColour(outlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.drawEllipse(outletBounds.withSizeKeepingCentre(ioletRadius + 5, ioletRadius + 5), 1.5f);
        }
        
        g.restoreState();
        
        p.lineTo(lineBounds.getBottomLeft().translated(cornerRadius, 0));
        
        p.quadraticTo(lineBounds.getBottomLeft(), lineBounds.getBottomLeft().translated(0, -cornerRadius));
        
        p.lineTo(lineBounds.getTopLeft().translated(0, cornerRadius));
        p.quadraticTo(lineBounds.getTopLeft(), lineBounds.getTopLeft().translated(cornerRadius, 0));
        p.closeSubPath();
        
        g.setColour(findColour(PlugDataColour::textObjectBackgroundColourId));
        g.fillPath(p);
        
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.strokePath(p, PathStrokeType(1.0f));
    }
    
    void resized() override
    {
        nameLabel.setBounds(getLocalBounds().reduced(16, 4));
    }
    
    void mouseDrag(MouseEvent const& e) override
    {
        if (dragger) return;
        
        dragger = std::make_unique<DraggedPaletteItem>(this);
        
        dragger->onMouseUp = [this](Point<int> screenPosition){
            
            auto* cnv = editor->getCurrentCanvas();
            
            if(cnv && cnv->viewport && cnv->viewport->getScreenBounds().contains(screenPosition))
            {
                auto position = cnv->getLocalPoint(nullptr, screenPosition) + Point<int>(Object::margin, Object::margin) - cnv->canvasOrigin;
                auto result = palettePatch + StringArray{"\n#X restore ", String(position.x), String(position.y), paletteName}.joinIntoString(" ") + ";";
                
                auto* ptr = cnv->patch.getPointer();
                
                cnv->pd->lockAudioThread();
                libpd_paste(ptr, result.toRawUTF8());
                cnv->pd->unlockAudioThread();
                
                cnv->synchronise();
            }
            else
            {
                // TODO: reorder!
            }

            dragger.reset(nullptr);
        };
    }
    
    void mouseUp(const MouseEvent& e) override
    {
        if(!e.mouseWasDraggedSinceMouseDown() && e.getNumberOfClicks() >= 2)
        {
            nameLabel.showEditor();
        }
    }
    
    std::pair<std::vector<bool>, std::vector<bool>> countIolets(String const& patchAsString)
    {
        std::pair<std::vector<bool>, std::vector<bool>> result;
        auto& [inlets, outlets] = result;
        int canvasDepth = -1;

        auto isObject = [](StringArray& tokens) {
            return tokens[0] == "#X" && tokens[1] != "connect" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
        };

        auto isStartingCanvas = [](StringArray& tokens) {
            return tokens[0] == "#N" && tokens[1] == "canvas" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789") && tokens[4].containsOnly("-0123456789") && tokens[5].containsOnly("-0123456789");
        };

        auto isEndingCanvas = [](StringArray& tokens) {
            return tokens[0] == "#X" && tokens[1] == "restore" && tokens[2].containsOnly("-0123456789") && tokens[3].containsOnly("-0123456789");
        };
        
        auto countIolet = [&inlets = result.first, &outlets = result.second](String name){
            if(name == "inlet")   inlets.push_back(false);
            if(name == "outlet")  outlets.push_back(false);
            if(name == "inlet~")  inlets.push_back(true);
            if(name == "outlet~") outlets.push_back(true);
        };

        for (auto& line : StringArray::fromLines(patchAsString)) {

            line = line.upToLastOccurrenceOf(";", false, false);

            auto tokens = StringArray::fromTokens(line, true);

            if (isStartingCanvas(tokens)) {
                canvasDepth++;
            }

            if (canvasDepth == 0 && isObject(tokens)) {
                countIolet(tokens[4]);
            }

            if (isEndingCanvas(tokens)) {
                if (canvasDepth == 1) {
                    countIolet(tokens[1]);
                }
                canvasDepth--;
            }
        }
        
        return result;
    }

    Label nameLabel;
    PluginEditor* editor;
    std::unique_ptr<DraggedPaletteItem> dragger;
    String paletteName, palettePatch;
};

class PaletteComponent : public Component
{
public:
    PaletteComponent(PluginEditor* editor, ValueTree tree)
    {
        for(auto item : tree)
        {
            itemHolder.addAndMakeVisible(items.add(new PaletteItem(editor, item)));
        }
        
        nameLabel.setText(tree.getProperty("Name"), dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.setJustificationType(Justification::centred);
        
        nameLabel.onEditorShow = [this](){
            if(auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
                editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
                editor->setJustification(Justification::centred);
            }
        };
        
        nameLabel.onTextChange = [this, tree]() mutable {
            tree.setProperty("Name", nameLabel.getText(), nullptr);
        };
        
        setSize(1, items.size() * 40 + 40);
        addAndMakeVisible(nameLabel);
        
        viewport.setViewedComponent(&itemHolder, false);
        addAndMakeVisible(viewport);
    }
    
    void resized() override
    {
        viewport.setBounds(getLocalBounds().withTrimmedTop(32));
        nameLabel.setBounds(getLocalBounds().removeFromTop(32));
        
        auto itemsBounds = getLocalBounds();
        auto height = 40;
        auto totalHeight = 0;
        
        for(auto* item : items)
        {
            totalHeight += height;
            item->setBounds(itemsBounds.removeFromTop(height));
        }
        
        itemHolder.setBounds(getLocalBounds().withHeight(totalHeight));
    }
    
    void paint (Graphics& g) override
    {
        
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(30));
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(30, 0, getWidth());
    }

    Component itemHolder;
    Viewport viewport;
    Label nameLabel;
    OwnedArray<PaletteItem> items;
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
    , public SettingsFileListener, public ValueTree::Listener {
public:
    explicit Palettes(PluginEditor* e) : editor(e), resizer(this)
    {
        if (!palettesFile.exists()) {
            palettesFile.create();

            palettesTree = ValueTree("Palettes");

            for (auto [name, palette] : defaultPalettes) {
                                
                ValueTree categoryTree = ValueTree("Category");
                categoryTree.setProperty("Name", name, nullptr);
                
                for(auto& [paletteName, patch] : palette)
                {
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
        
        palettesTree.addListener(this);

        addButton.getProperties().set("Style", "SmallIcon");
        addButton.onClick = [this, e]() {
            PopupMenu menu;
            menu.addItem(1, "New palette");
            menu.addItem(2, "New palette from clipboard");

            PopupMenu defaultPalettesMenu;

            for (auto& [name, palette] : defaultPalettes) {
                defaultPalettesMenu.addItem(name, [this, name = name, palette = palette]() {
                    auto existingTree = palettesTree.getChildWithProperty("Name", name);
                    if (existingTree.isValid()) {
                        showPalette(existingTree);
                    } else {
                        
                        ValueTree categoryTree = ValueTree("Category");
                        categoryTree.setProperty("Name", name, nullptr);
                        
                        for(auto& [paletteName, patch] : palette)
                        {
                            ValueTree paletteTree("Item");
                            paletteTree.setProperty("Name", paletteName, nullptr);
                            paletteTree.setProperty("Patch", patch, nullptr);
                            categoryTree.appendChild(paletteTree, nullptr);
                        }
                    
                        palettesTree.appendChild(categoryTree, nullptr);
                        paletteSelectors.getLast()->triggerClick();
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

        paletteBar.setVisible(true);
        paletteViewport.setViewedComponent(&paletteBar, false);
        paletteViewport.setScrollBarsShown(true, true, true, true);
        paletteViewport.setScrollBarThickness(4);
        paletteViewport.setScrollBarPosition(false, false);

        addAndMakeVisible(paletteViewport);
        addAndMakeVisible(view.get());
        addAndMakeVisible(resizer);

        resizer.setAlwaysOnTop(true);

        paletteBar.addAndMakeVisible(addButton);

        setSize(300, 0);
        
        updatePalettes();

        showPalettes = SettingsFile::getInstance()->getProperty<bool>("show_palettes");
        setVisible(showPalettes);
    }

    ~Palettes() override
    {
        savePalettes();
    }

    bool isExpanded()
    {
        return view.get() && view->isVisible();
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

        if(view) view->setBounds(getLocalBounds().withTrimmedLeft(26));

        resizer.setBounds(getWidth() - 5, 0, 5, getHeight());

        repaint();

        paletteBar.addMouseListener(this, true);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (draggedTab) {
            draggedTab = nullptr;
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
        
    void showPalette(ValueTree paletteToShow)
    {
        if(!paletteToShow.isValid())
        {
            for (auto* button : paletteSelectors) {
                button->setToggleState(false, dontSendNotification);
            }
            
            resizer.setVisible(false);
            view.reset(nullptr);
        }
        else {
            view = std::make_unique<PaletteComponent>(editor, paletteToShow);
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
            g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        }

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromLeft(26));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawVerticalLine(25, 0, getHeight());

        if (view) {
            g.drawVerticalLine(getWidth() - 1, 0, getHeight());
        }
    }

    void savePalettes()
    {
        //view->savePalette();
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
                    showPalette(ValueTree());
                } else {
                    button->setToggleState(true, dontSendNotification);
                    savePalettes();
                    showPalette(palettesTree.getChildWithProperty("Name", name));
                }
            };

            button->rightClicked = [this, palette]() {
                palettesTree.removeChild(palette, nullptr);

            };
            paletteBar.addAndMakeVisible(*button);
        }

        if (isPositiveAndBelow(lastIdx, paletteSelectors.size())) {
            paletteSelectors[lastIdx]->setToggleState(true, dontSendNotification);
        } else if (view) {
            if (!paletteSelectors.size()) {
                showPalette(ValueTree());
            } else {
                paletteSelectors[std::max(0, lastIdx - 1)]->triggerClick();
            }
        }

        resized();
    }
        
    // When a property in our pkginfo changes, save it immediately
    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override
    {
        savePalettes();
        updatePalettes();
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override
    {
        savePalettes();
        updatePalettes();
    }

    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
    {
        savePalettes();
        updatePalettes();
    }
        
    void valueTreeChildOrderChanged (ValueTree&, int, int) override
    {
        savePalettes();
        updatePalettes();
    }
        
    void valueTreeParentChanged (ValueTree&) override
    {
        savePalettes();
        updatePalettes();
    }

    void newPalette(bool fromClipboard)
    {
        auto patchPrefix = "#N canvas 827 239 527 327 12;\n";
        auto patch = fromClipboard ? patchPrefix + SystemClipboard::getTextFromClipboard() : "";

        ValueTree paletteTree = ValueTree("Palette");
        paletteTree.setProperty("Name", "", nullptr);
        paletteTree.setProperty("Patch", patch, nullptr);
        palettesTree.appendChild(paletteTree, nullptr);

        paletteSelectors.getLast()->triggerClick();
    }

    PluginEditor* editor;
    File palettesFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("PaletteBar.xml");

    ValueTree palettesTree;

    std::unique_ptr<PaletteComponent> view = nullptr;

    Point<int> mouseDownPos;
    PaletteSelector* draggedTab = nullptr;

    Viewport paletteViewport;
    Component paletteBar;

    TextButton addButton = TextButton(Icons::Add);

    OwnedArray<PaletteSelector> paletteSelectors;

    static inline const String placeholderPatch = "#N canvas 827 239 527 327 12;\n"
                                                  "#X obj 72 264 outlet~;\n"
                                                  "#X obj 72 156 inlet;\n";


    std::map<String, std::map<String, String>> defaultPalettes = {
        { "Oscillators",
            {
                {"multi.osc", placeholderPatch },
                {"sawtooth", placeholderPatch },
                {"rectangle", placeholderPatch },
                {"triangle", placeholderPatch },
                {"sine", placeholderPatch },
                {"noise", placeholderPatch },
                {"phase ramp", placeholderPatch }
            }
        },
        { "Filters",
            {
                {"lowpass", placeholderPatch },
                {"svf", placeholderPatch },
                {"EQ", placeholderPatch }
            }
        },
        { "Effects",
            {
                {"sample delay", placeholderPatch },
                {"stereo delay", placeholderPatch },
                {"chorus", placeholderPatch },
                {"phaser", placeholderPatch },
                {"flanger", placeholderPatch },
                {"drive", placeholderPatch },
                {"bitcrusher", placeholderPatch },
                {"reverb", placeholderPatch },
            }
        },
        { "Sequencers",
            {
                {"bpm metronome", placeholderPatch },
                {"adsr", placeholderPatch },
                {"drum sequencer", placeholderPatch },
            }
        }
        
        
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
