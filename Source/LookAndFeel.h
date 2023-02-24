/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>
#include <map>

#include "Utility/StackShadow.h"
#include "Utility/SettingsFile.h"

struct Icons {
    inline static const String Open = "b";
    inline static const String Save = "c";
    inline static const String SaveAs = "d";
    inline static const String Undo = "e";
    inline static const String Redo = "f";
    inline static const String Add = "g";
    inline static const String Settings = "h";
    inline static const String Hide = "i";
    inline static const String Show = "i";
    inline static const String Clear = "k";
    inline static const String ClearLarge = "l";
    inline static const String Lock = "m";
    inline static const String Unlock = "n";
    inline static const String ConnectionStyle = "o";
    inline static const String Power = "p";
    inline static const String Audio = "q";
    inline static const String Search = "r";
    inline static const String Wand = "s";
    inline static const String Pencil = "t";
    inline static const String Grid = "u";
    inline static const String Pin = "v";
    inline static const String Keyboard = "w";
    inline static const String Folder = "x";
    inline static const String OpenedFolder = "y";
    inline static const String File = "z";
    inline static const String New = "z";
    inline static const String AutoScroll = "A";
    inline static const String Restore = "B";
    inline static const String Error = "C";
    inline static const String Message = "D";
    inline static const String Parameters = "E";
    inline static const String Presentation = "F";
    inline static const String Externals = "G";
    inline static const String Refresh = "H";
    inline static const String Up = "I";
    inline static const String Down = "J";
    inline static const String Edit = "K";
    inline static const String ThinDown = "L";
    inline static const String Sine = "M";
    inline static const String Documentation = "N";
    inline static const String AddCircled = "O";
    inline static const String Console = "P";
    inline static const String GitHub = "Q";
    inline static const String Wrench = "R";
    inline static const String Back = "S";
    inline static const String Forward = "T";
    inline static const String Library = "U";
    inline static const String Menu = "V";
    inline static const String Info = "W";
    inline static const String History = "X";
    inline static const String Protection = "Y";
};

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,

    tabBackgroundColourId,
    tabTextColourId,
    activeTabBackgroundColourId,
    activeTabTextColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    guiObjectBackgroundColourId,
    textObjectBackgroundColourId,

    objectOutlineColourId,
    objectSelectedOutlineColourId,
    outlineColourId,

    ioletAreaColourId,
    ioletOutlineColourId,

    dataColourId,
    connectionColourId,
    signalColourId,

    dialogBackgroundColourId,

    sidebarBackgroundColourId,
    sidebarTextColourId,
    sidebarActiveBackgroundColourId,
    sidebarActiveTextColourId,

    levelMeterActiveColourId,
    levelMeterInactiveColourId,
    levelMeterTrackColourId,
    levelMeterThumbColourId,

    panelBackgroundColourId,
    panelTextColourId,
    panelActiveBackgroundColourId,
    panelActiveTextColourId,

    popupMenuBackgroundColourId,
    popupMenuActiveBackgroundColourId,
    popupMenuTextColourId,
    popupMenuActiveTextColourId,

    sliderThumbColourId,
    scrollbarThumbColourId,
    resizeableCornerColourId,
    gridLineColourId,
    caretColourId,

    /* iteration hack */
    numberOfColours
};

inline const std::map<PlugDataColour, std::tuple<String, String, String>> PlugDataColourNames = {

    { toolbarBackgroundColourId, { "Toolbar Background", "toolbar_background", "Toolbar" } },
    { toolbarTextColourId, { "Toolbar Text", "toolbar_text", "Toolbar" } },
    { toolbarActiveColourId, { "Toolbar Active", "toolbar_active", "Toolbar" } },

    { tabBackgroundColourId, { "Tab Background", "tab_background", "Tabbar" } },

    { tabTextColourId, { "Tab Text", "tab_text", "Tabbar" } },
    { activeTabBackgroundColourId, { "Active Tab Background", "active_tab_background", "Tabbar" } },
    { activeTabTextColourId, { "Active Tab Text", "active_tab_text", "Tabbar" } },

    { canvasBackgroundColourId, { "Canvas Background", "canvas_background", "Canvas" } },
    { canvasTextColourId, { "Canvas Text", "canvas_text", "Canvas" } },
    { canvasDotsColourId, { "Canvas Dots Colour", "canvas_dots", "Canvas" } },
    { outlineColourId, { "Outline Colour", "outline_colour", "Canvas" } },

    { guiObjectBackgroundColourId, { "GUI Object Background", "default_object_background", "Object" } },
    { textObjectBackgroundColourId, { "Text Object Background", "text_object_background", "Object" } },
    { objectOutlineColourId, { "Object outline colour", "object_outline_colour", "Object" } },
    { objectSelectedOutlineColourId, { "Selected object outline colour", "selected_object_outline_colour", "Object" } },

    { ioletAreaColourId, { "Inlet/Outlet Area Colour", "iolet_area_colour", "Inlet/Outlet" } },
    { ioletOutlineColourId, { "Inlet/Outlet Outline Colour", "iolet_outline_colour", "Inlet/Outlet" } },

    { dataColourId, { "Data Colour", "data_colour", "Canvas" } },
    { connectionColourId, { "Connection Colour", "connection_colour", "Canvas" } },
    { signalColourId, { "Signal Colour", "signal_colour", "Canvas" } },
    { resizeableCornerColourId, { "Graph Resizer", "graph_resizer", "Canvas" } },
    { gridLineColourId, { "Grid Line Colour", "grid_colour", "Canvas" } },

    { popupMenuBackgroundColourId, { "Popup Menu Background", "popup_background", "Popup Menu" } },
    { popupMenuActiveBackgroundColourId, { "Popup Menu Background Active", "popup_background_active", "Popup Menu" } },
    { popupMenuTextColourId, { "Popup Menu Text", "popup_text", "Popup Menu" } },
    { popupMenuActiveTextColourId, { "Popup Menu Active Text", "popup_active_text", "Popup Menu" } },

    { dialogBackgroundColourId, { "Dialog Background", "dialog_background", "Other" } },
    { caretColourId, { "Text Editor Caret", "caret_colour", "Other" } },

    { levelMeterActiveColourId, { "Level Meter Active", "levelmeter_active", "Level Meter" } },
    { levelMeterInactiveColourId, { "Level Meter inactive", "levelmeter_inactive", "Level Meter" } },

    { levelMeterTrackColourId, { "Level Meter Track", "levelmeter_track", "Level Meter" } },
    { levelMeterThumbColourId, { "Level Meter Thumb", "levelmeter_thumb", "Level Meter" } },

    { sliderThumbColourId, { "Slider Thumb", "slider_thumb", "Other" } },
    { scrollbarThumbColourId, { "Scrollbar Thumb", "scrollbar_thumb", "Other" } },

    { panelBackgroundColourId, { "Panel Background", "panel_colour", "Panel" } },
    { panelTextColourId, { "Panel Text", "panel_text", "Panel" } },
    { panelActiveBackgroundColourId, { "Panel Background Active", "panel_background_active", "Panel" } },
    { panelActiveTextColourId, { "Panel Active Text", "panel_active_text", "Panel" } },

    { sidebarBackgroundColourId, { "Sidebar Background", "sidebar_colour", "Sidebar" } },
    { sidebarTextColourId, { "Sidebar Text", "sidebar_text", "Sidebar" } },
    { sidebarActiveBackgroundColourId, { "Sidebar Background Active", "sidebar_background_active", "Sidebar" } },
    { sidebarActiveTextColourId, { "Sidebar Active Text", "sidebar_active_text", "Sidebar" } },
};

struct Fonts {
    Fonts()
    {
        Typeface::setTypefaceCacheSize(7);

        // jassert(!instance);

        // Our unicode font is too big, the compiler will run out of memory
        // To prevent this, we split the BinaryData into multiple files, and add them back together here
        std::vector<char> interUnicode;
        int i = 0;
        while (true) {
            int size;
            auto* resource = BinaryData::getNamedResource((String("InterUnicode_") + String(i) + "_ttf").toRawUTF8(), size);

            if (!resource) {
                break;
            }

            interUnicode.insert(interUnicode.end(), resource, resource + size);
            i++;
        }

        // Initialise typefaces
        defaultTypeface = Typeface::createSystemTypefaceFor(interUnicode.data(), interUnicode.size());
        currentTypeface = defaultTypeface;

        thinTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize);

        boldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);

        semiBoldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);

        iconTypeface = Typeface::createSystemTypefaceFor(BinaryData::IconFont_ttf, BinaryData::IconFont_ttfSize);

        monoTypeface = Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMono_ttf, BinaryData::IBMPlexMono_ttfSize);

        instance = this;
    }

    static Font getCurrentFont() { return Font(instance->currentTypeface); }
    static Font getDefaultFont() { return Font(instance->defaultTypeface); }
    static Font getBoldFont() { return Font(instance->boldTypeface); }
    static Font getSemiBoldFont() { return Font(instance->semiBoldTypeface); }
    static Font getThinFont() { return Font(instance->thinTypeface); }
    static Font getIconFont() { return Font(instance->iconTypeface); }
    static Font getMonospaceFont() { return Font(instance->monoTypeface); }

    static Font setCurrentFont(Font font) { return instance->currentTypeface = font.getTypefacePtr(); }

private:
    // This is effectively a singleton because it's loaded through SharedResourcePointer
    static inline Fonts* instance = nullptr;

    // Default typeface is Inter combined with Unicode symbols from GoNotoUniversal and emojis from NotoEmoji
    Typeface::Ptr defaultTypeface;

    Typeface::Ptr currentTypeface;

    Typeface::Ptr thinTypeface;
    Typeface::Ptr boldTypeface;
    Typeface::Ptr semiBoldTypeface;
    Typeface::Ptr iconTypeface;
    Typeface::Ptr monoTypeface;
};

enum FontStyle {
    Regular,
    Bold,
    Semibold,
    Thin,
    Monospace,
};

struct PlugDataLook : public LookAndFeel_V4 {

    // Makes sure fonts get initialised
    SharedResourcePointer<Fonts> fonts;

    PlugDataLook()
    {
        setDefaultSansSerifTypeface(Fonts::getCurrentFont().getTypefacePtr());
    }

    class PlugData_DocumentWindowButton : public Button {
    public:
        PlugData_DocumentWindowButton(String const& name, Path normal, Path toggled)
            : Button(name)
            , normalShape(std::move(normal))
            , toggledShape(std::move(toggled))
        {
        }

        void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto colour = findColour(TextButton::textColourOffId);

            g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f) : colour);

            if (shouldDrawButtonAsHighlighted) {
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

        if (s.getProperties()["Style"] == "VolumeSlider") {
            return 6;
        }
        return LookAndFeel_V4::getSliderThumbRadius(s);
    }

    void fillResizableWindowBackground(Graphics& g, int w, int h, BorderSize<int> const& border, ResizableWindow& window) override
    {
        if (auto* dialog = dynamic_cast<FileChooserDialogBox*>(&window)) {
            g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));
        }
    }

    void drawResizableWindowBorder(Graphics&, int w, int h, BorderSize<int> const& border, ResizableWindow&) override
    {
    }

    void drawButtonBackground(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (button.getProperties()["Style"].toString().contains("Icon")) {
            return;
        } else {
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override
    {
        if (button.getProperties()["Style"] == "LargeIcon") {
            button.setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
            button.setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));

            LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
        } else if (button.getProperties()["Style"] == "SmallIcon") {
            Font font(getTextButtonFont(button, button.getHeight()));
            g.setFont(font);

            if (!button.isEnabled()) {
                g.setColour(Colours::grey);
            } else if (button.getToggleState()) {
                g.setColour(button.findColour(TextButton::textColourOnId));
            } else if (isMouseOverButton) {
                g.setColour(button.findColour(TextButton::textColourOnId).brighter(0.8f));
            } else {
                g.setColour(button.findColour(TextButton::textColourOffId));
            }

            int const yIndent = jmin(4, button.proportionOfHeight(0.3f));
            int const cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

            int const fontHeight = roundToInt(font.getHeight() * 0.6f);
            int const leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
            int const rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            int const textWidth = button.getWidth() - leftIndent - rightIndent;

            if (textWidth > 0)
                g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, 2);
        } else {
            Font font(getTextButtonFont(button, button.getHeight()));
            g.setFont(font);
            auto colour = button.findColour(button.getToggleState() ? TextButton::textColourOnId
                                                                    : TextButton::textColourOffId)
                              .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

            int const yIndent = jmin(4, button.proportionOfHeight(0.3f));
            int const cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

            int const fontHeight = roundToInt(font.getHeight() * 0.6f);
            int const leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
            int const rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            int const textWidth = button.getWidth() - leftIndent - rightIndent;

            g.setColour(colour);

            if (textWidth > 0) {
                g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, 1);
            }
        }
    }

    Font getTextButtonFont(TextButton& but, int buttonHeight) override
    {

        if (!but.getProperties()["FontScale"].isVoid()) {
            float scale = static_cast<float>(but.getProperties()["FontScale"]);
            if (but.getProperties()["Style"] == "Icon") {

                return Fonts::getIconFont().withHeight(buttonHeight * scale);
            } else {
                return Font(buttonHeight * scale);
            }
        }
        if (but.getProperties()["Style"] == "SmallIcon") {
            return Fonts::getIconFont().withHeight(buttonHeight * 0.44f);
        }
        // For large buttons, the icon should actually be smaller
        if (but.getProperties()["Style"] == "LargeIcon") {
            return Fonts::getIconFont().withHeight(buttonHeight * 0.34f);
        }

        return { buttonHeight / 1.7f };
    }

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        if (slider.getProperties()["Style"] == "VolumeSlider") {
            drawVolumeSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        } else if (slider.getProperties()["Style"] == "SliderObject") {
            drawGUIObjectSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, slider);
        } else {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    // TODO: do we use this??
    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, Image const* icon, bool drawTitleTextOnLeft) override
    {
        if (w * h == 0)
            return;

        g.setColour(findColour(ComboBox::backgroundColourId));
        g.fillAll();

        PlugDataLook::drawText(g, window.getName(), 0, 0, w, h, getCurrentColourScheme().getUIColour(ColourScheme::defaultText), h * 0.65f);
    }

    Button* createDocumentWindowButton(int buttonType) override
    {
        Path shape;
        auto crossThickness = 0.15f;

        if (buttonType == DocumentWindow::closeButton) {
            shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
            shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);

            return new PlugData_DocumentWindowButton("close", shape, shape);
        }

        if (buttonType == DocumentWindow::minimiseButton) {
            shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

            return new PlugData_DocumentWindowButton("minimise", shape, shape);
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

        if (button.getIndex() != button.getTabbedButtonBar().getNumTabs() - 1) {
            g.setColour(button.findColour(PlugDataColour::outlineColourId));
            g.drawLine(Line<float>(w - 0.5f, 0, w - 0.5f, h), 1.0f);
        }

        drawTabButtonText(button, g, isMouseOver, isMouseDown);
    }

    void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, int const w, int const h) override
    {
    }

    Font getTabButtonFont(TabBarButton&, float height) override
    {
        return Fonts::getCurrentFont().withHeight(height * 0.47f);
    }

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override
    {

        auto background = findColour(PlugDataColour::popupMenuBackgroundColourId);

        if (Desktop::canUseSemiTransparentWindows()) {
            Path shadowPath;
            shadowPath.addRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, height).reduced(10.0f), PlugDataLook::defaultCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 10, { 0, 2 });

            // Add a bit of alpha to disable the opaque flag

            g.setColour(background);

            auto bounds = Rectangle<float>(0, 0, width, height).reduced(7);
            g.fillRoundedRectangle(bounds, PlugDataLook::defaultCornerRadius);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(bounds, PlugDataLook::defaultCornerRadius, 1.0f);
        } else {
            auto bounds = Rectangle<float>(0, 0, width, height);

            g.setColour(background);
            g.fillRect(bounds);

            g.setColour(findColour(PlugDataColour::outlineColourId));
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

        if (isSeparator) {
            auto r = area.reduced(margin + 5, 0);
            r.removeFromTop(roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

            g.setColour(findColour(PlugDataColour::popupMenuTextColourId).withAlpha(0.3f));
            g.fillRect(r.removeFromTop(1));
        } else {
            auto r = area.reduced(margin, 1);

            auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isHighlighted && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(4, 0), 4.0f);
                colour = findColour(PlugDataColour::popupMenuActiveTextColourId);
            }

            g.setColour(colour);

            r.reduce(jmin(5, area.getWidth() / 20), 0);

            auto font = getPopupMenuFont();

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight(maxFontHeight);

            g.setFont(font);

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();

            if (icon != nullptr) {
                icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
                r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));
            } else if (isTicked) {
                auto tick = getTickShape(1.0f);
                g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
            }

            if (hasSubMenu) {
                auto arrowH = 0.6f * getPopupMenuFont().getAscent();

                auto x = static_cast<float>(r.removeFromRight((int)arrowH).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            PlugDataLook::drawFittedText(g, text, r, colour);

            if (shortcutKeyText.isNotEmpty()) {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

                g.setColour(colour);
                g.drawText(shortcutKeyText, r.translated(-2, 0), Justification::centredRight);
            }
        }
    }

    int getMenuWindowFlags() override
    {
        return ComponentPeer::windowIsTemporary;
    }

    int getPopupMenuBorderSize() override
    {
        if (Desktop::canUseSemiTransparentWindows()) {
            return 10;
        } else {
            return 2;
        }
    };

    void drawTreeviewPlusMinusBox(Graphics& g, Rectangle<float> const& area, Colour backgroundColour, bool isOpen, bool isMouseOver) override
    {
        Path p;
        p.addTriangle(0.0f, 0.0f, 1.0f, isOpen ? 0.0f : 0.5f, isOpen ? 0.5f : 0.0f, 1.0f);
        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
        g.fillPath(p, p.getTransformToScaleToFit(area.reduced(2, area.getHeight() / 4), true));
    }

    void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override
    {
        bool inspectorElement = object.getProperties()["Style"] == "Inspector";

        auto cornerSize = inspectorElement ? 0.0f : 3.0f;
        Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(object.findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

        if (!inspectorElement) {
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

    void drawResizableFrame(Graphics& g, int w, int h, BorderSize<int> const& border) override
    {
    }

    void drawGUIObjectSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider& slider)
    {
        auto sliderBounds = slider.getLocalBounds().toFloat().reduced(1.0f);

        g.setColour(findColour(Slider::backgroundColourId));
        g.fillRect(sliderBounds);

        constexpr auto thumbSize = 4.0f;
        constexpr auto halfThumbSize = thumbSize / 2.0f;
        auto cornerSize = PlugDataLook::objectCornerRadius / 2.0f;

        Path toDraw;
        if (slider.isHorizontal()) {
            sliderPos = jmap<float>(sliderPos, x, width, x, width - thumbSize);

            auto b = Rectangle<float>(thumbSize, height).translated(sliderPos, y);

            g.setColour(findColour(Slider::trackColourId));
            g.fillRoundedRectangle(b, cornerSize);
        } else {
            sliderPos = jmap<float>(sliderPos, y, height, y, height - thumbSize);
            auto b = Rectangle<float>(width, thumbSize).translated(x, sliderPos);

            g.setColour(findColour(Slider::trackColourId));
            g.fillRoundedRectangle(b, cornerSize);
        }
    }

    void drawVolumeSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider)
    {
        float trackWidth = 4.;

        x += 1;
        width -= 2;

        Point<float> startPoint(slider.isHorizontal() ? x : x + width * 0.5f, slider.isHorizontal() ? y + height * 0.5f : height + y);
        Point<float> endPoint(slider.isHorizontal() ? width + x : startPoint.x, slider.isHorizontal() ? startPoint.y : y);

        Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);

        g.setColour(slider.findColour(PlugDataColour::levelMeterInactiveColourId));
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

        g.setColour(slider.findColour(PlugDataColour::levelMeterTrackColourId));
        g.strokePath(valueTrack, { trackWidth, PathStrokeType::mitered });
        g.setColour(slider.findColour(PlugDataColour::levelMeterThumbColourId));

        g.fillRoundedRectangle(Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(22)).withCentre(maxPoint), 2.0f);
    }

    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor) override
    {
        if (dynamic_cast<AlertWindow*>(textEditor.getParentComponent()) != nullptr) {
            g.setColour(textEditor.findColour(TextEditor::backgroundColourId));
            g.fillRect(0, 0, width, height);
        } else {
            g.fillAll(textEditor.findColour(TextEditor::backgroundColourId));
        }
    }

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override
    {
        if (textEditor.getProperties()["NoOutline"].isVoid()) {
            if (textEditor.isEnabled()) {
                if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly()) {
                    g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                    g.drawRect(0, 0, width, height, 1);
                } else {
                    g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                    g.drawRect(0, 0, width, height);
                }
            }
        }
    }

    void drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        Path corner;

        corner.startNewSubPath(0, h);
        corner.lineTo(w, h);
        corner.lineTo(w, 0);
        corner = corner.createPathWithRoundedCorners(PlugDataLook::windowCornerRadius);
        corner.lineTo(0, h);

        g.setColour(findColour(PlugDataColour::resizeableCornerColourId).withAlpha(isMouseOver ? 1.0f : 0.6f));
        g.fillPath(corner);
    }

    void drawTooltip(Graphics& g, String const& text, int width, int height) override
    {
        auto bounds = Rectangle<float>(0, 0, width, height);
        auto cornerSize = PlugDataLook::defaultCornerRadius;

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

        float const tooltipFontSize = 14.0f;
        int const maxToolTipWidth = 1000;

        AttributedString s;
        s.setJustification(Justification::centredLeft);

        auto lines = StringArray::fromLines(text);

        for (auto const& line : lines) {
            if (line.contains("(") && line.contains(")")) {
                auto type = line.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
                auto description = line.fromFirstOccurrenceOf(")", false, false);
                s.append(type + ":", Fonts::getSemiBoldFont().withHeight(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));

                s.append(description + "\n", Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            } else {
                s.append(line, Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            }
        }

        TextLayout tl;
        tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
        tl.draw(g, bounds.withSizeKeepingCentre(width - 20, height - 2));
    }

    // For drawing icons with icon font
    static void drawIcon(Graphics& g, String const& icon, Rectangle<int> bounds, Colour colour, int fontHeight = -1, bool centred = true)
    {
        if (fontHeight < 0)
            fontHeight = bounds.getHeight() / 1.2f;

        auto justification = centred ? Justification::centred : Justification::centredLeft;
        g.setFont(Fonts::getIconFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(icon, bounds, justification, false);
    }

    static void drawIcon(Graphics& g, String const& icon, int x, int y, int size, Colour colour, int fontHeight = -1, bool centred = true)
    {
        drawIcon(g, icon, { x, y, size, size }, colour, fontHeight, centred);
    }

    // For drawing bold, semibold or thin text
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        Font font;
        switch (style) {
        case Regular:
            font = Fonts::getCurrentFont();
            break;
        case Bold:
            font = Fonts::getBoldFont();
            break;
        case Semibold:
            font = Fonts::getSemiBoldFont();
            break;
        case Thin:
            font = Fonts::getThinFont();
            break;
        case Monospace:
            font = Fonts::getMonospaceFont();
            break;
        }

        g.setFont(font.withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    static void drawStyledText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawStyledText(g, textToDraw, { x, y, w, h }, colour, style, fontHeight, justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<float> bounds, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    static void drawText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawText(g, textToDraw, Rectangle<int>(x, y, w, h), colour, fontHeight, justification);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, int numLines = 1, float minimumHoriontalScale = 1.0f, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawFittedText(textToDraw, bounds, justification, numLines, minimumHoriontalScale);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour const& colour, int numLines = 1, float minimumHoriontalScale = 1.0f, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawFittedText(g, textToDraw, { x, y, w, h }, colour, numLines, minimumHoriontalScale, fontHeight, justification);
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        g.fillAll(label.findColour(Label::backgroundColourId));

        if (!label.isBeingEdited()) {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const Font font = Fonts::getCurrentFont().withHeight(label.getFont().getHeight());

            auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

            // TODO: check if this is correct, can we get the correct numlines and scale?
            g.setFont(font);
            g.setColour(label.findColour(Label::textColourId));

            g.drawFittedText(label.getText(), textArea, label.getJustificationType(), 1, 1.0f);

            g.setColour(label.findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
        } else if (label.isEnabled()) {
            g.setColour(label.findColour(Label::outlineColourId));
        }

        g.drawRect(label.getLocalBounds());
    }

    void drawPropertyComponentLabel(Graphics& g, int width, int height, PropertyComponent& component) override
    {
        auto indent = jmin(10, component.getWidth() / 10);

        auto colour = component.findColour(PropertyComponent::labelTextColourId)
                          .withMultipliedAlpha(component.isEnabled() ? 1.0f : 0.6f);

        auto r = getPropertyComponentContentPosition(component);

        PlugDataLook::drawFittedText(g, component.getName(), indent, r.getY(), r.getX() - 5, r.getHeight(), colour, 1, 1.0f, (float)jmin(height, 24) * 0.65f, Justification::centredLeft);
    }

    void drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height) override
    {
        auto buttonSize = (float)height * 0.75f;
        auto buttonIndent = ((float)height - buttonSize) * 0.5f;

        drawTreeviewPlusMinusBox(g, { buttonIndent, buttonIndent, buttonSize, buttonSize }, findColour(ResizableWindow::backgroundColourId), isOpen, false);

        auto textX = static_cast<int>((buttonIndent * 2.0f + buttonSize + 2.0f));

        PlugDataLook::drawStyledText(g, name, textX, 0, std::max(width - textX - 4, 0), height, findColour(PropertyComponent::labelTextColourId), Bold, height * 0.6f);
    }

    Rectangle<int> getTooltipBounds(String const& tipText, Point<int> screenPos, Rectangle<int> parentArea) override
    {
        float const tooltipFontSize = 14.0f;
        int const maxToolTipWidth = 1000;

        AttributedString s;
        s.setJustification(Justification::centredLeft);

        auto lines = StringArray::fromLines(tipText);

        for (auto const& line : lines) {
            if (line.contains("(") && line.contains(")")) {
                auto type = line.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
                auto description = line.fromFirstOccurrenceOf(")", false, false);
                s.append(type + ":", Fonts::getSemiBoldFont().withHeight(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));

                s.append(description + "\n", Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            } else {
                s.append(line, Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            }
        }

        TextLayout tl;
        tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);

        int marginX = 22.0f;
        int marginY = 10.0f;

        auto w = (int)(tl.getWidth() + marginX);
        auto h = (int)(tl.getHeight() + marginY);

        return Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
            screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
            w, h)
            .constrainedWithin(parentArea);
    }

    int getTreeViewIndentSize(TreeView&) override
    {
        return 36;
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
            colours.at(PlugDataColour::sliderThumbColourId));
        setColour(ScrollBar::thumbColourId,
            colours.at(PlugDataColour::scrollbarThumbColourId));
        setColour(DirectoryContentsDisplayComponent::highlightColourId,
            colours.at(PlugDataColour::panelActiveBackgroundColourId));
        setColour(CaretComponent::caretColourId,
            colours.at(PlugDataColour::caretColourId));

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

        // Add dummy alpha to prevent JUCE from making it opaque
        setColour(PopupMenu::backgroundColourId,
            colours.at(PlugDataColour::panelBackgroundColourId).withAlpha(0.99f));

        setColour(ResizableWindow::backgroundColourId,
            colours.at(PlugDataColour::canvasBackgroundColourId));
        setColour(ScrollBar::backgroundColourId,
            colours.at(PlugDataColour::canvasBackgroundColourId));
        setColour(Slider::backgroundColourId,
            colours.at(PlugDataColour::canvasBackgroundColourId));
        setColour(Slider::trackColourId,
            colours.at(PlugDataColour::sliderThumbColourId));
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
            colours.at(PlugDataColour::outlineColourId));
        setColour(ComboBox::outlineColourId,
            colours.at(PlugDataColour::outlineColourId));
        setColour(TextEditor::outlineColourId,
            colours.at(PlugDataColour::outlineColourId));
        setColour(ListBox::outlineColourId,
            colours.at(PlugDataColour::outlineColourId));

        setColour(Slider::textBoxOutlineColourId,
            Colours::transparentBlack);
        setColour(TreeView::backgroundColourId,
            Colours::transparentBlack);
    }

    static void setDefaultFont(String fontName)
    {
        auto& lnf = dynamic_cast<PlugDataLook&>(getDefaultLookAndFeel());
        if (fontName.isEmpty() || fontName == "Inter") {
            auto defaultFont = Fonts::getDefaultFont();
            lnf.setDefaultSansSerifTypeface(defaultFont.getTypefacePtr());
            Fonts::setCurrentFont(defaultFont);
        } else {
            auto newDefaultFont = Font(fontName, 15, Font::plain);
            Fonts::setCurrentFont(newDefaultFont);
            lnf.setDefaultSansSerifTypeface(newDefaultFont.getTypefacePtr());
        }
    }

    static inline const String defaultThemesXml = "<ColourThemes>\n"
                                                  "    <Theme theme=\"max\" toolbar_background=\"ff333333\" toolbar_text=\"ffe4e4e4\"\n"
                                                  "           toolbar_active=\"ff72aedf\" tab_background=\"ff333333\" tab_text=\"ffe4e4e4\"\n"
                                                  "           active_tab_background=\"ff494949\" active_tab_text=\"ff72aedf\" canvas_background=\"ffe5e5e5\"\n"
                                                  "           canvas_text=\"ffeeeeee\" canvas_dots=\"ff7f7f7f\" default_object_background=\"ff333333\"\n"
                                                  "           object_outline_colour=\"ff696969\" selected_object_outline_colour=\"ff72aedf\"\n"
                                                  "           outline_colour=\"ff393939\" data_colour=\"ff72aedf\" connection_colour=\"ffb3b3b3\"\n"
                                                  "           signal_colour=\"ffe1ef00\" dialog_background=\"ff333333\" sidebar_colour=\"ff3e3e3e\"\n"
                                                  "           sidebar_text=\"ffe4e4e4\" sidebar_background_active=\"ff72aedf\"\n"
                                                  "           sidebar_active_text=\"ffe4e4e4\" levelmeter_active=\"ff72aedf\" levelmeter_inactive=\"ff5d5d5d\"\n"
                                                  "           levelmeter_track=\"ff333333\" levelmeter_thumb=\"ffe4e4e4\" panel_colour=\"ff232323\"\n"
                                                  "           panel_text=\"ffe4e4e4\" panel_background_active=\"ff72aedf\" panel_active_text=\"ffe4e4e4\"\n"
                                                  "           popup_background=\"ff333333\" popup_background_active=\"ff72aedf\"\n"
                                                  "           popup_text=\"ffe4e4e4\" popup_active_text=\"ffe4e4e4\" slider_thumb=\"ff72aedf\" scrollbar_thumb=\"ffa9a9a9\"\n"
                                                  "           graph_resizer=\"ff72aedf\" grid_colour=\"ff72aedf\" caret_colour=\"ff72aedf\"\n"
                                                  "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"0\"\n"
                                                  "           square_iolets=\"0\" square_object_corners=\"1\" iolet_area_colour=\"ff808080\"\n"
                                                  "           iolet_outline_colour=\"ff696969\" text_object_background=\"ff333333\"/>\n"
                                                  "    <Theme theme=\"classic\" toolbar_background=\"ffffffff\" toolbar_text=\"ff000000\"\n"
                                                  "           toolbar_active=\"ff787878\" tab_background=\"ffffffff\" tab_text=\"ff000000\"\n"
                                                  "           active_tab_background=\"ffffffff\" active_tab_text=\"ff000000\" canvas_background=\"ffffffff\"\n"
                                                  "           canvas_text=\"ff000000\" canvas_dots=\"ffffffff\" default_object_background=\"ffffffff\"\n"
                                                  "           text_object_background=\"ffffffff\" object_outline_colour=\"ff000000\"\n"
                                                  "           selected_object_outline_colour=\"ff000000\" outline_colour=\"ff000000\"\n"
                                                  "           iolet_area_colour=\"ffffffff\" iolet_outline_colour=\"ff000000\"\n"
                                                  "           data_colour=\"ff000000\" connection_colour=\"ff000000\" signal_colour=\"ff000000\"\n"
                                                  "           dialog_background=\"ffffffff\" sidebar_colour=\"ffffffff\" sidebar_text=\"ff000000\"\n"
                                                  "           sidebar_background_active=\"ff000000\" sidebar_active_text=\"ffffffff\"\n"
                                                  "           levelmeter_active=\"ff000000\" levelmeter_inactive=\"ffffffff\" levelmeter_track=\"ff000000\"\n"
                                                  "           levelmeter_thumb=\"ff000000\" panel_colour=\"ffffffff\" panel_text=\"ff000000\"\n"
                                                  "           panel_background_active=\"ff000000\" panel_active_text=\"ffffffff\"\n"
                                                  "           popup_background=\"ffffffff\" popup_background_active=\"ff000000\"\n"
                                                  "           popup_text=\"ff000000\" popup_active_text=\"ffffffff\" slider_thumb=\"ff000000\" scrollbar_thumb=\"ffa9a9a9\"\n"
                                                  "           graph_resizer=\"ff000000\" grid_colour=\"ff000000\" caret_colour=\"ff000000\"\n"
                                                  "           dashed_signal_connections=\"0\" straight_connections=\"1\" thin_connections=\"1\"\n"
                                                  "           square_iolets=\"1\" square_object_corners=\"1\"/>\n"
                                                  "    <Theme theme=\"classic_dark\" toolbar_background=\"ff000000\" toolbar_text=\"ffffffff\"\n"
                                                  "           toolbar_active=\"ff787878\" tab_background=\"ff000000\" tab_text=\"ffffffff\"\n"
                                                  "           active_tab_background=\"ff000000\" active_tab_text=\"ffffffff\" canvas_background=\"ff000000\"\n"
                                                  "           canvas_text=\"ffffffff\" canvas_dots=\"ff000000\" default_object_background=\"ff000000\"\n"
                                                  "           object_outline_colour=\"ffffffff\" selected_object_outline_colour=\"ffffffff\"\n"
                                                  "           outline_colour=\"ffffffff\" data_colour=\"ffffffff\" connection_colour=\"ffffffff\"\n"
                                                  "           signal_colour=\"ffffffff\" dialog_background=\"ff000000\" sidebar_colour=\"ff000000\"\n"
                                                  "           sidebar_text=\"ffffffff\" sidebar_background_active=\"ffffffff\"\n"
                                                  "           sidebar_active_text=\"ff000000\" levelmeter_active=\"ffffffff\" levelmeter_inactive=\"ff000000\"\n"
                                                  "           levelmeter_track=\"ffffffff\" levelmeter_thumb=\"ffffffff\" panel_colour=\"ff000000\"\n"
                                                  "           panel_text=\"ffffffff\" panel_background_active=\"ffffffff\" panel_active_text=\"ff000000\"\n"
                                                  "           popup_background=\"ff000000\" popup_background_active=\"ffffffff\"\n"
                                                  "           popup_text=\"ffffffff\" popup_active_text=\"ff000000\" slider_thumb=\"ffffffff\" scrollbar_thumb=\"ff7f7f7f\"\n"
                                                  "           graph_resizer=\"ffffffff\" grid_colour=\"ffffffff\" caret_colour=\"ffffffff\"\n"
                                                  "           dashed_signal_connections=\"0\" straight_connections=\"1\" thin_connections=\"1\"\n"
                                                  "           square_iolets=\"1\" square_object_corners=\"1\" iolet_area_colour=\"ff000000\"\n"
                                                  "           iolet_outline_colour=\"ffffffff\" text_object_background=\"ff000000\"/>\n"
                                                  "    <Theme theme=\"dark\" toolbar_background=\"ff191919\" toolbar_text=\"ffffffff\"\n"
                                                  "           toolbar_active=\"ff42a2c8\" tab_background=\"ff191919\" tab_text=\"ffffffff\"\n"
                                                  "           active_tab_background=\"ff232323\" active_tab_text=\"ffffffff\" canvas_background=\"ff232323\"\n"
                                                  "           canvas_text=\"ffffffff\" canvas_dots=\"ff7f7f7f\" default_object_background=\"ff191919\"\n"
                                                  "           object_outline_colour=\"ff696969\" selected_object_outline_colour=\"ff42a2c8\"\n"
                                                  "           outline_colour=\"ff393939\" data_colour=\"ff42a2c8\" connection_colour=\"ffe1e1e1\"\n"
                                                  "           signal_colour=\"ffff8500\" dialog_background=\"ff191919\" sidebar_colour=\"ff191919\"\n"
                                                  "           sidebar_text=\"ffffffff\" sidebar_background_active=\"ff282828\"\n"
                                                  "           sidebar_active_text=\"ffffffff\" levelmeter_active=\"ff42a2c8\" levelmeter_inactive=\"ff2d2d2d\"\n"
                                                  "           levelmeter_track=\"fff5f5f5\" levelmeter_thumb=\"fff5f5f5\" panel_colour=\"ff232323\"\n"
                                                  "           panel_text=\"ffffffff\" panel_background_active=\"ff373737\" panel_active_text=\"ffffffff\"\n"
                                                  "           popup_background=\"ff191919\" popup_background_active=\"ff282828\"\n"
                                                  "           popup_text=\"ffffffff\" popup_active_text=\"ffffffff\" slider_thumb=\"ff42a2c8\" scrollbar_thumb=\"ff7f7f7f\"\n"
                                                  "           graph_resizer=\"ff42a2c8\" grid_colour=\"ff42a2c8\" caret_colour=\"ff42a2c8\"\n"
                                                  "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"0\"\n"
                                                  "           square_iolets=\"0\" square_object_corners=\"0\" text_object_background=\"ff232323\"\n"
                                                  "           iolet_area_colour=\"ff232323\" iolet_outline_colour=\"ff696969\"/>\n"
                                                  "    <Theme theme=\"light\" toolbar_background=\"ffe4e4e4\" toolbar_text=\"ff5a5a5a\"\n"
                                                  "           toolbar_active=\"ff007aff\" tab_background=\"ffe4e4e4\" tab_text=\"ff5a5a5a\"\n"
                                                  "           active_tab_background=\"fffafafa\" active_tab_text=\"ff5a5a5a\" canvas_background=\"fffafafa\"\n"
                                                  "           canvas_text=\"ff5a5a5a\" canvas_dots=\"ff909090\" default_object_background=\"ffe4e4e4\"\n"
                                                  "           object_outline_colour=\"ffa8a8a8\" selected_object_outline_colour=\"ff007aff\"\n"
                                                  "           outline_colour=\"ffc8c8c8\" data_colour=\"ff007aff\" connection_colour=\"ffb3b3b3\"\n"
                                                  "           signal_colour=\"ffff8500\" dialog_background=\"ffe4e4e4\" sidebar_colour=\"ffeeeeee\"\n"
                                                  "           sidebar_text=\"ff5a5a5a\" sidebar_background_active=\"ffd9d9d9\"\n"
                                                  "           sidebar_active_text=\"ff5a5a5a\" levelmeter_active=\"ff007aff\" levelmeter_inactive=\"ffeeeeee\"\n"
                                                  "           levelmeter_track=\"ff5a5a5a\" levelmeter_thumb=\"ff7a7a7a\" panel_colour=\"fffafafa\"\n"
                                                  "           panel_text=\"ff5a5a5a\" panel_background_active=\"ffebebeb\" panel_active_text=\"ff5a5a5a\"\n"
                                                  "           popup_background=\"ffe4e4e4\" popup_background_active=\"ffcfcfcf\"\n"
                                                  "           popup_text=\"ff5a5a5a\" popup_active_text=\"ff5a5a5a\" slider_thumb=\"ff007aff\" scrollbar_thumb=\"ffa9a9a9\"\n"
                                                  "           graph_resizer=\"ff007aff\" grid_colour=\"ff007aff\" caret_colour=\"ff007aff\"\n"
                                                  "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"0\"\n"
                                                  "           square_iolets=\"0\" square_object_corners=\"0\" text_object_background=\"fffafafa\"\n"
                                                  "           iolet_area_colour=\"fffafafa\" iolet_outline_colour=\"ffa8a8a8\"/>\n"
                                                  "    <Theme theme=\"warm\" toolbar_background=\"ffd2cdc4\" toolbar_text=\"ff5a5a5a\"\n"
                                                  "           toolbar_active=\"ff5da0c4\" tab_background=\"ffd2cdc4\" tab_text=\"ff5a5a5a\"\n"
                                                  "           active_tab_background=\"ffdedad3\" active_tab_text=\"ff5a5a5a\" canvas_background=\"ffe3dfd9\"\n"
                                                  "           canvas_text=\"ff5a5a5a\" canvas_dots=\"ff909090\" default_object_background=\"ffe3dfd9\"\n"
                                                  "           object_outline_colour=\"ff968e82\" selected_object_outline_colour=\"ff5da0c4\"\n"
                                                  "           outline_colour=\"ff968e82\" data_colour=\"ff5da0c4\" connection_colour=\"ffb3b3b3\"\n"
                                                  "           signal_colour=\"ffff8502\" dialog_background=\"ffd2cdc4\" sidebar_colour=\"ffdedad3\"\n"
                                                  "           sidebar_text=\"ff5a5a5a\" sidebar_background_active=\"ffd2cdc4\"\n"
                                                  "           sidebar_active_text=\"ff5a5a5a\" levelmeter_active=\"ff5da0c4\" levelmeter_inactive=\"ffd2cdc4\"\n"
                                                  "           levelmeter_track=\"ff5a5a5a\" levelmeter_thumb=\"ff7a7a7a\" panel_colour=\"ffe3dfd9\"\n"
                                                  "           panel_text=\"ff5a5a5a\" panel_background_active=\"ffebebeb\" panel_active_text=\"ff5a5a5a\"\n"
                                                  "           popup_background=\"ffd2cdc4\" popup_background_active=\"ffdedad3\"\n"
                                                  "           popup_text=\"ff5a5a5a\" popup_active_text=\"ff5a5a5a\" slider_thumb=\"ff5da0c4\" scrollbar_thumb=\"ffa9a9a9\"\n"
                                                  "           graph_resizer=\"ff5da0c4\" grid_colour=\"ff5da0c4\" caret_colour=\"ff5da0c4\"\n"
                                                  "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"0\"\n"
                                                  "           square_iolets=\"0\" square_object_corners=\"0\" iolet_area_colour=\"ffe3dfd9\"\n"
                                                  "           iolet_outline_colour=\"ff968e82\" text_object_background=\"ffe3dfd9\"/>\n"
                                                  "    <Theme theme=\"fangs\" toolbar_background=\"ff232323\" toolbar_text=\"ffffffff\"\n"
                                                  "           toolbar_active=\"ff5bcefa\" tab_background=\"ff232323\" tab_text=\"ffffffff\"\n"
                                                  "           active_tab_background=\"ff3a3a3a\" active_tab_text=\"ffffffff\" canvas_background=\"ff383838\"\n"
                                                  "           canvas_text=\"ffffffff\" canvas_dots=\"ffa0a0a0\" default_object_background=\"ff191919\"\n"
                                                  "           object_outline_colour=\"ff232323\" selected_object_outline_colour=\"ffffacab\"\n"
                                                  "           outline_colour=\"ff575757\" data_colour=\"ff5bcefa\" connection_colour=\"ffa0a0a0\"\n"
                                                  "           signal_colour=\"ffffacab\" dialog_background=\"ff191919\" sidebar_colour=\"ff232323\"\n"
                                                  "           sidebar_text=\"ffffffff\" sidebar_background_active=\"ff383838\"\n"
                                                  "           sidebar_active_text=\"ffffffff\" levelmeter_active=\"ff5bcefa\" levelmeter_inactive=\"ff2d2d2d\"\n"
                                                  "           levelmeter_track=\"fff5f5f5\" levelmeter_thumb=\"fff5f5f5\" panel_colour=\"ff383838\"\n"
                                                  "           panel_text=\"ffffffff\" panel_background_active=\"ff232323\" panel_active_text=\"ffffffff\"\n"
                                                  "           popup_background=\"ff232323\" popup_background_active=\"ff383838\"\n"
                                                  "           popup_text=\"ffffffff\" popup_active_text=\"ffffffff\" scrollbar_thumb=\"ff8e8e8e\"\n"
                                                  "           graph_resizer=\"ff5bcefa\" grid_colour=\"ff5bcefa\" caret_colour=\"ffffacab\"\n"
                                                  "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"1\"\n"
                                                  "           square_iolets=\"1\" square_object_corners=\"0\" text_object_background=\"ff232323\"\n"
                                                  "           iolet_area_colour=\"ff232323\" iolet_outline_colour=\"ff696969\" slider_thumb=\"ff8e8e8e\"/>\n"
                                                  "  </ColourThemes>";

    static void resetColours(ValueTree themesTree)
    {
        auto defaultThemesTree = ValueTree::fromXml(PlugDataLook::defaultThemesXml);

        for (auto themeTree : defaultThemesTree) {
            if (themesTree.getChildWithProperty("theme", themeTree.getProperty("theme").toString()).isValid()) {
                auto childToRemove = themesTree.getChildWithProperty("theme", themeTree.getProperty("theme"));
                themesTree.removeChild(childToRemove, nullptr);
            }

            themesTree.appendChild(themeTree.createCopy(), nullptr);
        }

        selectedThemes = { "light", "dark" };
    }

    static Colour getThemeColour(ValueTree themeTree, PlugDataColour colourId)
    {
        return Colour::fromString(themeTree.getProperty(std::get<1>(PlugDataColourNames.at(colourId))).toString());
    }

    void setTheme(ValueTree themeTree)
    {
        std::map<PlugDataColour, Colour> colours;

        // Quick check if this tree is valid
        if (!themeTree.hasProperty("theme"))
            return;

        for (auto const& [colourId, colourNames] : PlugDataColourNames) {
            auto [id, colourName, category] = colourNames;
            colours[colourId] = Colour::fromString(themeTree.getProperty(colourName).toString());
        }

        setColours(colours);
        currentTheme = themeTree.getProperty("theme").toString();

        objectCornerRadius = themeTree.getProperty("square_object_corners") ? 0.0f : 2.75f;
        useDashedConnections = themeTree.getProperty("dashed_signal_connections");
        useStraightConnections = themeTree.getProperty("straight_connections");
        useThinConnections = themeTree.getProperty("thin_connections");
        useSquareIolets = themeTree.getProperty("square_iolets");
    }

    static StringArray getAllThemes()
    {
        auto themeTree = SettingsFile::getInstance()->getColourThemesTree();
        StringArray allThemes;
        for (auto theme : themeTree) {
            allThemes.add(theme.getProperty("theme").toString());
        }

        return allThemes;
    }

    static bool getUseDashedConnections()
    {
        return useDashedConnections;
    }
    static bool getUseStraightConnections()
    {
        return useStraightConnections;
    }
    static bool getUseThinConnections()
    {
        return useThinConnections;
    }
    static bool getUseSquareIolets()
    {
        return useSquareIolets;
    }

    static inline bool useDashedConnections = true;
    static inline bool useStraightConnections = false;
    static inline bool useThinConnections = false;
    static inline bool useSquareIolets = false;

    static inline String currentTheme = "light";
    static inline StringArray selectedThemes = { "light", "dark" };

    inline static float const windowCornerRadius = 7.5f;
    inline static float const defaultCornerRadius = 6.0f;
    inline static float const smallCornerRadius = 4.0f;
    inline static float objectCornerRadius = 2.75f;
};
