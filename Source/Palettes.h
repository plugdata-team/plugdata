/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <JuceHeader.h>
#include "Canvas.h"
#include "Pd/PdInstance.h"

class PluginEditor;
class PaletteView : public Component, public Value::Listener
{
public:
    PaletteView(PluginEditor* e) : editor(e), pd(e->pd) {
        
        editModeButton.setButtonText(Icons::Edit);
        editModeButton.getProperties().set("Style", "SmallIcon");
        editModeButton.setClickingTogglesState(true);
        addAndMakeVisible(editModeButton);
        
        addAndMakeVisible(nameLabel);
        nameLabel.setJustificationType(Justification::centred);
        
        nameLabel.toBehind(&editModeButton);
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
        
        locked.referTo(cnv->locked);
        
        auto name = paletteTree.getProperty("Name").toString();
        if(name.isNotEmpty()) {
            nameLabel.setText(name, dontSendNotification);
        }
        else {
            nameLabel.setText("", dontSendNotification);
            nameLabel.showEditor();
        }
        
        nameLabel.onEditorShow = [this](){
            if(auto* editor = nameLabel.getCurrentTextEditor()) {
                editor->setJustification(Justification::centred);
            }
        };
       
        nameLabel.setEditable(true, true);
        
        nameLabel.onTextChange = [this](){
            paletteTree.setProperty("Name", nameLabel.getText(), nullptr);
            updatePalettes();
        };
        
        cnv->locked.setValue(true);
        editModeButton.onClick = [this](){
            cnv->locked = !editModeButton.getToggleState();
        };
        
        cnv->setColour(PlugDataColour::canvasBackgroundColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
        cnv->setColour(PlugDataColour::canvasDotsColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
        
        resized();
        repaint();
        cnv->repaint();
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
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRect(getLocalBounds().removeFromTop(26));
        
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawHorizontalLine(25, 0, getWidth());
    }
    
    std::function<void()> updatePalettes = [](){};
    
private:
    
    void resized() override
    {
        auto b = getLocalBounds();
        auto topPanel = b.removeFromTop(26);
        
        nameLabel.setBounds(topPanel);
        editModeButton.setBounds(topPanel.removeFromRight(26));
        
        if(cnv)
        {
            cnv->viewport->getPositioner()->applyNewBounds(b);
        }
    }
    
    
    Value locked;
    pd::Instance* pd;
    std::unique_ptr<Canvas> cnv;
    std::unique_ptr<pd::Patch> patch;
    ValueTree paletteTree;
    PluginEditor* editor;
    
    Label nameLabel;
    
    TextButton editModeButton;
};

class PaletteSelector : public TextButton
{
    
public:
    PaletteSelector(String textToShow) {
        setRadioGroupId(1011);
        setButtonText(textToShow);
        setClickingTogglesState(true);
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
    Palettes(PluginEditor* editor) : view(editor) {
        
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
        
        hideButton.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        hideButton.setAlwaysOnTop(true);
        hideButton.onClick = [this](){
            setViewHidden(true);
        };
        
        updatePalettes();
        addAndMakeVisible(paletteBar);
        addAndMakeVisible(view);
        addAndMakeVisible(hideButton);
        paletteBar.addAndMakeVisible(addButton);
        
        view.updatePalettes = [this](){
            updatePalettes();
        };
        
        setViewHidden(true);
    }
    
    ~Palettes()
    {
        savePalettes();
    }
    
    bool isExpanded() {
        return view.isVisible();
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
        
        hideButton.setBounds(getLocalBounds().withSize(26, 26).translated(26, 0));
        repaint();
    }
    
    void setViewHidden(bool hidden) {
        if(hidden) {
            for(auto* button : paletteSelectors) {
                button->setToggleState(false, sendNotification);
            }
        }
        
        view.setVisible(!hidden);
        hideButton.setVisible(!hidden);
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
            button->onClick = [this, name](){
                setViewHidden(false);
                view.showPalette(palettesTree.getChildWithProperty("Name", name));
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
    
    File palettesFile = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Palettes.xml");
    
    ValueTree palettesTree;
    
    PaletteView view;
    
    Component paletteBar;
    
    TextButton hideButton = TextButton("<");
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
};
