/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>
#include <map>

struct Constants {
    inline static float const windowCornerRadius = 7.5f;
    inline static float const defaultCornerRadius = 6.0f;
    inline static float const smallCornerRadius = 4.0f;
    inline static float const objectCornerRadius = 2.75f;
};

#include "Utility/StackShadow.h"

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

    defaultObjectBackgroundColourId,
    objectOutlineColourId,
    objectSelectedOutlineColourId,
    outlineColourId,

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
    { defaultObjectBackgroundColourId, { "Default Object Background", "default_object_background", "Canvas" } },
    { outlineColourId, { "Outline Colour", "outline_colour", "Canvas" } },
    { objectOutlineColourId, { "Object outline colour", "object_outline_colour", "Canvas" } },
    { objectSelectedOutlineColourId, { "Selected object outline colour", "selected_object_outline_colour", "Canvas" } },
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

struct Resources {
    Typeface::Ptr defaultTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);

    Typeface::Ptr thinTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize);

    Typeface::Ptr boldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);

    Typeface::Ptr semiBoldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);

    Typeface::Ptr iconTypeface = Typeface::createSystemTypefaceFor(BinaryData::PlugDataFont_ttf, BinaryData::PlugDataFont_ttfSize);

    Typeface::Ptr monoTypeface = Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMono_ttf, BinaryData::IBMPlexMono_ttfSize);
};

struct PlugDataLook : public LookAndFeel_V4 {
    SharedResourcePointer<Resources> resources;

    Font defaultFont;
    Font boldFont;
    Font semiBoldFont;
    Font thinFont;
    Font iconFont;
    Font monoFont;

    PlugDataLook()
        : defaultFont(resources->defaultTypeface)
        , boldFont(resources->boldTypeface)
        , semiBoldFont(resources->semiBoldTypeface)
        , thinFont(resources->thinTypeface)
        , iconFont(resources->iconTypeface)
        , monoFont(resources->monoTypeface)
    {
        setTheme(false);
        setDefaultSansSerifTypeface(resources->defaultTypeface);
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
        if (s.getName().startsWith("statusbar")) {
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
        if (button.getName().startsWith("toolbar")) {
            drawToolbarButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        } else if (button.getName().startsWith("tabbar")) {
            g.fillAll(findColour(PlugDataColour::tabBackgroundColourId));
        } else if (button.getName().startsWith("statusbar")) {
            drawStatusbarButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        } else if (button.getName().startsWith("pd")) {
            drawPdButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        } else if (button.getName().startsWith("inspector")) {
            drawInspectorButton(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        } else {
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override
    {
        if (button.getName().startsWith("toolbar")) {
            button.setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarActiveColourId));
            button.setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));

            LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
        } else if (button.getName().startsWith("statusbar")) {
            drawStatusbarButtonText(g, button, isMouseOverButton, isButtonDown);
        } else {
            LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
        }
    }

    Font getTextButtonFont(TextButton& but, int buttonHeight) override
    {
        if (but.getName().startsWith("toolbar")) {
            return getToolbarFont(buttonHeight * 1.16f);
        }
        if (but.getName().startsWith("statusbar:oversample")) {
            return { buttonHeight / 2.0f };
        }
        if (but.getName().startsWith("tabbar")) {
            return iconFont.withHeight(buttonHeight / 2.4f);
        }
        if (but.getName().startsWith("statusbar") || but.getName().startsWith("tab")) {
            return getStatusbarFont(buttonHeight * 1.08f);
        }

        return { buttonHeight / 1.7f };
    }

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        if (slider.getName().startsWith("statusbar")) {
            drawVolumeSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        } else if (slider.getName() == "object:slider") {
            drawGUIObjectSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, slider);
        } else {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, Image const* icon, bool drawTitleTextOnLeft) override
    {
        if (w * h == 0)
            return;

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

        auto textArea = button.getLocalBounds();
        AttributedString attributedTabTitle(button.getButtonText());
        auto tabTextColour = findColour(isActive ? PlugDataColour::activeTabTextColourId : PlugDataColour::tabTextColourId);
        attributedTabTitle.setColour(tabTextColour);
        attributedTabTitle.setFont(Font(12));
        attributedTabTitle.setJustification(Justification::centred);
        attributedTabTitle.draw(g, textArea.toFloat());
    }

    void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, int const w, int const h) override
    {
    }

    Font getTabButtonFont(TabBarButton&, float height) override
    {
        return { height * 0.4f };
    }

    Font getToolbarFont(int buttonHeight)
    {
        return iconFont.withHeight(buttonHeight / 3.5);
    }

    Font getStatusbarFont(int buttonHeight)
    {
        return iconFont.withHeight(buttonHeight / 2.5);
    }

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override
    {

        // Fill background if there's no support for transparent popupmenus
#ifdef PLUGDATA_STANDALONE
        if (!Desktop::canUseSemiTransparentWindows()) {
            g.fillAll(findColour(ResizableWindow::backgroundColourId));
        }
#endif

        Path shadowPath;
        shadowPath.addRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, height).reduced(10.0f), Constants::defaultCornerRadius);
        StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 10, { 0, 2 });

        // Add a bit of alpha to disable the opaque flag
        auto background = findColour(PlugDataColour::popupMenuBackgroundColourId);
        g.setColour(background);

        auto bounds = Rectangle<float>(0, 0, width, height).reduced(7);
        g.fillRoundedRectangle(bounds, Constants::defaultCornerRadius);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(bounds, Constants::defaultCornerRadius, 1.0f);
    }

    void drawPopupMenuItem(Graphics& g, Rectangle<int> const& area,
        bool const isSeparator, bool const isActive,
        bool const isHighlighted, bool const isTicked,
        bool const hasSubMenu, String const& text,
        String const& shortcutKeyText,
        Drawable const* icon, Colour const* const textColourToUse) override
    {
        if (isSeparator) {
            auto r = area.reduced(14, 0);
            r.removeFromTop(roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

            g.setColour(findColour(PlugDataColour::popupMenuTextColourId).withAlpha(0.3f));
            g.fillRect(r.removeFromTop(1));
        } else {
            auto r = area.reduced(9, 1);

            if (isHighlighted && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(4, 0), 4.0f);

                g.setColour(findColour(PlugDataColour::popupMenuActiveTextColourId));
            } else {
                g.setColour(findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f));
            }

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
            g.drawFittedText(text, r, Justification::centredLeft, 1);

            if (shortcutKeyText.isNotEmpty()) {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

                g.drawText(shortcutKeyText, r.translated(-2, 0), Justification::centredRight, true);
            }
        }
    }

    int getMenuWindowFlags() override
    {
        return ComponentPeer::windowIsTemporary;
    }

    int getPopupMenuBorderSize() override
    {
        return 10;
    };

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override
    {
        if (textEditor.getName() == "sidebar::searcheditor")
            return;

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

    void drawTreeviewPlusMinusBox(Graphics& g, Rectangle<float> const& area, Colour backgroundColour, bool isOpen, bool isMouseOver) override
    {
        Path p;
        p.addTriangle(0.0f, 0.0f, 1.0f, isOpen ? 0.0f : 0.5f, isOpen ? 0.5f : 0.0f, 1.0f);
        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
        g.fillPath(p, p.getTransformToScaleToFit(area.reduced(2, area.getHeight() / 4), true));
    }

    void drawToolbarButton(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
    }

    void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override
    {
        bool inspectorElement = object.getName().startsWith("inspector");

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

    void drawStatusbarButton(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
    }

    void drawResizableFrame(Graphics& g, int w, int h, BorderSize<int> const& border) override
    {
    }

    void drawInspectorButton(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
            baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);

        if (!shouldDrawButtonAsHighlighted && !button.getToggleState())
            baseColour = Colours::transparentBlack;

        g.setColour(baseColour);
        g.fillRect(bounds);
        g.setColour(button.findColour(ComboBox::outlineColourId));
    }

    void drawStatusbarButtonText(Graphics& g, TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        Font font(getTextButtonFont(button, button.getHeight()));
        g.setFont(font);

        if (!button.isEnabled()) {
            g.setColour(Colours::grey);
        } else if (button.getToggleState()) {
            g.setColour(button.findColour(PlugDataColour::toolbarActiveColourId));
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour(button.findColour(PlugDataColour::toolbarActiveColourId).brighter(0.8f));
        } else {
            g.setColour(button.findColour(PlugDataColour::toolbarTextColourId));
        }

        int const yIndent = jmin(4, button.proportionOfHeight(0.3f));
        int const cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

        int const fontHeight = roundToInt(font.getHeight() * 0.6f);
        int const leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        int const rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        int const textWidth = button.getWidth() - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, 2);
    }

    void drawPdButton(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto cornerSize = Constants::defaultCornerRadius;
        auto bounds = button.getLocalBounds().toFloat();

        auto baseColour = findColour(TextButton::buttonColourId);

        auto highlightColour = findColour(TextButton::buttonOnColourId);

        if (shouldDrawButtonAsDown || button.getToggleState())
            baseColour = highlightColour;

        baseColour = baseColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        g.setColour(baseColour);

        auto flatOnLeft = button.isConnectedOnLeft();
        auto flatOnRight = button.isConnectedOnRight();
        auto flatOnTop = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {
            Path path;
            path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize, cornerSize, !(flatOnLeft || flatOnTop), !(flatOnRight || flatOnTop), !(flatOnLeft || flatOnBottom), !(flatOnRight || flatOnBottom));

            g.fillPath(path);

            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.strokePath(path, PathStrokeType(1.0f));
        } else {
            int dimension = std::min(bounds.getHeight(), bounds.getWidth()) / 2.0f;
            auto centre = bounds.getCentre();
            auto ellpiseBounds = Rectangle<float>(centre.translated(-dimension, -dimension), centre.translated(dimension, dimension));
            g.fillEllipse(ellpiseBounds);

            g.setColour(button.findColour(ComboBox::outlineColourId));
            g.drawEllipse(ellpiseBounds, 1.0f);
        }
    }
    void drawGUIObjectSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider& slider)
    {
        auto sliderBounds = slider.getLocalBounds().toFloat().reduced(1.0f);

        g.setColour(findColour(Slider::backgroundColourId));
        g.fillRect(sliderBounds);

        Path toDraw;
        if (slider.isHorizontal()) {
            sliderPos = jmap<float>(sliderPos, x, width - (2 * x), 1.0f, width);
            auto b = sliderBounds.withTrimmedRight(width - sliderPos);
            toDraw.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 1.0f, 1.0f, true, false, true, false);
        } else {
            sliderPos = jmap<float>(sliderPos, y, height, 0.0f, height - 2.0f);
            auto b = sliderBounds.withTrimmedTop(sliderPos);
            toDraw.addRoundedRectangle(b.getX(), b.getY(), b.getWidth(), b.getHeight(), 1.0f, 1.0f, false, false, true, true);
        }

        g.setColour(findColour(Slider::trackColourId));
        g.fillPath(toDraw);
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

    void drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height) override
    {
        auto buttonSize = (float)height * 0.75f;
        auto buttonIndent = ((float)height - buttonSize) * 0.5f;

        drawTreeviewPlusMinusBox(g, { buttonIndent, buttonIndent, buttonSize, buttonSize }, findColour(ResizableWindow::backgroundColourId), isOpen, false);

        auto textX = static_cast<int>((buttonIndent * 2.0f + buttonSize + 2.0f));

        g.setColour(findColour(PropertyComponent::labelTextColourId));

        g.setFont({ (float)height * 0.6f, Font::bold });
        g.drawText(name, textX, 0, width - textX - 4, height, Justification::centredLeft, true);
    }

    void drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        Path corner;

        corner.startNewSubPath(0, h);
        corner.lineTo(w, h);
        corner.lineTo(w, 0);
        corner = corner.createPathWithRoundedCorners(Constants::windowCornerRadius);
        corner.lineTo(0, h);

        g.setColour(findColour(PlugDataColour::resizeableCornerColourId).withAlpha(isMouseOver ? 1.0f : 0.6f));
        g.fillPath(corner);
    }

    void drawTooltip(Graphics& g, String const& text, int width, int height) override
    {
        Rectangle<int> bounds(width, height);
        auto cornerSize = Constants::defaultCornerRadius;

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

        float const tooltipFontSize = 14.0f;
        int const maxToolTipWidth = 400;

        AttributedString s;
        s.setJustification(Justification::centred);

        auto lines = StringArray::fromLines(text);

        for (const auto& line : lines) {
            if (line.contains("(") && line.contains(")")) {
                auto type = line.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false);
                auto description = line.fromFirstOccurrenceOf(")", false, false);
                s.append(type + ":", semiBoldFont.withHeight(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));

                s.append(description + "\n", Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            } else {
                s.append(line, Font(tooltipFontSize), findColour(PlugDataColour::popupMenuTextColourId));
            }
        }

        TextLayout tl;
        tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
        tl.draw(g, { static_cast<float>(width), static_cast<float>(height) });
    }

    Rectangle<int> getTooltipBounds(String const& tipText, Point<int> screenPos, Rectangle<int> parentArea) override
    {
        float const tooltipFontSize = 14.0f;
        int const maxToolTipWidth = 400;

        AttributedString s;
        s.setJustification(Justification::centred);
        s.append(tipText, Font(tooltipFontSize, Font::bold), Colours::black);

        TextLayout tl;
        tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);

        auto w = (int)(tl.getWidth() + 18.0f);
        auto h = (int)(tl.getHeight() + 10.0f);

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
            colours.at(PlugDataColour::scrollbarThumbColourId));
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
            colours.at(PlugDataColour::scrollbarThumbColourId));
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
            lnf.setDefaultSansSerifTypeface(lnf.defaultFont.getTypefacePtr());
        } else {
            auto newFont = Font(fontName, 15, Font::plain);
            lnf.setDefaultSansSerifTypeface(newFont.getTypefacePtr());
        }
    }

    inline static const std::map<PlugDataColour, Colour> defaultDarkTheme = {
        { PlugDataColour::toolbarBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::toolbarTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::toolbarActiveColourId, Colour(66, 162, 200) },

        { PlugDataColour::tabBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::tabTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::activeTabBackgroundColourId, Colour(35, 35, 35) },
        { PlugDataColour::activeTabTextColourId, Colour(255, 255, 255) },

        { PlugDataColour::canvasBackgroundColourId, Colour(35, 35, 35) },
        { PlugDataColour::canvasTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::canvasDotsColourId, Colour(127, 127, 127) },

        { PlugDataColour::outlineColourId, Colour(57, 57, 57) },

        { PlugDataColour::defaultObjectBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::objectOutlineColourId, Colour(105, 105, 105) },
        { PlugDataColour::objectSelectedOutlineColourId, Colour(66, 162, 200) },

        { PlugDataColour::dataColourId, Colour(66, 162, 200) },
        { PlugDataColour::connectionColourId, Colour(225, 225, 225) },
        { PlugDataColour::signalColourId, Colour(255, 133, 0) },
        { PlugDataColour::dialogBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::panelBackgroundColourId, Colour(35, 35, 35) },
        { PlugDataColour::panelTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::panelActiveBackgroundColourId, Colour(55, 55, 55) },
        { PlugDataColour::panelActiveTextColourId, Colour(255, 255, 255) },

        { PlugDataColour::scrollbarThumbColourId, Colour(66, 162, 200) },
        { PlugDataColour::gridLineColourId, Colour(66, 162, 200) },
        { PlugDataColour::caretColourId, Colour(66, 162, 200) },
        { PlugDataColour::resizeableCornerColourId, Colour(66, 162, 200) },

        { PlugDataColour::sidebarBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::sidebarTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::sidebarActiveBackgroundColourId, Colour(40, 40, 40) },
        { PlugDataColour::sidebarActiveTextColourId, Colour(255, 255, 255) },

        { PlugDataColour::levelMeterActiveColourId, Colour(66, 162, 200) },
        { PlugDataColour::levelMeterInactiveColourId, Colour(45, 45, 45) },
        { PlugDataColour::levelMeterTrackColourId, Colour(245, 245, 245) },
        { PlugDataColour::levelMeterThumbColourId, Colour(245, 245, 245) },

        { PlugDataColour::popupMenuBackgroundColourId, Colour(25, 25, 25) },
        { PlugDataColour::popupMenuActiveBackgroundColourId, Colour(40, 40, 40) },
        { PlugDataColour::popupMenuTextColourId, Colour(255, 255, 255) },
        { PlugDataColour::popupMenuActiveTextColourId, Colour(255, 255, 255) }
    };

    inline static const std::map<PlugDataColour, Colour> defaultLightTheme = {
        { PlugDataColour::toolbarBackgroundColourId, Colour(228, 228, 228) },
        { PlugDataColour::toolbarTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::toolbarActiveColourId, Colour(0, 122, 255) },

        { PlugDataColour::tabBackgroundColourId, Colour(228, 228, 228) },
        { PlugDataColour::tabTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::activeTabBackgroundColourId, Colour(250, 250, 250) },
        { PlugDataColour::activeTabTextColourId, Colour(90, 90, 90) },

        { PlugDataColour::canvasBackgroundColourId, Colour(250, 250, 250) },
        { PlugDataColour::canvasTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::canvasDotsColourId, Colour(144, 144, 144) },

        { PlugDataColour::outlineColourId, Colour(200, 200, 200) },
        { PlugDataColour::dataColourId, Colour(0, 122, 255) },
        { PlugDataColour::connectionColourId, Colour(179, 179, 179) },
        { PlugDataColour::signalColourId, Colour(255, 133, 0) },

        { PlugDataColour::defaultObjectBackgroundColourId, Colour(228, 228, 228) },
        { PlugDataColour::objectOutlineColourId, Colour(168, 168, 168) },
        { PlugDataColour::objectSelectedOutlineColourId, Colour(0, 122, 255) },

        { PlugDataColour::dialogBackgroundColourId, Colour(228, 228, 228) },
        { PlugDataColour::panelBackgroundColourId, Colour(250, 250, 250) },
        { PlugDataColour::panelTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::panelActiveBackgroundColourId, Colour(235, 235, 235) },
        { PlugDataColour::panelActiveTextColourId, Colour(90, 90, 90) },

        { PlugDataColour::scrollbarThumbColourId, Colour(0, 122, 255) },
        { PlugDataColour::resizeableCornerColourId, Colour(0, 122, 255) },
        { PlugDataColour::gridLineColourId, Colour(0, 122, 255) },
        { PlugDataColour::caretColourId, Colour(0, 122, 255) },

        { PlugDataColour::sidebarBackgroundColourId, Colour(238, 238, 238) },
        { PlugDataColour::sidebarTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::sidebarActiveBackgroundColourId, Colour(217, 217, 217) },
        { PlugDataColour::sidebarActiveTextColourId, Colour(90, 90, 90) },

        { PlugDataColour::levelMeterActiveColourId, Colour(0, 122, 255) },
        { PlugDataColour::levelMeterInactiveColourId, Colour(238, 238, 238) },
        { PlugDataColour::levelMeterTrackColourId, Colour(90, 90, 90) },
        { PlugDataColour::levelMeterThumbColourId, Colour(122, 122, 122) },

        { PlugDataColour::popupMenuBackgroundColourId, Colour(228, 228, 228) },
        { PlugDataColour::popupMenuActiveBackgroundColourId, Colour(207, 207, 207) },
        { PlugDataColour::popupMenuTextColourId, Colour(90, 90, 90) },
        { PlugDataColour::popupMenuActiveTextColourId, Colour(90, 90, 90) }
    };

    inline static const std::map<String, std::map<PlugDataColour, Colour>> defaultThemes = {
        { "light", defaultLightTheme },
        { "dark", defaultDarkTheme }
    };

    inline static std::map<String, std::map<PlugDataColour, Colour>> colourSettings = defaultThemes;

    void resetColours()
    {
        colourSettings = defaultThemes;
    }

    void setThemeColour(String themeName, PlugDataColour colourId, Colour colour)
    {
        colourSettings[themeName][colourId] = colour;
    }

    void setTheme(bool useLightTheme)
    {
        if (useLightTheme) {
            setColours(colourSettings.at("light"));
        } else {
            setColours(colourSettings.at("dark"));
        }

        isUsingLightTheme = useLightTheme;
    }

    // TODO: swap this out for a string theme name perhaps?
    static inline bool isUsingLightTheme = true;
    std::unique_ptr<Drawable> folderImage;
};
