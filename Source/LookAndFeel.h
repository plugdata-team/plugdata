#pragma once

#include <JuceHeader.h>


struct MainLook : public LookAndFeel_V4
{
    MainLook() {
        setColour(PopupMenu::backgroundColourId, Colour(25, 25, 25));
        setColour (ResizableWindow::backgroundColourId, Colour(50, 50, 50));
        setColour (TextButton::buttonColourId, Colour(31, 31, 31));
        setColour (TextButton::buttonOnColourId, Colour(25, 25, 25));
        setColour (juce::TextEditor::backgroundColourId, Colour(68, 68, 68));
        setColour (SidePanel::backgroundColour, Colour(50, 50, 50));
        setColour (ComboBox::backgroundColourId, Colour(50, 50, 50));
        setColour (ListBox::backgroundColourId, Colour(50, 50, 50));
        setColour (Slider::backgroundColourId, Colour(60, 60, 60));
        setColour (Slider::trackColourId, Colour(50, 50, 50));
        setColour(CodeEditorComponent::backgroundColourId, Colour(50, 50, 50));
        setColour(CodeEditorComponent::defaultTextColourId, Colours::white);
        setColour(TextEditor::textColourId, Colours::white);
        setColour(TooltipWindow::backgroundColourId, Colour(25, 25, 25).withAlpha(float(0.8)));
        setColour (PopupMenu::backgroundColourId, Colour(50, 50, 50));
        setColour (PopupMenu::highlightedBackgroundColourId, Colour(41, 41, 41));
        
        setColour(CodeEditorComponent::lineNumberBackgroundId, Colour(41, 41, 41));
    }
    
    int getTabButtonBestWidth(TabBarButton& button, int tabDepth) override {
        auto& button_bar = button.getTabbedButtonBar();
       
        return button_bar.getWidth() / button_bar.getNumTabs();
    }
    
    void drawTabButton (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        /*
        Path tabShape;
        createTabButtonShape (button, tabShape, isMouseOver, isMouseDown);

        auto activeArea = button.getActiveArea();
        tabShape.applyTransform (AffineTransform::translation ((float) activeArea.getX(),
                                                               (float) activeArea.getY()));

        DropShadow (Colours::black.withAlpha (0.5f), 2, Point<int> (0, 1)).drawForPath (g, tabShape);

        fillTabButtonShape (button, g, tabShape, isMouseOver, isMouseDown);
        */
        
        
       
        g.setColour(button.getToggleState() ? Colour(55, 55, 55) : Colour(41, 41, 41));
        g.fillRect(button.getLocalBounds());
        
        g.setColour(Colour(120, 120, 120));
        g.drawLine(0, button.getHeight() - 1, button.getWidth(), button.getHeight() - 1);
        g.drawLine(button.getWidth(), 0, button.getWidth(), button.getHeight());
        
        drawTabButtonText (button, g, isMouseOver, isMouseDown);
    }
    
    
    Font getTabButtonFont (TabBarButton&, float height) override
    {
        return { height * 0.4f };
    }
    
    
};

struct PdGuiLook : public MainLook
{
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour,  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
        
        auto cornerSize = 6.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);

        auto baseColour = backgroundColour;

        auto highlight_colour = Colour (0xff42a2c8);
        
        if (shouldDrawButtonAsDown)
            baseColour = highlight_colour;
        
        baseColour = baseColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                              .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

        g.setColour (baseColour);

        auto flatOnLeft   = button.isConnectedOnLeft();
        auto flatOnRight  = button.isConnectedOnRight();
        auto flatOnTop    = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
        {
            Path path;
            path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                      bounds.getWidth(), bounds.getHeight(),
                                      cornerSize, cornerSize,
                                      ! (flatOnLeft  || flatOnTop),
                                      ! (flatOnRight || flatOnTop),
                                      ! (flatOnLeft  || flatOnBottom),
                                      ! (flatOnRight || flatOnBottom));

            g.fillPath (path);

            g.setColour (button.findColour (ComboBox::outlineColourId));
            g.strokePath (path, PathStrokeType (1.0f));
        }
        else
        {
            g.fillRoundedRectangle (bounds, cornerSize);

            g.setColour (button.findColour (ComboBox::outlineColourId));
            g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
        }
        
    }
    
};

struct ToolbarLook : public MainLook
{
    
    inline static Font icon_font = Font(Typeface::createSystemTypefaceFor (BinaryData::cerite_font_ttf, BinaryData::cerite_font_ttfSize));
    
    bool icons;
    
    ToolbarLook(bool use_icons = true) {
        icons = use_icons;
    }
    
    void drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour,  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
        
        auto rect = button.getLocalBounds();
        
        auto base_colour = Colour(31, 31, 31);
        
        auto highlight_colour = Colour (0xff42a2c8);
        
        if(shouldDrawButtonAsHighlighted || button.getToggleState())
            highlight_colour = highlight_colour.brighter (0.4f);
        
        if(shouldDrawButtonAsDown)
            highlight_colour = highlight_colour.brighter (0.1f);
        else
            highlight_colour = highlight_colour.darker(0.2f);
        
        
        g.setColour(base_colour);
        g.fillRect(rect);
        
        auto highlight_rect = Rectangle<float>(rect.getX(), rect.getY() + rect.getHeight() - 8, rect.getWidth(), 4);
        
        g.setColour(highlight_colour);
        g.fillRect(highlight_rect);
        
    }
    
    
    Font getTextButtonFont (TextButton&, int buttonHeight)
    {
        icon_font.setHeight(buttonHeight / 3.6);
        return icons ? icon_font : Font (jmin (15.0f, (float) buttonHeight * 0.6f));
    }
};

struct SidebarLook : public MainLook
{
    float scalar;
    
    SidebarLook(float button_scalar = 1.0f) {
        scalar = button_scalar;
        setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    }
    
    Font getTextButtonFont (TextButton&, int buttonHeight)
    {
        ToolbarLook::icon_font.setHeight(buttonHeight / (3.8 / scalar));
        return ToolbarLook::icon_font;
    }
    
    
    
};
