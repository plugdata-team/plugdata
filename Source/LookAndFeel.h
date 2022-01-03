/*
 // Copyright (c) 2021 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

struct MainLook : public LookAndFeel_V4 {

    inline static DropShadow shadow = DropShadow(Colour{10, 10, 10}, 12, {0, 0});
    
    inline static Colour highlightColour = Colour(0xff42a2c8);
    inline static Colour firstBackground = Colour(23, 23, 23);
    inline static Colour secondBackground = Colour(32, 32, 32);
    
    Typeface::Ptr typeface = Typeface::createSystemTypefaceFor(BinaryData::InterRegular_otf, BinaryData::InterRegular_otfSize);
    Font defaultFont = Font(typeface);
    
    MainLook()
    {
        setColour(PopupMenu::backgroundColourId, firstBackground);
        setColour(ResizableWindow::backgroundColourId, secondBackground);
        setColour(TextButton::buttonColourId, firstBackground);
        setColour(TextButton::buttonOnColourId, highlightColour);
        setColour(juce::TextEditor::backgroundColourId, Colour(45, 45, 45));
        setColour(SidePanel::backgroundColour, Colour(50, 50, 50));
        setColour(ComboBox::backgroundColourId, firstBackground);
        setColour(ListBox::backgroundColourId, firstBackground);
        setColour(Slider::backgroundColourId, Colour(60, 60, 60));
        setColour(Slider::trackColourId, Colour(50, 50, 50));
        setColour(CodeEditorComponent::backgroundColourId, Colour(50, 50, 50));
        setColour(CodeEditorComponent::defaultTextColourId, Colours::white);
        setColour(TextEditor::textColourId, Colours::white);
        setColour(TooltipWindow::backgroundColourId, firstBackground.withAlpha(float(0.8)));

        setColour(PopupMenu::backgroundColourId, secondBackground);
        setColour(PopupMenu::highlightedBackgroundColourId, highlightColour);

        setColour(CodeEditorComponent::lineNumberBackgroundId, Colour(41, 41, 41));
        
        setDefaultSansSerifTypeface(typeface);
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

        auto isActive = window.isActiveWindow();

        g.setColour(firstBackground);
        g.fillAll();

        Font font((float)h * 0.65f, Font::plain);
        g.setFont(font);

        auto textW = font.getStringWidth(window.getName());
        auto iconW = 0;
        auto iconH = 0;

        if (icon != nullptr) {
            iconH = static_cast<int>(font.getHeight());
            iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
        }

        textW = jmin(titleSpaceW, textW + iconW);
        auto textX = drawTitleTextOnLeft ? titleSpaceX : jmax(titleSpaceX, (w - textW) / 2);

        if (textX + textW > titleSpaceX + titleSpaceW)
            textX = titleSpaceX + titleSpaceW - textW;

        if (icon != nullptr) {
            g.setOpacity(isActive ? 1.0f : 0.6f);
            g.drawImageWithin(*icon, textX, (h - iconH) / 2, iconW, iconH,
                RectanglePlacement::centred, false);
            textX += iconW;
            textW -= iconW;
        }

        if (window.isColourSpecified(DocumentWindow::textColourId) || isColourSpecified(DocumentWindow::textColourId))
            g.setColour(window.findColour(DocumentWindow::textColourId));
        else
            g.setColour(
                getCurrentColourScheme().getUIColour(ColourScheme::defaultText));

        g.setColour(Colours::white);
        g.drawText(window.getName(), textX, 0, textW, h, Justification::centredLeft,
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
    
    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        
        return Font(buttonHeight / 2.0f);
    }
};

struct PdGuiLook : public MainLook {

    PdGuiLook()
    {
        setColour(TextButton::buttonOnColourId, highlightColour);
        // setColour(TextButton::buttonColourId, Colours::transparentBlack);

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

    Font icon_font = Font(Typeface::createSystemTypefaceFor(BinaryData::forkawesomewebfont_ttf, BinaryData::forkawesomewebfont_ttfSize));

    bool icons;

    ToolbarLook(bool use_icons = true)
    {
        icons = use_icons;
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
        icon_font.setHeight(buttonHeight / 3.6);
        return icons ? icon_font : Font(jmin(15.0f, (float)buttonHeight * 0.6f));
    }
};

struct StatusbarLook : public MainLook {
    float scalar;
    Font icon_font = Font(Typeface::createSystemTypefaceFor(BinaryData::fontaudio_ttf, BinaryData::fontaudio_ttfSize));

    
    Font extra_icon_font = Font(Typeface::createSystemTypefaceFor(BinaryData::forkawesomewebfont_ttf, BinaryData::forkawesomewebfont_ttfSize));

    bool font_choice;

    StatusbarLook(bool font = true, float button_scalar = 1.0f)
    {
        scalar = button_scalar;
        font_choice = font;

        setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));

        setColour(TextButton::textColourOnId, highlightColour);
        setColour(TextButton::textColourOffId, Colours::white);
        setColour(TextButton::buttonOnColourId, Colour(10, 10, 10));

        setColour(Slider::trackColourId, firstBackground);
    }

    Font getTextButtonFont(TextButton&, int buttonHeight)
    {
        auto font = font_choice ? icon_font.withHeight(buttonHeight / (3.2 / scalar)) : extra_icon_font.withHeight(buttonHeight / (3.8 / scalar));
        return font;
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
    // BoxEditorLook();

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
        auto leftIndent = 25;
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
};
