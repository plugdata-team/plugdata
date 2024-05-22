/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Constants.h"

inline std::map<PlugDataColour, std::tuple<String, String, String>> const PlugDataColourNames = {

    { toolbarBackgroundColourId, { "Toolbar background", "toolbar_background", "Toolbar" } },
    { toolbarTextColourId, { "Toolbar text", "toolbar_text", "Toolbar" } },
    { toolbarHoverColourId, { "Toolbar hover", "toolbar_hover", "Toolbar" } },
    { toolbarActiveColourId, { "Toolbar active text", "toolbar_active", "Toolbar" } },

    { tabBackgroundColourId, { "Tab background", "tabbar_background", "Tabbar" } },

    { tabTextColourId, { "Tab text", "tab_text", "Tabbar" } },
    { activeTabBackgroundColourId, { "Selected tab background", "selected_tab_background", "Tabbar" } },
    { activeTabTextColourId, { "Selected tab text", "selected_tab_text", "Tabbar" } },

    { canvasBackgroundColourId, { "Canvas background", "canvas_background", "Canvas" } },
    { canvasTextColourId, { "Canvas text", "canvas_text", "Canvas" } },
    { canvasDotsColourId, { "Canvas dots colour", "canvas_dots", "Canvas" } },

    { guiObjectBackgroundColourId, { "GUI object background", "default_object_background", "Object" } },
    { guiObjectInternalOutlineColour, { "GUI Object internal outline colour", "gui_internal_outline_colour", "Object" } },
    { textObjectBackgroundColourId, { "Object background", "text_object_background", "Object" } },
    { commentTextColourId, { "Comment text", "comment_text_colour", "Object" } },
    { objectOutlineColourId, { "Object outline", "object_outline_colour", "Object" } },
    { objectSelectedOutlineColourId, { "Selected object outline", "selected_object_outline_colour", "Object" } },

    { ioletAreaColourId, { "Inlet/Outlet area", "iolet_area_colour", "Inlet/Outlet" } },
    { ioletOutlineColourId, { "Inlet/Outlet outline", "iolet_outline_colour", "Inlet/Outlet" } },

    { dataColourId, { "Data colour", "data_colour", "Canvas" } },
    { connectionColourId, { "Connection", "connection_colour", "Canvas" } },
    { signalColourId, { "Signal", "signal_colour", "Canvas" } },
    { gemColourId, { "Gem", "gem_colour", "Canvas" } },
    { graphAreaColourId, { "Graph resizer", "graph_area", "Canvas" } },
    { gridLineColourId, { "Grid line", "grid_colour", "Canvas" } },

    { popupMenuBackgroundColourId, { "Popup menu background", "popup_background", "Popup Menu" } },
    { popupMenuActiveBackgroundColourId, { "Popup menu background active", "popup_background_active", "Popup Menu" } },
    { popupMenuTextColourId, { "Popup menu text", "popup_text", "Popup Menu" } },
    { popupMenuActiveTextColourId, { "Popup menu active text", "popup_active_text", "Popup Menu" } },
    { outlineColourId, { "Popup menu outline", "outline_colour", "Popup Menu" } },

    { dialogBackgroundColourId, { "Dialog background", "dialog_background", "Other" } },
    { caretColourId, { "Text editor caret", "caret_colour", "Other" } },
    { toolbarOutlineColourId, { "Outline", "toolbar_outline_colour", "Other" } },

    { levelMeterActiveColourId, { "Level meter active", "levelmeter_active", "Level Meter" } },
    { levelMeterBackgroundColourId, { "Level meter track", "levelmeter_background", "Level Meter" } },
    { levelMeterThumbColourId, { "Level meter thumb", "levelmeter_thumb", "Level Meter" } },

    { sliderThumbColourId, { "Slider thumb", "slider_thumb", "Other" } },
    { scrollbarThumbColourId, { "Scrollbar thumb", "scrollbar_thumb", "Other" } },

    { panelBackgroundColourId, { "Panel background", "panel_background", "Panel" } },
    { panelForegroundColourId, { "Panel foreground", "panel_foreground", "Panel" } },
    { panelTextColourId, { "Panel text", "panel_text", "Panel" } },
    { panelActiveBackgroundColourId, { "Panel background active", "panel_background_active", "Panel" } },
    { panelActiveTextColourId, { "Panel active text", "panel_active_text", "Panel" } },

    { sidebarBackgroundColourId, { "Sidebar background", "sidebar_colour", "Sidebar" } },
    { sidebarTextColourId, { "Sidebar text", "sidebar_text", "Sidebar" } },
    { sidebarActiveBackgroundColourId, { "Sidebar background active", "sidebar_background_active", "Sidebar" } },
    { sidebarActiveTextColourId, { "Sidebar active text", "sidebar_active_text", "Sidebar" } },
};

struct PlugDataLook : public LookAndFeel_V4 {

    // Makes sure fonts get initialised
    SharedResourcePointer<Fonts> fonts;

    PlugDataLook();

    void fillResizableWindowBackground(Graphics& g, int w, int h, BorderSize<int> const& border, ResizableWindow& window) override;

    void drawResizableWindowBorder(Graphics&, int w, int h, BorderSize<int> const& border, ResizableWindow&) override { }

    void drawTextButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown);

    void drawToolbarButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown);

    void drawCallOutBoxBackground(CallOutBox& box, Graphics& g, Path const& path, Image& cachedImage) override;

    int getCallOutBoxBorderSize(CallOutBox const& c) override;

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override;

    Font getTextButtonFont(TextButton& but, int buttonHeight) override;
    Font getLabelFont(Label& label) override;

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider::SliderStyle const style, Slider& slider) override;

    Button* createDocumentWindowButton(int buttonType) override;

    void positionDocumentWindowButtons(DocumentWindow& window,
        int titleBarX, int titleBarY, int titleBarW, int titleBarH,
        Button* minimiseButton,
        Button* maximiseButton,
        Button* closeButton,
        bool positionTitleBarButtonsOnLeft) override;

    Rectangle<int> getTabButtonExtraComponentBounds(TabBarButton const& button, Rectangle<int>& textArea, Component& comp) override;

    int getTabButtonBestWidth(TabBarButton& button, int tabDepth) override;

    int getTabButtonOverlap(int tabDepth) override;

    void drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override;

    void drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown, bool isForceDrawn);

    void drawTabButtonText(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override;

    void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, int const w, int const h) override { }

    Button* createTabBarExtrasButton() override;

    Font getTabButtonFont(TabBarButton&, float height) override;

    void drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height,
        bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, [[maybe_unused]] bool isMouseDown) override;

    void getIdealPopupMenuItemSize(String const& text, bool const isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override;

    Path getTickShape(float height) override;

    void drawPopupMenuItem(Graphics& g, Rectangle<int> const& area,
        bool const isSeparator, bool const isActive,
        bool const isHighlighted, bool const isTicked,
        bool const hasSubMenu, String const& text,
        String const& shortcutKeyText,
        Drawable const* icon, Colour const* const textColourToUse) override;

    int getMenuWindowFlags() override;

    int getPopupMenuBorderSize() override;

    void drawTreeviewPlusMinusBox(Graphics& g, Rectangle<float> const& area, Colour, bool isOpen, bool isMouseOver) override;

    void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override;

    Font getComboBoxFont(ComboBox&) override;

    PopupMenu::Options getOptionsForComboBoxPopupMenu(ComboBox& box, Label& label) override;

    void drawResizableFrame(Graphics& g, int w, int h, BorderSize<int> const& border) override { }

    void drawGUIObjectSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider& slider);

    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor) override;

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override;

    void drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    void drawLasso(Graphics& g, Component& lassoComp) override;
    
    void drawAlertBox (Graphics& g, AlertWindow& alert,
                       const Rectangle<int>& textArea, TextLayout& textLayout) override;

    void drawTooltip(Graphics& g, String const& text, int width, int height) override;

    void drawLabel(Graphics& g, Label& label) override;

    static Path getSquircle(Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft = true, bool const curveTopRight = true, bool const curveBottomLeft = true, bool const curveBottomRight = true);

    static void fillSmoothedRectangle(Graphics& g, Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft = true, bool const curveTopRight = true, bool const curveBottomLeft = true, bool const curveBottomRight = true);

    static void drawSmoothedRectangle(Graphics& g, PathStrokeType strokeType, Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft = true, bool const curveTopRight = true, bool const curveBottomLeft = true, bool const curveBottomRight = true);

    void drawPropertyComponentLabel(Graphics& g, int width, int height, PropertyComponent& component) override;

    void drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height) override;

    Rectangle<int> getTooltipBounds(String const& tipText, Point<int> screenPos, Rectangle<int> parentArea) override;

    int getTreeViewIndentSize(TreeView&) override;

    void setColours(std::map<PlugDataColour, Colour> colours);

    static void setDefaultFont(String const& fontName);

    static String const defaultThemesXml;

    static void resetColours(ValueTree themesTree);

    static Colour getThemeColour(ValueTree themeTree, PlugDataColour colourId);

    void setTheme(ValueTree themeTree);

    static StringArray getAllThemes();

    static bool getUseDashedConnections();
    static bool getUseStraightConnections();
    static bool getUseThinConnections();
    static bool getUseSquareIolets();

    static inline bool useDashedConnections = true;
    static inline bool useStraightConnections = false;
    static inline bool useThinConnections = false;
    static inline bool useSquareIolets = false;

    static inline String currentTheme = "light";
    static inline StringArray selectedThemes = { "light", "dark" };
};
