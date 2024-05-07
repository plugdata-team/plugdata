/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Object.h"
#include "Objects/ObjectBase.h"
#include <m_pd.h>
#include <m_imp.h>

class SearchPanelSettings : public Component
{
public:
    struct SearchPanelSettingsButton : public TextButton {
        String const icon;
        String const description;

        SearchPanelSettingsButton(String iconString, String descriptionString)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
            setClickingTogglesState(true);

            auto sortLayerOrder = SettingsFile::getInstance()->getProperty<bool>("search_order");
            setToggleState(sortLayerOrder, dontSendNotification);

            onClick = [this](){
                SettingsFile::getInstance()->setProperty("search_order", var(getToggleState()));
            };
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::toolbarTextColourId);
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14);

            if (getToggleState()) {
                colour = findColour(PlugDataColour::toolbarActiveColourId);
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    SearchPanelSettings()
    {
        addAndMakeVisible(sortLayerOrder);

        setSize(150, 28);
    };

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int buttonHeight = buttonBounds.getHeight();

        sortLayerOrder.setBounds(buttonBounds.removeFromTop(buttonHeight));
    }
private:
    SearchPanelSettingsButton sortLayerOrder = SearchPanelSettingsButton(Icons::AutoScroll, "Display layer order");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPanelSettings);
};

class SearchPanel : public Component, public KeyListener, public Timer
{
public:
    explicit SearchPanel(PluginEditor* pluginEditor) : editor(pluginEditor)
    {
        input.setBackgroundColour(PlugDataColour::sidebarActiveBackgroundColourId);
        input.setTextToShowWhenEmpty("Type to search in patch", findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));

        input.onTextChange = [this]() {
            patchTree.setFilterString(input.getText());
        };

        input.addKeyListener(this);
        patchTree.addKeyListener(this);
        
        patchTree.onClick = [this](ValueTree& tree){
            auto* ptr = reinterpret_cast<void*>(static_cast<int64>(tree.getProperty("Object")));
            editor->highlightSearchTarget(ptr, true);
        };
        
        patchTree.onSelect = [this](ValueTree& tree){
            auto* ptr = reinterpret_cast<void*>(static_cast<int64>(tree.getProperty("TopLevel")));
            editor->highlightSearchTarget(ptr, false);
        };

        addAndMakeVisible(patchTree);
        addAndMakeVisible(input);

        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 5, 1 });
    }
    
    
    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        return false;
    }

    void clear()
    {
        patchTree.clearValueTree();
    }
    
    void timerCallback() override
    {
        auto* cnv = editor->getCurrentCanvas();
        if(cnv && (currentCanvas.getComponent() != cnv || cnv->needsSearchUpdate))
        {
            currentCanvas = cnv;
            currentCanvas->needsSearchUpdate = false;
            updateResults();
        }
    }
    
    void visibilityChanged() override
    {
        if(isVisible())
        {
            startTimer(100);
        }
        else {
            stopTimer();
        }
    }
    
    void lookAndFeelChanged() override
    {
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        input.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::sidebarTextColourId));
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(getLocalBounds());

        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
        g.fillRoundedRectangle(input.getBounds().reduced(6, 4).toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto backgroundColour = findColour(PlugDataColour::sidebarBackgroundColourId);
        auto transparentColour = backgroundColour.withAlpha(0.0f);

        // Draw a gradient to fade the content out underneath the search input
        g.setGradientFill(ColourGradient(backgroundColour, 0.0f, 30.0f, transparentColour, 0.0f, 42.0f, false));
        g.fillRect(Rectangle<int>(0, input.getBottom(), getWidth(), 12));

        auto colour = findColour(PlugDataColour::sidebarTextColourId);
        Fonts::drawIcon(g, Icons::Search, 2, 1, 32, colour, 12);
    }

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show search settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [this, settingsCalloutButton]() {
            auto* editor = findParentComponentOfClass<PluginEditor>();
            auto* sidebar = getParentComponent();
            auto bounds = editor->getLocalArea(sidebar, settingsCalloutButton->getBounds());

            auto docsSettings = std::make_unique<SearchPanelSettings>();
            CallOutBox::launchAsynchronously(std::move(docsSettings), bounds, editor);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }
    
    void updateResults()
    {
        auto* cnv = editor->getCurrentCanvas();
        if(cnv) {
            cnv->pd->lockAudioThread(); // It locks inside of this anyway, so we might as well lock around it to prevent constantly locking/unlocking
            auto tree = generatePatchTree(cnv->refCountedPatch);
            patchTree.setValueTree(tree);
            cnv->pd->unlockAudioThread();
        }
    }
    
    void grabFocus()
    {
        input.grabKeyboardFocus();
    }
    
    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(34);

        tableBounds.removeFromTop(2);

        input.setBounds(inputBounds.reduced(5, 4));
        patchTree.setBounds(tableBounds);
    }

    ValueTree generatePatchTree(pd::Patch::Ptr patch, void* topLevel = nullptr)
    {
        ValueTree patchTree("Patch");
        for (auto objectPtr : patch->getObjects()) {
            if (auto object = objectPtr.get<t_pd>()) {
                auto* top = topLevel ? topLevel : object.get();
                String type = String::fromUTF8(pd::Interface::getObjectClassName(object.get()));

                if (!pd::Interface::checkObject(object.get()))
                    continue;

                char* objectText;
                int len;
                pd::Interface::getObjectText(object.cast<t_text>(), &objectText, &len);

                int x, y, w, h;
                pd::Interface::getObjectBounds(patch->getPointer().get(), object.cast<t_gobj>(), &x, &y, &w, &h);

                auto name = String::fromUTF8(objectText, len);
                auto nameWithoutArgs = name.upToFirstOccurrenceOf(" ", false, false);
                auto positionText = " (" + String(x) + ":" + String(y) + ")";

                ValueTree element("Object");
                if (type == "canvas" || type == "graph") {
                    pd::Patch::Ptr subpatch = new pd::Patch(objectPtr, editor->pd, false);
                    ValueTree subpatchTree = generatePatchTree(subpatch, top);
                    element.copyPropertiesAndChildrenFrom(subpatchTree, nullptr);
                    
                    if(auto patchPtr = subpatch->getPointer())
                    {
                        if(patchPtr->gl_list)
                        {
                            t_class* c = patchPtr->gl_list->g_pd;
                            if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                                name = "array"; // TODO: add array name
                            } else if (patchPtr->gl_isgraph) {
                                name = nameWithoutArgs;
                            }
                        }
                        else if(patchPtr->gl_isgraph)
                        {
                            name = nameWithoutArgs;
                        }
                    }
                
                    element.setProperty("Name", name, nullptr);
                    element.setProperty("RightText", positionText, nullptr);
                    element.setProperty("Icon", canvas_isabstraction(subpatch->getPointer().get()) ? Icons::File : Icons::Object, nullptr);
                    element.setProperty("Object", reinterpret_cast<int64>(object.cast<void>()), nullptr);
                    element.setProperty("TopLevel", reinterpret_cast<int64>(top), nullptr);
                } else {
                    String finalFormatedName;

                    switch(hash(type)){
                        case hash("bng"):
                        case hash("button"):
                        case hash("hsl"):
                        case hash("vsl"):
                        case hash("slider"):
                        case hash("tgl"):
                        case hash("nbx"):
                        case hash("numbox~"):
                        case hash("vradio"):
                        case hash("hradio"):
                        case hash("cnv"):
                        case hash("vu"):
                        case hash("pad"):
                        case hash("keyboard"):
                        case hash("pic"):
                        case hash("gatom"):
                        case hash("canvas"):
                        case hash("scope~"):
                        case hash("function"):
                        case hash("bicoeff"):
                        case hash("messbox"):
                        case hash("note"):
                        case hash("knob"):
                        {
                            finalFormatedName = nameWithoutArgs;
                            break;
                        }
                        case hash("message"):
                        {
                            finalFormatedName = "msg " + name;
                            break;
                        }
                        case hash("comment"):
                        {
                            finalFormatedName = "comment " + name;
                            break;
                        }
                        default:
                        {
                            finalFormatedName = name;
                            break;
                        }
                            
                    }
                    
                    element.setProperty("Name", finalFormatedName, nullptr);
                    element.setProperty("RightText", positionText, nullptr);
                    element.setProperty("Icon", Icons::Object, nullptr);
                    element.setProperty("Object", reinterpret_cast<int64>(object.cast<void>()), nullptr);
                    element.setProperty("TopLevel", reinterpret_cast<int64>(top), nullptr);
                }

                patchTree.appendChild(element, nullptr);
            }
        }

        return patchTree;
    }

    SafePointer<Canvas> currentCanvas;
    PluginEditor* editor;
    ValueTreeViewerComponent patchTree = ValueTreeViewerComponent("(Subpatch)");
    SearchEditor input;
};
