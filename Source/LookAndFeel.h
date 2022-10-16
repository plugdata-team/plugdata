/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>
#include <map>

struct Icons
{
    inline static const CharPointer_UTF8 New = CharPointer_UTF8("\xef\x85\x9b");
    inline static const CharPointer_UTF8 Open = CharPointer_UTF8("\xef\x81\xbb");
    inline static const CharPointer_UTF8 Save = CharPointer_UTF8("\xef\x83\x87");
    inline static const CharPointer_UTF8 SaveAs = CharPointer_UTF8("\xef\x80\x99");
    inline static const CharPointer_UTF8 Undo = CharPointer_UTF8("\xef\x83\xa2");
    inline static const CharPointer_UTF8 Redo = CharPointer_UTF8("\xef\x80\x9e");
    inline static const CharPointer_UTF8 Add = CharPointer_UTF8("\xef\x81\xa7");
    inline static const CharPointer_UTF8 Settings = CharPointer_UTF8("\xef\x80\x93");
    inline static const CharPointer_UTF8 Hide = CharPointer_UTF8("\xef\x81\x94");
    inline static const CharPointer_UTF8 Show = CharPointer_UTF8("\xef\x81\x93");
    inline static const CharPointer_UTF8 Clear = CharPointer_UTF8("\xef\x80\x8d");
    inline static const CharPointer_UTF8 Lock = CharPointer_UTF8("\xef\x80\xa3");
    inline static const CharPointer_UTF8 Unlock = CharPointer_UTF8("\xef\x82\x9c");
    inline static const CharPointer_UTF8 ConnectionStyle = CharPointer_UTF8("\xee\xa1\xbc");
    inline static const CharPointer_UTF8 Power = CharPointer_UTF8("\xef\x80\x91");
    inline static const CharPointer_UTF8 Audio = CharPointer_UTF8("\xef\x80\xa8");
    inline static const CharPointer_UTF8 Search = CharPointer_UTF8("\xef\x80\x82");
    inline static const CharPointer_UTF8 Wand = CharPointer_UTF8("\xef\x83\x90");
    inline static const CharPointer_UTF8 Pencil = CharPointer_UTF8("\xef\x87\xbc");
    inline static const CharPointer_UTF8 Colour = CharPointer_UTF8("\xef\x87\xbb");
    inline static const CharPointer_UTF8 Grid = CharPointer_UTF8("\xef\x83\x8e");
    inline static const CharPointer_UTF8 Theme = CharPointer_UTF8("\xef\x81\x82");
    inline static const CharPointer_UTF8 ZoomIn = CharPointer_UTF8("\xef\x80\x8e");
    inline static const CharPointer_UTF8 ZoomOut = CharPointer_UTF8("\xef\x80\x90");
    inline static const CharPointer_UTF8 Pin = CharPointer_UTF8("\xef\x82\x8d");
    inline static const CharPointer_UTF8 Keyboard = CharPointer_UTF8("\xef\x84\x9c");
    inline static const CharPointer_UTF8 Folder = CharPointer_UTF8("\xef\x81\xbb");
    inline static const CharPointer_UTF8 OpenedFolder = CharPointer_UTF8("\xef\x81\xbc");
    inline static const CharPointer_UTF8 File = CharPointer_UTF8("\xef\x85\x9c");
    inline static const CharPointer_UTF8 AutoScroll = CharPointer_UTF8("\xef\x80\xb4");
    inline static const CharPointer_UTF8 Restore = CharPointer_UTF8("\xef\x83\xa2");
    inline static const CharPointer_UTF8 Error = CharPointer_UTF8("\xef\x81\xb1");
    inline static const CharPointer_UTF8 Message = CharPointer_UTF8("\xef\x81\xb5");
    inline static const CharPointer_UTF8 Parameters = CharPointer_UTF8("\xef\x87\x9e");
    inline static const CharPointer_UTF8 Presentation = CharPointer_UTF8("\xef\x81\xab");
    inline static const CharPointer_UTF8 Externals = CharPointer_UTF8("\xef\x84\xae");
    inline static const CharPointer_UTF8 Info = CharPointer_UTF8("\xef\x81\x9a");
    inline static const CharPointer_UTF8 Refresh = CharPointer_UTF8("\xef\x80\xa1");
    inline static const CharPointer_UTF8 Up = CharPointer_UTF8("\xef\x81\xa2");
    inline static const CharPointer_UTF8 Down = CharPointer_UTF8("\xef\x81\xa3");
    inline static const CharPointer_UTF8 Edit = CharPointer_UTF8("\xef\x81\x80");
    inline static const CharPointer_UTF8 ThinDown = CharPointer_UTF8("\xef\x84\x87");
    inline static const CharPointer_UTF8 Sine = CharPointer_UTF8("\xee\xa1\x95");
    inline static const CharPointer_UTF8 Documentation = CharPointer_UTF8("\xef\x80\xad");
    inline static const CharPointer_UTF8 AddCircled = CharPointer_UTF8("\xef\x81\x95");
    inline static const CharPointer_UTF8 Console = CharPointer_UTF8("\xef\x84\xa0");
};


enum PlugDataColour
{
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,
    
    tabBackgroundColourId,
    tabTextColourId,
    tabBorderColourId, /* TODO: rename to outline */
    activeTabBackgroundColourId,
    activeTabTextColourId,
    activeTabBorderColourId, /* TODO: rename to outline */
    
    canvasBackgroundColourId,
    canvasTextColourId,
    canvasLockedOutlineColourId,
    canvasUnlockedOutlineColourId,
    
    defaultObjectBackgroundColourId,
    outlineColourId,  /* TODO: rename to objectOutlineColourId */
    canvasActiveColourId, /* TODO: rename to selectedObjectOutlineColourId */
    dataColourId,
    connectionColourId,
    signalColourId,
    
    panelBackgroundColourId,
    panelBackgroundOffsetColourId,
    panelTextColourId,
    panelActiveBackgroundColourId, /* TODO: rename to panelSelectedItemBackgroundColourId */
    panelActiveTextColourId, /* TODO: rename to panelSelectedItemTextColourId */
    
    scrollbarBackgroundColourId, /* TODO: rename */
    
    /* iteration hack */
    numberOfColours
};

inline static const std::map<PlugDataColour, std::pair<String, String>> PlugDataColourNames = {
    
    {toolbarBackgroundColourId, {"Toolbar Background", "toolbar_background"}},
    {toolbarTextColourId, {"Toolbar Text", "toolbar_text"}},
    {toolbarActiveColourId, {"Toolbar Active", "toolbar_active"}},
    {tabBackgroundColourId, {"Tab Background", "tab_background"}},
    {tabTextColourId, {"Tab Text", "tab_text"}},
    {tabBorderColourId, {"Tab Border", "tab_border"}}, /* TODO: rename to outline */
    {activeTabBackgroundColourId, {"Active Tab Background", "active_tab_background"}},
    {activeTabTextColourId, {"Active Tab Text", "active_tab_text"}},
    {activeTabBorderColourId, {"Active Tab Border", "active_tab_border"}}, /* TODO: rename to outline */
    {canvasBackgroundColourId, {"Canvas Background", "canvas_background"}},
    {canvasTextColourId, {"Canvas Text", "canvas_text"}},
    {canvasLockedOutlineColourId, {"Canvas Locked Outline", "canvas_locked_outline"}},
    {canvasUnlockedOutlineColourId, {"Canvas Unlocked Outline", "canvas_unlocked_outline"}},
    {defaultObjectBackgroundColourId, {"Default Object Background", "default_object_background"}},
    {outlineColourId, {"Outline Colour", "outline_colour"}},  /* TODO: rename to objectOutlineColourId */
    {canvasActiveColourId, {"Canvas Active Colour", "canvas_active_colour"}}, /* TODO: rename to {selectedObjectOutlineColourId */
    {dataColourId, {"Data Colour", "data_colour"}},
    {connectionColourId, {"Connection Colour", "connection_colour"}},
    {signalColourId, {"Signal Colour", "signal_colour"}},
    {panelBackgroundColourId, {"Sidebar Background", "sidebar_colour"}},
    {panelBackgroundOffsetColourId, {"Sidebar Offset", "sidebar_offset"}},
    {panelTextColourId, {"Sidebar Text", "sidebar_text"}},
    {panelActiveBackgroundColourId, {"Sidebar Background Active", "sidebar_background_active"}}, /* TODO: rename to {panelSelectedItemBackgroundColourId */
    {panelActiveTextColourId, {"Panel Active Text", "panel_active_text"}}, /* TODO: rename to {panelSelectedItemTextColourId */
    {scrollbarBackgroundColourId, {"Scrollbar Background", "scrollbar_background"}}, /* TODO: rename */
};
    
    
    struct Resources
    {
        Typeface::Ptr defaultTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
        
        Typeface::Ptr thinTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize);
        
        Typeface::Ptr boldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);
        
        Typeface::Ptr iconTypeface = Typeface::createSystemTypefaceFor(BinaryData::PlugDataFont_ttf, BinaryData::PlugDataFont_ttfSize);
    };
    
    struct PlugDataLook : public LookAndFeel_V4
    {
        SharedResourcePointer<Resources> resources;
        
        Font defaultFont;
        Font boldFont;
        Font thinFont;
        Font iconFont;
        
        
        PlugDataLook() :
        defaultFont(resources->defaultTypeface),
        boldFont(resources->boldTypeface),
        thinFont(resources->thinTypeface),
        iconFont(resources->iconTypeface)
        {
            setTheme(false);
            setDefaultSansSerifTypeface(resources->defaultTypeface);
        }
        
        class PlugData_DocumentWindowButton : public Button
        {
        public:
            PlugData_DocumentWindowButton(const String& name, Path normal, Path toggled) : Button(name), normalShape(std::move(normal)), toggledShape(std::move(toggled))
            {
            }
            
            void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
            {
                auto colour = findColour(TextButton::textColourOffId);
                
                g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f) : colour);
                
                if (shouldDrawButtonAsHighlighted)
                {
                    g.setColour(findColour(Slider::thumbColourId));
                }
                
                auto& p = getToggleState() ? toggledShape : normalShape;
                
                auto reducedRect = Justification(Justification::centred).appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds()).toFloat().reduced(getHeight() * 0.3f);
                
                g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
            }
            
        private:
            Colour colour;
            Path normalShape, toggledShape;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugData_DocumentWindowButton)
        };
        
        int getSliderThumbRadius(Slider& s) override
        {
            if (s.getName().startsWith("statusbar"))
            {
                return 6;
            }
            return LookAndFeel_V4::getSliderThumbRadius(s);
        }
        
        void fillResizableWindowBackground(Graphics& g, int w, int h, const BorderSize<int>& border, ResizableWindow& window) override
        {
            if(auto* dialog = dynamic_cast<FileChooserDialogBox*>(&window)) {
                g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));
            }
        }
        
        void drawResizableWindowBorder(Graphics&, int w, int h, const BorderSize<int>& border, ResizableWindow&) override
        {
        }
        
        void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            if (button.getName().startsWith("toolbar") || button.getName().startsWith("tabbar"))
            {
                drawToolbarButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
            else if (button.getName().startsWith("statusbar"))
            {
                drawStatusbarButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
            else if (button.getName().startsWith("suggestions"))
            {
                drawSuggestionButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
            else if (button.getName().startsWith("pd"))
            {
                drawPdButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
            else if (button.getName().startsWith("inspector"))
            {
                drawInspectorButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
            else
            {
                LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            }
        }
        
        void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override
        {
            if (button.getName().startsWith("suggestions"))
            {
                drawSuggestionButtonText(g, button, isMouseOverButton, isButtonDown);
            }
            else if (button.getName().startsWith("statusbar"))
            {
                drawStatusbarButtonText(g, button, isMouseOverButton, isButtonDown);
            }
            else
            {
                LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
            }
        }
        
        Font getTextButtonFont(TextButton& but, int buttonHeight) override
        {
            if (but.getName().startsWith("toolbar"))
            {
                return getToolbarFont(buttonHeight);
            }
            if (but.getName().startsWith("statusbar:oversample"))
            {
                return {buttonHeight / 2.2f};
            }
            if (but.getName().startsWith("tabbar"))
            {
                return iconFont.withHeight(buttonHeight / 2.7f);
            }
            if (but.getName().startsWith("statusbar") || but.getName().startsWith("tab"))
            {
                return getStatusbarFont(buttonHeight);
            }
            if (but.getName().startsWith("suggestions"))
            {
                return getSuggestionFont(buttonHeight);
            }
            
            return {buttonHeight / 1.7f};
        }
        
        void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
        {
            if (slider.getName().startsWith("statusbar"))
            {
                drawVolumeSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            }
            else
            {
                LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            }
        }
        
        void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const Image* icon, bool drawTitleTextOnLeft) override
        {
            if (w * h == 0) return;
            
            g.setColour(findColour(ComboBox::backgroundColourId));
            g.fillAll();
            
            Font font(h * 0.65f, Font::plain);
            g.setFont(font);
            
            g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::defaultText));
            
            g.setColour(Colours::white);
            g.drawText(window.getName(), 0, 0, w, h, Justification::centred, true);
        }
        
        Button* createDocumentWindowButton(int buttonType) override
        {
            Path shape;
            auto crossThickness = 0.15f;
            
            if (buttonType == DocumentWindow::closeButton)
            {
                shape.addLineSegment({0.0f, 0.0f, 1.0f, 1.0f}, crossThickness);
                shape.addLineSegment({1.0f, 0.0f, 0.0f, 1.0f}, crossThickness);
                
                return new PlugData_DocumentWindowButton("close", shape, shape);
            }
            
            if (buttonType == DocumentWindow::minimiseButton)
            {
                shape.addLineSegment({0.0f, 0.5f, 1.0f, 0.5f}, crossThickness);
                
                return new PlugData_DocumentWindowButton("minimise", shape, shape);
            }
            
            if (buttonType == DocumentWindow::maximiseButton)
            {
                shape.addLineSegment({0.5f, 0.0f, 0.5f, 1.0f}, crossThickness);
                shape.addLineSegment({0.0f, 0.5f, 1.0f, 0.5f}, crossThickness);
                
                Path fullscreenShape;
                fullscreenShape.startNewSubPath(45.0f, 100.0f);
                fullscreenShape.lineTo(0.0f, 100.0f);
                fullscreenShape.lineTo(0.0f, 0.0f);
                fullscreenShape.lineTo(100.0f, 0.0f);
                fullscreenShape.lineTo(100.0f, 45.0f);
                fullscreenShape.addRectangle(45.0f, 45.0f, 100.0f, 100.0f);
                PathStrokeType(30.0f).createStrokedPath(fullscreenShape, fullscreenShape);
                
                return new PlugData_DocumentWindowButton("maximise", shape, fullscreenShape);
            }
            
            jassertfalse;
            return nullptr;
        }
        
        int getTabButtonBestWidth(TabBarButton& button, int tabDepth) override
        {
            auto& buttonBar = button.getTabbedButtonBar();
            return (buttonBar.getWidth() / buttonBar.getNumTabs()) + 1;
        }
        
        int getTabButtonOverlap(int tabDepth) override
        {
            return 0;
        }
        
        void drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override
        {
            bool isActive = button.getToggleState();
            g.setColour(findColour(isActive ? PlugDataColour::activeTabBackgroundColourId : PlugDataColour::tabBackgroundColourId));
            
            g.fillRect(button.getLocalBounds());
            
            int w = button.getWidth();
            int h = button.getHeight();
            
            if (button.getIndex() != button.getTabbedButtonBar().getNumTabs() - 1)
            {
                g.setColour(button.findColour(PlugDataColour::outlineColourId));
                g.drawLine(Line<float>(w - 0.5f, 0, w - 0.5f, h), 1.0f);
            }
            
            TextLayout textLayout;
            auto textArea = button.getLocalBounds();
            AttributedString attributedTabTitle(button.getTitle());
            auto tabTextColour = findColour(isActive ? PlugDataColour::activeTabTextColourId : PlugDataColour::tabTextColourId);
            attributedTabTitle.setColour(tabTextColour);
            attributedTabTitle.setFont(defaultFont);
            attributedTabTitle.setJustification(Justification::centred);
            textLayout.createLayout(attributedTabTitle, textArea.getWidth());
            textLayout.draw(g, textArea.toFloat());
        }
        
        void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, const int w, const int h) override
        {
        }
        
        Font getTabButtonFont(TabBarButton&, float height) override
        {
            return {height * 0.4f};
        }
        
        Font getToolbarFont(int buttonHeight)
        {
            return iconFont.withHeight(buttonHeight / 3.5);
        }
        
        Font getStatusbarFont(int buttonHeight)
        {
            return iconFont.withHeight(buttonHeight / 2.5);
        }
        
        Font getSuggestionFont(int buttonHeight)
        {
            return {buttonHeight / 1.9f};
        }
        
        void drawPopupMenuBackground(Graphics& g, int width, int height) override
        {
            // Add a bit of alpha to disable the opaque flag
            auto background = findColour(PopupMenu::backgroundColourId);
            g.setColour(background);
            
            // Fill background if there's no support for transparent popupmenus
#ifdef PLUGDATA_STANDALONE
            if (!Desktop::canUseSemiTransparentWindows())
            {
                g.fillAll(findColour(ResizableWindow::backgroundColourId));
            }
#endif
            
            // On linux, the canUseSemiTransparentWindows flag sometimes incorrectly returns true
#ifdef JUCE_LINUX
            g.fillAll(findColour(ResizableWindow::backgroundColourId));
#endif
            
            auto bounds = Rectangle<float>(2, 2, width - 4, height - 4);
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(findColour(PopupMenu::textColourId));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        }
        
        int getPopupMenuBorderSize() override
        {
            return 5;
        };
        
        void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override
        {
            if (textEditor.getName() == "sidebar::searcheditor") return;
            
            if (dynamic_cast<AlertWindow*>(textEditor.getParentComponent()) == nullptr)
            {
                if (textEditor.isEnabled())
                {
                    if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly())
                    {
                        g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                        g.drawRect(0, 0, width, height, 2);
                    }
                    else
                    {
                        g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                        g.drawRect(0, 0, width, height, 1);
                    }
                }
            }
        }
        
        void drawTreeviewPlusMinusBox(Graphics& g, const Rectangle<float>& area, Colour backgroundColour, bool isOpen, bool isMouseOver) override
        {
            Path p;
            p.addTriangle(0.0f, 0.0f, 1.0f, isOpen ? 0.0f : 0.5f, isOpen ? 0.5f : 0.0f, 1.0f);
            g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
            g.fillPath(p, p.getTransformToScaleToFit(area.reduced(2, area.getHeight() / 4), true));
        }
        
        void drawToolbarButton(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto rect = button.getLocalBounds();
            
            auto baseColour = findColour(ComboBox::backgroundColourId);
            g.setColour(baseColour);
            g.fillRect(rect);
        }
        
        void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override
        {
            bool inspectorElement = object.getName().startsWith("inspector");
            auto cornerSize = inspectorElement ? 0.0f : 3.0f;
            Rectangle<int> boxBounds(0, 0, width, height);
            
            g.setColour(object.findColour(ComboBox::backgroundColourId));
            g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
            
            if (!inspectorElement)
            {
                g.setColour(object.findColour(ComboBox::outlineColourId));
                g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
            }
            
            Rectangle<int> arrowZone(width - 20, 2, 14, height - 4);
            Path path;
            path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
            path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
            path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);
            g.setColour(object.findColour(ComboBox::arrowColourId).withAlpha((object.isEnabled() ? 0.9f : 0.2f)));
            
            g.strokePath(path, PathStrokeType(2.0f));
        }
        
        void drawStatusbarButton(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
        }
        
        void drawResizableFrame(Graphics& g, int w, int h, const BorderSize<int>& border) override
        {
        }
        
        void drawSuggestionButton(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto buttonArea = button.getLocalBounds();
            
            if (shouldDrawButtonAsDown)
            {
                g.setColour(backgroundColour.darker());
            }
            else if (shouldDrawButtonAsHighlighted)
            {
                g.setColour(backgroundColour.brighter());
            }
            else
            {
                g.setColour(backgroundColour);
            }
            
            g.fillRect(buttonArea.toFloat());
        }
        
        void drawInspectorButton(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
            
            auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
            
            if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
            
            if (!shouldDrawButtonAsHighlighted && !button.getToggleState()) baseColour = Colours::transparentBlack;
            
            g.setColour(baseColour);
            g.fillRect(bounds);
            g.setColour(button.findColour(ComboBox::outlineColourId));
        }
        
        void drawSuggestionButtonText(Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto font = getTextButtonFont(button, button.getHeight());
            g.setFont(font);
            g.setColour((button.getToggleState() ? Colours::white : findColour(TextButton::textColourOffId)).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
            auto yIndent = jmin(4, button.proportionOfHeight(0.3f));
            auto cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
            auto fontHeight = roundToInt(font.getHeight() * 0.6f);
            auto leftIndent = 28;
            auto rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            auto textWidth = button.getWidth() - leftIndent - rightIndent;
            
            if (textWidth > 0) g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::left, 2);
        }
        
        void drawStatusbarButtonText(Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            Font font(getTextButtonFont(button, button.getHeight()));
            g.setFont(font);
            
            if (!button.isEnabled())
            {
                g.setColour(Colours::grey);
            }
            else if (button.getToggleState())
            {
                g.setColour(button.findColour(Slider::thumbColourId));
            }
            else if (shouldDrawButtonAsHighlighted)
            {
                g.setColour(button.findColour(Slider::thumbColourId).brighter(0.8f));
            }
            else
            {
                g.setColour(button.findColour(TextButton::textColourOffId));
            }
            
            const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
            const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
            
            const int fontHeight = roundToInt(font.getHeight() * 0.6f);
            const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
            const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            const int textWidth = button.getWidth() - leftIndent - rightIndent;
            
            if (textWidth > 0) g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, 2);
        }
        
        void drawPdButton(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto cornerSize = 6.0f;
            auto bounds = button.getLocalBounds().toFloat();
            
            auto baseColour = findColour(TextButton::buttonColourId);
            
            auto highlightColour = findColour(TextButton::buttonOnColourId);
            
            if (shouldDrawButtonAsDown || button.getToggleState()) baseColour = highlightColour;
            
            baseColour = baseColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
            
            g.setColour(baseColour);
            
            auto flatOnLeft = button.isConnectedOnLeft();
            auto flatOnRight = button.isConnectedOnRight();
            auto flatOnTop = button.isConnectedOnTop();
            auto flatOnBottom = button.isConnectedOnBottom();
            
            if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
            {
                Path path;
                path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, !(flatOnLeft || flatOnTop), !(flatOnRight || flatOnTop), !(flatOnLeft || flatOnBottom), !(flatOnRight || flatOnBottom));
                
                g.fillPath(path);
                
                g.setColour(button.findColour(ComboBox::outlineColourId));
                g.strokePath(path, PathStrokeType(1.0f));
            }
            else
            {
                int dimension = std::min(bounds.getHeight(), bounds.getWidth()) / 2.0f;
                auto centre = bounds.getCentre();
                auto ellpiseBounds = Rectangle<float>(centre.translated(-dimension, -dimension), centre.translated(dimension, dimension));
                g.fillEllipse(ellpiseBounds);
                
                g.setColour(button.findColour(ComboBox::outlineColourId));
                g.drawEllipse(ellpiseBounds, 1.0f);
            }
        }
        
        void drawVolumeSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider)
        {
            float trackWidth = 6.;
            Point<float> startPoint(slider.isHorizontal() ? x : x + width * 0.5f, slider.isHorizontal() ? y + height * 0.5f : height + y);
            Point<float> endPoint(slider.isHorizontal() ? width + x : startPoint.x, slider.isHorizontal() ? startPoint.y : y);
            
            Path backgroundTrack;
            backgroundTrack.startNewSubPath(startPoint);
            backgroundTrack.lineTo(endPoint);
            g.setColour(slider.findColour(Slider::backgroundColourId));
            g.strokePath(backgroundTrack, {trackWidth, PathStrokeType::mitered});
            Path valueTrack;
            Point<float> minPoint, maxPoint;
            auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
            auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
            minPoint = startPoint;
            maxPoint = {kx, ky};
            auto thumbWidth = getSliderThumbRadius(slider);
            valueTrack.startNewSubPath(minPoint);
            valueTrack.lineTo(maxPoint);
            
            g.setColour(slider.findColour(TextButton::buttonColourId));
            g.strokePath(valueTrack, {trackWidth, PathStrokeType::mitered});
            g.setColour(slider.findColour(Slider::thumbColourId));
            
            g.fillRoundedRectangle(Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(24)).withCentre(maxPoint), 2.0f);
            
            g.setColour(findColour(ComboBox::backgroundColourId));
            g.drawRoundedRectangle(Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(24)).withCentre(maxPoint), 2.0f, 1.0f);
        }
        
        void drawPropertyPanelSectionHeader(Graphics& g, const String& name, bool isOpen, int width, int height) override
        {
            auto buttonSize = (float)height * 0.75f;
            auto buttonIndent = ((float)height - buttonSize) * 0.5f;
            
            drawTreeviewPlusMinusBox(g, {buttonIndent, buttonIndent, buttonSize, buttonSize}, findColour(ResizableWindow::backgroundColourId), isOpen, false);
            
            auto textX = static_cast<int>((buttonIndent * 2.0f + buttonSize + 2.0f));
            
            g.setColour(findColour(PropertyComponent::labelTextColourId));
            
            g.setFont({(float)height * 0.6f, Font::bold});
            g.drawText(name, textX, 0, width - textX - 4, height, Justification::centredLeft, true);
        }
        
        struct PdLook : public LookAndFeel_V4
        {
            PdLook()
            {
                // FIX THIS!
                setColour(TextButton::buttonColourId, Colour(23, 23, 23));
                setColour(TextButton::buttonOnColourId, Colour(0xff42a2c8));
                
                setColour(Slider::thumbColourId, Colour(0xff42a2c8));
                setColour(ComboBox::backgroundColourId, Colour(23, 23, 23));
                setColour(ListBox::backgroundColourId, Colour(23, 23, 23));
                setColour(Slider::backgroundColourId, Colour(60, 60, 60));
                setColour(Slider::trackColourId, Colour(90, 90, 90));
                
                setColour(TextEditor::backgroundColourId, Colour(45, 45, 45));
                setColour(TextEditor::textColourId, Colours::white);
                setColour(TextEditor::outlineColourId, findColour(ComboBox::outlineColourId));
                
                setColour(PlugDataColour::toolbarBackgroundColourId, findColour(ComboBox::backgroundColourId));
                setColour(PlugDataColour::canvasBackgroundColourId, findColour(ResizableWindow::backgroundColourId));
                setColour(PlugDataColour::toolbarTextColourId, findColour(ComboBox::textColourId));
                setColour(PlugDataColour::canvasTextColourId, findColour(ComboBox::textColourId));
                setColour(PlugDataColour::outlineColourId, findColour(ComboBox::outlineColourId).interpolatedWith(findColour(ComboBox::backgroundColourId), 0.5f));
                //            setColour(PlugDataColour::canvasOutlineColourId, findColour(ComboBox::outlineColourId));
            }
            
            int getSliderThumbRadius(Slider&) override
            {
                return 0;
            }
            
            void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override
            {
                if (dynamic_cast<AlertWindow*>(textEditor.getParentComponent()) == nullptr)
                {
                    if (textEditor.isEnabled())
                    {
                        if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly())
                        {
                            g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                            g.drawRect(0, 0, width, height, 2);
                        }
                        else
                        {
                            g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                            g.drawRect(0, 0, width, height, 1);
                        }
                    }
                }
            }
            
            void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
            {
                auto sliderBounds = slider.getLocalBounds().toFloat().reduced(1.0f);
                
                g.setColour(findColour(Slider::backgroundColourId));
                g.fillRect(sliderBounds);
                
                Path toDraw;
                if (slider.isHorizontal())
                {
                    sliderPos = jmap<float>(sliderPos, x, width - (2 * x), 1.0f, width);
                    auto b = sliderBounds.withTrimmedRight(width - sliderPos);
                    toDraw.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 1.0f, 1.0f, true, false, true, false);
                }
                else
                {
                    sliderPos = jmap<float>(sliderPos, y, height, 0.0f, height - 2.0f);
                    auto b = sliderBounds.withTrimmedTop(sliderPos);
                    toDraw.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 1.0f, 1.0f, false, false, true, true);
                }
                
                g.setColour(findColour(Slider::trackColourId));
                g.fillPath(toDraw);
            }
            
            void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
            {
                auto baseColour = button.findColour(TextButton::buttonColourId);
                
                auto highlightColour = button.findColour(TextButton::buttonOnColourId);
                
                Path path;
                path.addRectangle(button.getLocalBounds());
                
                g.setColour(baseColour);
                
                g.fillRect(button.getLocalBounds());
                
                g.setColour(button.findColour(ComboBox::outlineColourId));
                g.strokePath(path, PathStrokeType(1.0f));
                
                if (shouldDrawButtonAsDown || button.getToggleState())
                {
                    g.setColour(highlightColour);
                    g.fillRect(button.getLocalBounds().reduced(button.getWidth() * 0.25f));
                }
            }
        };
        
        void drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
        {
            Path corner;
            
            corner.addTriangle(Point<float>(0, h), Point<float>(w, h), Point<float>(w, 0));
            corner = corner.createPathWithRoundedCorners(2.0f);
            
            g.setColour(findColour(Slider::thumbColourId).withAlpha(isMouseOver ? 1.0f : 0.6f));
            g.fillPath(corner);
        }
        
        void drawTooltip(Graphics& g, const String& text, int width, int height) override
        {
            Rectangle<int> bounds(width, height);
            auto cornerSize = 5.0f;
            
            g.setColour(findColour(TooltipWindow::backgroundColourId));
            g.fillRoundedRectangle(bounds.toFloat(), cornerSize);
            
            g.setColour(findColour(TooltipWindow::outlineColourId));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 0.5f);
            
            const float tooltipFontSize = 13.0f;
            const int maxToolTipWidth = 400;
            
            AttributedString s;
            s.setJustification(Justification::centred);
            s.append(text, Font(tooltipFontSize, Font::bold), findColour(TooltipWindow::textColourId));
            
            TextLayout tl;
            tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
            tl.draw(g, {static_cast<float>(width), static_cast<float>(height)});
        }
        
        LookAndFeel* getPdLook()
        {
            return new PdLook;
        }
        
        static void paintStripes(Graphics& g, int itemHeight, int totalHeight, Component& owner, int selected, int offset, bool invert = false)
        {
            totalHeight += offset;
            int y = -offset;
            int i = 0;
            
            while (totalHeight)
            {
                if (totalHeight < itemHeight)
                {
                    itemHeight = totalHeight;
                }
                
                if (selected >= 0 && selected == i)
                {
                    g.setColour(owner.findColour(PlugDataColour::panelActiveBackgroundColourId));
                }
                else
                {
                    auto offColour = owner.findColour(PlugDataColour::panelBackgroundOffsetColourId);
                    auto onColour = owner.findColour(PlugDataColour::panelBackgroundColourId);
                    g.setColour((i + invert) & 1 ? onColour : offColour);
                }
                
                g.fillRect(0, y, owner.getWidth(), itemHeight);
                
                y += itemHeight;
                totalHeight -= itemHeight;
                i++;
            }
        }
        
        void setColours(std::map<PlugDataColour, Colour> colours)
        {
            for (auto colourId = 0; colourId < PlugDataColour::numberOfColours; colourId++) {
                setColour(colourId, colours.at(static_cast<PlugDataColour>(colourId)));
            }
            
            setColour(PopupMenu::highlightedBackgroundColourId,
                      colours.at(PlugDataColour::panelActiveBackgroundColourId));
            setColour(TextButton::textColourOnId,
                      colours.at(PlugDataColour::toolbarActiveColourId));
            setColour(Slider::thumbColourId,
                      colours.at(PlugDataColour::scrollbarBackgroundColourId));
            setColour(ScrollBar::thumbColourId,
                      colours.at(PlugDataColour::scrollbarBackgroundColourId));
            setColour(DirectoryContentsDisplayComponent::highlightColourId,
                      colours.at(PlugDataColour::panelActiveBackgroundColourId));
            // TODO: possibly add a colour for this
            setColour(CaretComponent::caretColourId,
                      colours.at(PlugDataColour::toolbarActiveColourId));
            
            setColour(TextButton::buttonColourId,
                      colours.at(PlugDataColour::toolbarBackgroundColourId));
            setColour(TextButton::buttonOnColourId,
                      colours.at(PlugDataColour::toolbarBackgroundColourId));
            setColour(ComboBox::backgroundColourId,
                      colours.at(PlugDataColour::toolbarBackgroundColourId));
            setColour(ListBox::backgroundColourId,
                      colours.at(PlugDataColour::toolbarBackgroundColourId));
            
            setColour(AlertWindow::backgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            getCurrentColourScheme().setUIColour(ColourScheme::UIColour::widgetBackground, colours.at(PlugDataColour::panelBackgroundColourId));
            
            setColour(TooltipWindow::backgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            setColour(PopupMenu::backgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            
            setColour(KeyMappingEditorComponent::backgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            setColour(ResizableWindow::backgroundColourId,
                      colours.at(PlugDataColour::canvasBackgroundColourId));
            setColour(Slider::backgroundColourId,
                      colours.at(PlugDataColour::canvasBackgroundColourId));
            setColour(Slider::trackColourId,
                      colours.at(PlugDataColour::scrollbarBackgroundColourId));
            setColour(TextEditor::backgroundColourId,
                      colours.at(PlugDataColour::canvasBackgroundColourId));
            setColour(FileBrowserComponent::currentPathBoxBackgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            setColour(FileBrowserComponent::filenameBoxBackgroundColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            
            setColour(TooltipWindow::textColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(TextButton::textColourOffId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(ComboBox::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(TableListBox::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(Label::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(Label::textWhenEditingColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(ListBox::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(TextEditor::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(PropertyComponent::labelTextColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(PopupMenu::textColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(KeyMappingEditorComponent::textColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(TabbedButtonBar::frontTextColourId,
                      colours.at(PlugDataColour::activeTabTextColourId));
            setColour(TabbedButtonBar::tabTextColourId,
                      colours.at(PlugDataColour::tabTextColourId));
            setColour(ToggleButton::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(ToggleButton::tickColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(ToggleButton::tickDisabledColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(ComboBox::arrowColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(DirectoryContentsDisplayComponent::textColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(Slider::textBoxTextColourId,
                      colours.at(PlugDataColour::canvasTextColourId));
            setColour(AlertWindow::textColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(FileBrowserComponent::currentPathBoxTextColourId,
                      colours.at(PlugDataColour::panelActiveTextColourId));
            setColour(FileBrowserComponent::currentPathBoxArrowColourId,
                      colours.at(PlugDataColour::panelActiveTextColourId));
            setColour(FileBrowserComponent::filenameBoxTextColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            setColour(FileChooserDialogBox::titleTextColourId,
                      colours.at(PlugDataColour::panelTextColourId));
            
            setColour(DirectoryContentsDisplayComponent::highlightedTextColourId,
                      colours.at(PlugDataColour::panelActiveTextColourId));
            
            setColour(TooltipWindow::outlineColourId,
                      colours.at(PlugDataColour::panelBackgroundColourId));
            setColour(ComboBox::outlineColourId,
                      colours.at(PlugDataColour::outlineColourId));
            setColour(TextEditor::outlineColourId,
                      colours.at(PlugDataColour::outlineColourId));
            
            setColour(Slider::textBoxOutlineColourId,
                      Colours::transparentBlack);
            setColour(TreeView::backgroundColourId,
                      Colours::transparentBlack);
        }
        
        static void setDefaultFont(String fontName)
        {
            auto& lnf = dynamic_cast<PlugDataLook&>(getDefaultLookAndFeel());
            if (fontName.isEmpty() || fontName == "Inter")
            {
                lnf.setDefaultSansSerifTypeface(lnf.defaultFont.getTypefacePtr());
            }
            else
            {
                auto newFont = Font(fontName, 15, Font::plain);
                lnf.setDefaultSansSerifTypeface(newFont.getTypefacePtr());
            }
        }
        
        inline static const std::map<PlugDataColour, Colour> defaultDarkTheme = {
            {PlugDataColour::toolbarBackgroundColourId, Colour(25, 25, 25)},
            {PlugDataColour::toolbarTextColourId, Colour(255, 255, 255)},
            {PlugDataColour::toolbarActiveColourId, Colour(66, 162, 200)},
            {PlugDataColour::outlineColourId, Colour(255, 255, 255)},
            
            {PlugDataColour::tabBackgroundColourId, Colour(25, 25, 25)},
            {PlugDataColour::tabTextColourId, Colour(255, 255, 255)},
            {PlugDataColour::tabBorderColourId, Colour(105, 105, 105)},
            {PlugDataColour::activeTabBackgroundColourId, Colour(35, 35, 35)},
            {PlugDataColour::activeTabTextColourId, Colour(255, 255, 255)},
            {PlugDataColour::activeTabBorderColourId, Colour(105, 105, 105)},
            
            {PlugDataColour::canvasBackgroundColourId, Colour(35, 35, 35)},
            {PlugDataColour::canvasTextColourId, Colour(255, 255, 255)},
            {PlugDataColour::canvasLockedOutlineColourId, Colour(255, 255, 255)},
            {PlugDataColour::canvasUnlockedOutlineColourId, Colour(255, 255, 255)},
            
            {PlugDataColour::outlineColourId, Colour(255, 255, 255)},
            {PlugDataColour::canvasActiveColourId, Colour(66, 162, 200)},
            {PlugDataColour::defaultObjectBackgroundColourId, Colour(25, 25, 25)},
            
            {PlugDataColour::dataColourId, Colour(66, 162, 200)},
            {PlugDataColour::connectionColourId, Colour(225, 225, 225)},
            {PlugDataColour::signalColourId, Colour(255, 133, 0)},
            
            {PlugDataColour::panelBackgroundColourId, Colour(35, 35, 35)},
            {PlugDataColour::panelBackgroundOffsetColourId, Colour(50, 50, 50)},
            {PlugDataColour::panelTextColourId, Colour(255, 255, 255)},
            {PlugDataColour::panelActiveBackgroundColourId, Colour(66, 162, 200)},
            {PlugDataColour::panelActiveTextColourId, Colour(0, 0, 0)},
            
            {PlugDataColour::scrollbarBackgroundColourId, Colour(66, 162, 200)},
            
            /* TODO: add toolbar outline, panel outline and canvas outline. add canvas locked and unlocked outline */
        };
        
        inline static const std::map<PlugDataColour, Colour> defaultLightTheme = {
            {PlugDataColour::toolbarBackgroundColourId, Colour(228, 228, 228)},
            {PlugDataColour::toolbarTextColourId, Colour(90, 90, 90)},
            {PlugDataColour::toolbarActiveColourId, Colour(0, 122, 255)},
            
            {PlugDataColour::tabBackgroundColourId, Colour(228, 228, 228)},
            {PlugDataColour::tabTextColourId, Colour(90, 90, 90)},
            {PlugDataColour::tabBorderColourId, Colour(168, 168, 168)},
            {PlugDataColour::activeTabBackgroundColourId, Colour(250, 250, 250)},
            {PlugDataColour::activeTabTextColourId, Colour(90, 90, 90)},
            {PlugDataColour::activeTabBorderColourId, Colour(168, 168, 168)},
            
            {PlugDataColour::canvasBackgroundColourId, Colour(250, 250, 250)},
            {PlugDataColour::canvasTextColourId, Colour(90, 90, 90)},
            {PlugDataColour::canvasActiveColourId, Colour(0, 122, 255)},
            {PlugDataColour::canvasLockedOutlineColourId, Colour(168, 168, 168)},
            {PlugDataColour::canvasUnlockedOutlineColourId, Colour(168, 168, 168)},
            
            {PlugDataColour::outlineColourId, Colour(168, 168, 168)},
            {PlugDataColour::dataColourId, Colour(0, 122, 255)},
            {PlugDataColour::connectionColourId, Colour(179, 179, 179)},
            {PlugDataColour::signalColourId, Colour(255, 133, 0)},
            {PlugDataColour::defaultObjectBackgroundColourId, Colour(228, 228, 228)},
            
            {PlugDataColour::panelBackgroundColourId, Colour(250, 250, 250)},
            {PlugDataColour::panelBackgroundOffsetColourId, Colour(228, 228, 228)},
            {PlugDataColour::panelTextColourId, Colour(90, 90, 90)},
            {PlugDataColour::panelActiveBackgroundColourId, Colour(0, 122, 255)},
            {PlugDataColour::panelActiveTextColourId, Colour(0, 0, 0)},
            
            {PlugDataColour::scrollbarBackgroundColourId, Colour(66, 162, 200)},
        };
        
        inline static const std::map<String, std::map<PlugDataColour, Colour>> defaultThemes = {
            {"light", defaultLightTheme},
            {"dark", defaultDarkTheme}
        };
        
        inline static std::map<String, std::map<PlugDataColour, Colour>> colourSettings = defaultThemes;
        
        void resetColours() {
            colourSettings = defaultThemes;
        }
        
        void setThemeColour(String themeName, PlugDataColour colourId, Colour colour) {
            colourSettings[themeName][colourId] = colour;
        }
        
        void setTheme(bool useLightTheme)
        {
            if (useLightTheme)
            {
                setColours(colourSettings.at("light"));
            }
            else
            {
                setColours(colourSettings.at("dark"));
            }
            
            isUsingLightTheme = useLightTheme;
        }
        
        // TODO: swap this out for a string theme name perhaps?
        static inline bool isUsingLightTheme = true;
        std::unique_ptr<Drawable> folderImage;
    };
