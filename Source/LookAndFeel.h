/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>


struct Resources
{
    Typeface::Ptr defaultTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterRegular_otf, BinaryData::InterRegular_otfSize);
    
    Typeface::Ptr iconTypeface = Typeface::createSystemTypefaceFor(BinaryData::PlugDataFont_ttf, BinaryData::PlugDataFont_ttfSize);
    
};

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
    inline static const CharPointer_UTF8 Inspector = CharPointer_UTF8("\xef\x87\x9e");
    inline static const CharPointer_UTF8 Console = CharPointer_UTF8("\xef\x84\xa0");
    inline static const CharPointer_UTF8 Clear = CharPointer_UTF8("\xef\x80\x8d");
    inline static const CharPointer_UTF8 Lock = CharPointer_UTF8("\xef\x80\xa3");
    inline static const CharPointer_UTF8 Unlock = CharPointer_UTF8("\xef\x82\x9c");
    inline static const CharPointer_UTF8 ConnectionStyle = CharPointer_UTF8("\xee\xa1\xbc");
    inline static const CharPointer_UTF8 Power = CharPointer_UTF8("\xef\x80\x91");
    inline static const CharPointer_UTF8 Audio = CharPointer_UTF8("\xef\x80\xa8");
    inline static const CharPointer_UTF8 Search = CharPointer_UTF8("\xef\x80\x82");
    inline static const CharPointer_UTF8 Wand = CharPointer_UTF8("\xef\x83\x90");
    
    inline static const CharPointer_UTF8 ZoomIn = CharPointer_UTF8("\xef\x80\x8e");
    inline static const CharPointer_UTF8 ZoomOut = CharPointer_UTF8 ("\xef\x80\x90");
    
    
    inline static const CharPointer_UTF8 AutoScroll = CharPointer_UTF8("\xef\x80\xb4");
    inline static const CharPointer_UTF8 Restore = CharPointer_UTF8("\xef\x83\xa2");
    inline static const CharPointer_UTF8 Error = CharPointer_UTF8("\xef\x81\xb1");
    inline static const CharPointer_UTF8 Message = CharPointer_UTF8("\xef\x81\xb5");
    
    
    
};



struct Canvas;
struct MainLook : public LookAndFeel_V4 {
    
    inline static DropShadow shadow = DropShadow(Colour{10, 10, 10}, 12, {0, 0});
    
    inline static Colour highlightColour = Colour(0xff42a2c8);
    inline static Colour firstBackground = Colour(23, 23, 23);
    inline static Colour secondBackground = Colour(32, 32, 32);
    Font defaultFont;
    
    MainLook(Resources& r) : defaultFont(r.defaultTypeface)
    {
        setColour(ResizableWindow::backgroundColourId, secondBackground);
        setColour(TextButton::buttonColourId, firstBackground);
        setColour(TextButton::buttonOnColourId, highlightColour);
        setColour(TextEditor::backgroundColourId, Colour(45, 45, 45));
        setColour(SidePanel::backgroundColour, Colour(50, 50, 50));
        setColour(ComboBox::backgroundColourId, firstBackground);
        setColour(ListBox::backgroundColourId, firstBackground);
        setColour(Slider::backgroundColourId, Colour(60, 60, 60));
        setColour(Slider::trackColourId, Colour(90, 90, 90));
        setColour(CodeEditorComponent::backgroundColourId, Colour(50, 50, 50));
        setColour(CodeEditorComponent::defaultTextColourId, Colours::white);
        setColour(TextEditor::textColourId, Colours::white);
        setColour(TooltipWindow::backgroundColourId, firstBackground.withAlpha(float(0.8)));
        
        setColour(PopupMenu::backgroundColourId, firstBackground.withAlpha(0.95f));
        setColour(PopupMenu::highlightedBackgroundColourId, highlightColour);
        
        setColour(CodeEditorComponent::lineNumberBackgroundId, Colour(41, 41, 41));
        
        //setColour(PopupMenu::backgroundColourId, firstBackground.withAlpha(0.95f));
        
        setDefaultSansSerifTypeface(r.defaultTypeface);
    }
    
    
    class PlugData_DocumentWindowButton : public Button {
    public:
        PlugData_DocumentWindowButton(const String& name, Colour c, const Path& normal, const Path& toggled)
        : Button(name)
        , colour(c)
        , normalShape(normal)
        , toggledShape(toggled)
        {
            
        }
        
        
        
        void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            
            auto background = MainLook::firstBackground;
            
            if (auto* rw = findParentComponentOfClass<ResizableWindow>())
                
                g.fillAll(background);
            
            g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f)
                        : colour);
            
            if (shouldDrawButtonAsHighlighted) {
                g.fillAll();
                g.setColour(background);
            }
            
            auto& p = getToggleState() ? toggledShape : normalShape;
            
            auto reducedRect = Justification(Justification::centred)
                .appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds())
                .toFloat()
                .reduced((float)getHeight() * 0.3f);
            
            g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
        }
        
    private:
        Colour colour;
        Path normalShape, toggledShape;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugData_DocumentWindowButton)
    };
    
    
    int getTabButtonBestWidth(TabBarButton& button, int tabDepth) override
    {
        auto& button_bar = button.getTabbedButtonBar();
        return button_bar.getWidth() / button_bar.getNumTabs();
    }
    
    void drawResizableFrame(Graphics& g, int w, int h, const BorderSize<int>& border) override
    {
    }
    
    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w,
                                    int h, int titleSpaceX, int titleSpaceW,
                                    const Image* icon,
                                    bool drawTitleTextOnLeft) override
    {
        if (w * h == 0)
            return;
        
        g.setColour(firstBackground);
        g.fillAll();
        
        Font font((float)h * 0.65f, Font::plain);
        g.setFont(font);
        
        g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::defaultText));
        
        g.setColour(Colours::white);
        g.drawText(window.getName(), 0, 0, w, h, Justification::centred,
                   true);
    }
    
    Button* createDocumentWindowButton(int buttonType) override
    {
        Path shape;
        auto crossThickness = 0.15f;
        
        if (buttonType == DocumentWindow::closeButton) {
            shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
            shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);
            
            return new PlugData_DocumentWindowButton("close", highlightColour, shape, shape);
        }
        
        if (buttonType == DocumentWindow::minimiseButton) {
            shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
            
            return new PlugData_DocumentWindowButton("minimise", highlightColour, shape, shape);
        }
        
        if (buttonType == DocumentWindow::maximiseButton) {
            shape.addLineSegment({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
            shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
            
            Path fullscreenShape;
            fullscreenShape.startNewSubPath(45.0f, 100.0f);
            fullscreenShape.lineTo(0.0f, 100.0f);
            fullscreenShape.lineTo(0.0f, 0.0f);
            fullscreenShape.lineTo(100.0f, 0.0f);
            fullscreenShape.lineTo(100.0f, 45.0f);
            fullscreenShape.addRectangle(45.0f, 45.0f, 100.0f, 100.0f);
            PathStrokeType(30.0f).createStrokedPath(fullscreenShape, fullscreenShape);
            
            return new PlugData_DocumentWindowButton("maximise", MainLook::highlightColour, shape, fullscreenShape);
        }
        
        jassertfalse;
        return nullptr;
    }
    
    void drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        g.setColour(button.getToggleState() ? Colour(55, 55, 55) : Colour(41, 41, 41));
        g.fillRect(button.getLocalBounds());
        
        g.setColour(Colour(120, 120, 120));
        g.drawLine(0, button.getHeight() - 1, button.getWidth(), button.getHeight() - 1);
        g.drawLine(button.getWidth(), 0, button.getWidth(), button.getHeight());
        
        drawTabButtonText(button, g, isMouseOver, isMouseDown);
    }
    
    Font getTabButtonFont(TabBarButton&, float height) override
    {
        return { height * 0.4f };
    }
    
    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        
        return Font(buttonHeight / 1.7f);
    }
    
    
    void drawPopupMenuBackground (Graphics& g, int width, int height) override
    {
        // Add a bit of alpha to disable the opaque flag
        auto background = findColour (PopupMenu::backgroundColourId);
        g.setColour(background);
        
        
        if(!Desktop::canUseSemiTransparentWindows() && JUCEApplicationBase::isStandaloneApp())  {
            auto bounds = Rectangle<float>(0, 0, width, height);
            g.fillRect(bounds);
            
            g.setColour (findColour (PopupMenu::textColourId).withAlpha (0.2f));
            g.drawRect(bounds, 1.0f);
            return;
        }
        
        
        auto bounds = Rectangle<float>(0, 0, width, height);
        g.fillRoundedRectangle(bounds, 5.0f);
        
        g.setColour (findColour (PopupMenu::textColourId).withAlpha (0.2f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
        
    }
    
};

struct PdGuiLook : public MainLook {
    
    PdGuiLook(Resources& r) : MainLook(r)
    {
        setColour(TextButton::buttonOnColourId, highlightColour);
        setColour(TextEditor::outlineColourId, findColour(ComboBox::outlineColourId));
    }
    
    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor)
    {
        if (dynamic_cast<AlertWindow*>(textEditor.getParentComponent()) == nullptr) {
            if (textEditor.isEnabled()) {
                if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly()) {
                    g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                    g.drawRect(0, 0, width, height, 2);
                } else {
                    g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                    g.drawRect(0, 0, width, height, 1);
                }
            }
        }
    }
    
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        
        auto cornerSize = 6.0f;
        auto bounds = button.getLocalBounds().toFloat(); //.reduced (0.5f, 0.5f);
        
        auto baseColour = findColour(TextButton::buttonColourId);
        
        auto highlightColour = findColour(TextButton::buttonOnColourId);
        
        if (shouldDrawButtonAsDown || button.getToggleState())
            baseColour = highlightColour;
        
        baseColour = baseColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
            .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
        
        g.setColour(baseColour);
        
        auto flatOnLeft = button.isConnectedOnLeft();
        auto flatOnRight = button.isConnectedOnRight();
        auto flatOnTop = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();
        
        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {
            Path path;
            path.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                     bounds.getWidth(), bounds.getHeight(),
                                     cornerSize, cornerSize,
                                     !(flatOnLeft || flatOnTop),
                                     !(flatOnRight || flatOnTop),
                                     !(flatOnLeft || flatOnBottom),
                                     !(flatOnRight || flatOnBottom));
            
            g.fillPath(path);
            
            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.strokePath(path, PathStrokeType(1.0f));
        } else {
            int dimension = std::min(bounds.getHeight(), bounds.getWidth()) / 2.0f;
            auto centre = bounds.getCentre();
            auto ellpiseBounds = Rectangle<float>(centre.translated(-dimension, -dimension), centre.translated(dimension, dimension));
            // g.fillRoundedRectangle (bounds, cornerSize);
            g.fillEllipse(ellpiseBounds);
            
            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.drawEllipse(ellpiseBounds, 1.0f);
        }
    }
};

struct ToolbarLook : public MainLook {
    
    Font iconFont = Font(Typeface::createSystemTypefaceFor(BinaryData::PlugDataFont_ttf, BinaryData::PlugDataFont_ttfSize));
    
    bool icons;
    
    ToolbarLook(Resources& r) : MainLook(r), iconFont(r.iconTypeface)
    {
    }
    
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        
        auto rect = button.getLocalBounds();
        
        auto base_colour = firstBackground;
        
        auto highlightColour = MainLook::highlightColour;
        
        if (shouldDrawButtonAsHighlighted || button.getToggleState())
            highlightColour = highlightColour.brighter(0.4f);
        
        if (shouldDrawButtonAsDown)
            highlightColour = highlightColour.brighter(0.2f);
        else
            highlightColour = highlightColour;
        
        g.setColour(base_colour);
        g.fillRect(rect);
        
        auto highlight_rect = Rectangle<float>(rect.getX(), rect.getY() + rect.getHeight() - 8, rect.getWidth(), 4);
        
        g.setColour(highlightColour);
        g.fillRect(highlight_rect);
    }
    
    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        return iconFont.withHeight(buttonHeight / 3.6);
    }
};

struct StatusbarLook : public MainLook {
    Font iconFont;
    StatusbarLook(Resources& r) : MainLook(r), iconFont(r.iconTypeface)
    {
        setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
        
        setColour(TextButton::textColourOnId, highlightColour);
        setColour(TextButton::textColourOffId, Colours::white);
        setColour(TextButton::buttonOnColourId, findColour(TextButton::buttonColourId));
        
        setColour(Slider::trackColourId, firstBackground);
    }
    
    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        return iconFont.withHeight(buttonHeight / 2.25);
    }
    
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        g.setColour(backgroundColour);
        g.fillRect(button.getLocalBounds());
    }
    
    void drawButtonText (Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        Font font (getTextButtonFont (button, button.getHeight()));
        g.setFont (font);
        
        auto colour = button.findColour((button.getToggleState() || shouldDrawButtonAsHighlighted)? TextButton::textColourOnId : TextButton::textColourOffId);
        
        g.setColour ((shouldDrawButtonAsHighlighted && !button.getToggleState()) ? colour.brighter(0.8f) : colour);
        
        if(!button.isEnabled()) {
            g.setColour(Colours::grey);
        }
        
        const int yIndent = jmin (4, button.proportionOfHeight (0.3f));
        const int cornerSize = jmin (button.getHeight(), button.getWidth()) / 2;

        const int fontHeight = roundToInt (font.getHeight() * 0.6f);
        const int leftIndent  = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        const int rightIndent = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        const int textWidth = button.getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText (button.getButtonText(),
                              leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                              Justification::centred, 2);
    }
    
    
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          const Slider::SliderStyle style, Slider& slider)
    {
        float trackWidth = 6.;
        Point<float> startPoint(slider.isHorizontal() ? x : x + width * 0.5f,
                                slider.isHorizontal() ? y + height * 0.5f : height + y);
        Point<float> endPoint(slider.isHorizontal() ? width + x : startPoint.x,
                              slider.isHorizontal() ? startPoint.y : y);
        Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);
        g.setColour(slider.findColour(Slider::backgroundColourId));
        g.strokePath(backgroundTrack, { trackWidth, PathStrokeType::mitered });
        Path valueTrack;
        Point<float> minPoint, maxPoint;
        auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
        auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
        minPoint = startPoint;
        maxPoint = { kx, ky };
        auto thumbWidth = getSliderThumbRadius(slider);
        valueTrack.startNewSubPath(minPoint);
        valueTrack.lineTo(maxPoint);
        g.setColour(slider.findColour(Slider::trackColourId));
        g.strokePath(valueTrack, { trackWidth, PathStrokeType::mitered });
        g.setColour(slider.findColour(Slider::thumbColourId));
        g.fillRect(Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(24)).withCentre(maxPoint));
    }
    
    int getSliderThumbRadius(Slider&)
    {
        return 6;
    }
};

class BoxEditorLook : public MainLook {
public:
     BoxEditorLook(Resources& r) : MainLook(r)
     {
     
     }
    
    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
    {
        
        auto font = getTextButtonFont(button, button.getHeight());
        g.setFont(font);
        g.setColour(button.findColour(button.getToggleState() ? TextButton::textColourOnId
                                      : TextButton::textColourOffId)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
        auto yIndent = jmin(4, button.proportionOfHeight(0.3f));
        auto cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
        auto fontHeight = roundToInt(font.getHeight() * 0.6f);
        auto leftIndent = 28;
        auto rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        auto textWidth = button.getWidth() - leftIndent - rightIndent;
        
        if (textWidth > 0)
            g.drawFittedText(button.getButtonText(),
                             leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                             Justification::left, 2);
    }
    
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        
        auto buttonArea = button.getLocalBounds();
        
        if (shouldDrawButtonAsDown) {
            g.setColour(backgroundColour.darker());
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour(backgroundColour.brighter());
        } else {
            g.setColour(backgroundColour);
        }
        
        g.fillRect(buttonArea.toFloat());
    }
    
    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        return Font(buttonHeight / 1.9f);
    }
};

