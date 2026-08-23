/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <bit>
#include <nanovg_async.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/SettingsFile.h"
#include "Constants.h"

struct PlugDataColours {
    Colour toolbarBackgroundColour;
    Colour toolbarTextColour;
    Colour toolbarActiveColour;
    Colour toolbarHoverColour;
    Colour toolbarOutlineColour;
    Colour activeTabBackgroundColour;
    Colour canvasBackgroundColour;
    Colour canvasTextColour;
    Colour canvasDotsColour;
    Colour presentationBackgroundColour;
    Colour guiObjectBackgroundColour;
    Colour guiObjectInternalOutlineColour;
    Colour textObjectBackgroundColour;
    Colour objectOutlineColour;
    Colour objectSelectedOutlineColour;
    Colour commentTextColour;
    Colour outlineColour;
    Colour ioletAreaColour;
    Colour ioletOutlineColour;
    Colour dataColour;
    Colour connectionColour;
    Colour signalColour;
    Colour gemColour;
    Colour dialogBackgroundColour;
    Colour sidebarBackgroundColour;
    Colour sidebarTextColour;
    Colour sidebarActiveBackgroundColour;
    Colour panelBackgroundColour;
    Colour panelForegroundColour;
    Colour panelTextColour;
    Colour panelActiveBackgroundColour;
    Colour popupMenuBackgroundColour;
    Colour popupMenuActiveBackgroundColour;
    Colour popupMenuTextColour;
    Colour scrollbarThumbColour;
    Colour graphAreaColour;
    Colour gridLineColour;
    Colour caretColour;
};

static inline NVGcolor nvgColour(Colour const& c)
{
    return std::bit_cast<NVGcolor>(c); // Both store as BGRA internally
}

inline UnorderedMap<PlugDataColour, std::tuple<String, String, String>> const PlugDataColourNames = {

    { toolbarBackgroundColourId, { "Toolbar background", "toolbar_background", "Toolbar" } },
    { toolbarTextColourId, { "Toolbar text", "toolbar_text", "Toolbar" } },
    { toolbarHoverColourId, { "Toolbar hover", "toolbar_hover", "Toolbar" } },
    { toolbarActiveColourId, { "Toolbar active text", "toolbar_active", "Toolbar" } },
    { activeTabBackgroundColourId, { "Selected tab background", "selected_tab_background", "Toolbar" } },

    { canvasBackgroundColourId, { "Canvas background", "canvas_background", "Canvas" } },
    { canvasTextColourId, { "Canvas text", "canvas_text", "Canvas" } },
    { canvasDotsColourId, { "Canvas dots colour", "canvas_dots", "Canvas" } },
    { presentationBackgroundColourId, { "Presentation background", "presentation_background", "Canvas" } },
    { dataColourId, { "Data colour", "data_colour", "Canvas" } },
    { connectionColourId, { "Connection", "connection_colour", "Canvas" } },
    { signalColourId, { "Signal", "signal_colour", "Canvas" } },
    { gemColourId, { "Gem", "gem_colour", "Canvas" } },
    { graphAreaColourId, { "Graph resizer", "graph_area", "Canvas" } },
    { gridLineColourId, { "Grid line", "grid_colour", "Canvas" } },

    { guiObjectBackgroundColourId, { "GUI object background", "default_object_background", "Object" } },
    { guiObjectInternalOutlineColourId, { "GUI object internal outline colour", "gui_internal_outline_colour", "Object" } },
    { textObjectBackgroundColourId, { "Object background", "text_object_background", "Object" } },
    { commentTextColourId, { "Comment text", "comment_text_colour", "Object" } },
    { objectOutlineColourId, { "Object outline", "object_outline_colour", "Object" } },
    { objectSelectedOutlineColourId, { "Selected object outline", "selected_object_outline_colour", "Object" } },
    { ioletAreaColourId, { "Inlet/Outlet area", "iolet_area_colour", "Object" } },
    { ioletOutlineColourId, { "Inlet/Outlet outline", "iolet_outline_colour", "Object" } },

    { popupMenuBackgroundColourId, { "Popup menu background", "popup_background", "Popup Menu" } },
    { popupMenuActiveBackgroundColourId, { "Popup menu background active", "popup_background_active", "Popup Menu" } },
    { popupMenuTextColourId, { "Popup menu text", "popup_text", "Popup Menu" } },
    { outlineColourId, { "Popup menu outline", "outline_colour", "Popup Menu" } },

    { dialogBackgroundColourId, { "Dialog background", "dialog_background", "Other" } },
    { caretColourId, { "Text editor caret", "caret_colour", "Other" } },
    { toolbarOutlineColourId, { "Outline", "toolbar_outline_colour", "Other" } },
    { scrollbarThumbColourId, { "Scrollbar thumb", "scrollbar_thumb", "Other" } },

    { panelBackgroundColourId, { "Panel background", "panel_background", "Properties Panel" } },
    { panelForegroundColourId, { "Panel foreground", "panel_foreground", "Properties Panel" } },
    { panelTextColourId, { "Panel text", "panel_text", "Properties Panel" } },
    { panelActiveBackgroundColourId, { "Panel background active", "panel_background_active", "Properties Panel" } },

    { sidebarBackgroundColourId, { "Sidebar background", "sidebar_colour", "Sidebar" } },
    { sidebarTextColourId, { "Sidebar text", "sidebar_text", "Sidebar" } },
    { sidebarActiveBackgroundColourId, { "Sidebar background active", "sidebar_background_active", "Sidebar" } },
};

struct PlugDataLook final : public LookAndFeel_V4
    , public SettingsFileListener {

    // Makes sure fonts get initialised
    SharedResourcePointer<Fonts> fonts;

    PlugDataLook();

    void settingsChanged(String const& name, var const& value) override;

    void fillResizableWindowBackground(Graphics& g, int w, int h, BorderSize<int> const& border, ResizableWindow& window) override;

    void drawResizableWindowBorder(Graphics&, int w, int h, BorderSize<int> const& border, ResizableWindow&) override { }

    void drawCallOutBoxBackground(CallOutBox& box, Graphics& g, Path const& path, Image& cachedImage) override;

    int getCallOutBoxBorderSize(CallOutBox const& c) override;

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override;

    Font getTextButtonFont(TextButton& but, int buttonHeight) override;

    Button* createDocumentWindowButton(int buttonType) override;

    void positionDocumentWindowButtons(DocumentWindow& window,
        int titleBarX, int titleBarY, int titleBarW, int titleBarH,
        Button* minimiseButton,
        Button* maximiseButton,
        Button* closeButton,
        bool positionTitleBarButtonsOnLeft) override;

    Font getTabButtonFont(TabBarButton&, float height) override;

    void drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height,
        bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, [[maybe_unused]] bool isMouseDown) override;

    void getIdealPopupMenuItemSize(String const& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override;

    Path getTickShape(float height) override;

    void drawPopupMenuItem(Graphics& g, Rectangle<int> const& area,
        bool isSeparator, bool isActive,
        bool isHighlighted, bool isTicked,
        bool hasSubMenu, String const& text,
        String const& shortcutKeyText,
        Drawable const* icon, Colour const* textColourToUse) override;

    int getMenuWindowFlags() override;

    int getPopupMenuBorderSize() override;

    void drawTreeviewPlusMinusBox(Graphics& g, Rectangle<float> const& area, Colour, bool isOpen, bool isMouseOver) override;

    void drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object) override;

    Font getComboBoxFont(ComboBox&) override;

    PopupMenu::Options getOptionsForComboBoxPopupMenu(ComboBox& box, Label& label) override;

    void drawResizableFrame(Graphics& g, int w, int h, BorderSize<int> const& border) override { }

    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor) override;

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor) override;

    void drawSpinningWaitAnimation(Graphics& g, Colour const& colour, int x, int y, int w, int h) override;

    void drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    void drawLasso(Graphics& g, Component& lassoComp) override;

    void drawAlertBox(Graphics& g, AlertWindow& alert,
        Rectangle<int> const& textArea, TextLayout& textLayout) override;

    void drawTooltip(Graphics& g, String const& text, int width, int height) override;

    void drawLabel(Graphics& g, Label& label) override;

    void drawPropertyComponentLabel(Graphics& g, int width, int height, PropertyComponent& component) override;

    void drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height) override;

    void drawTableHeaderBackground(Graphics&, TableHeaderComponent&) override;

    void drawTableHeaderColumn(Graphics&, TableHeaderComponent&,
        String const& columnName, int columnId,
        int width, int height,
        bool isMouseOver, bool isMouseDown, int columnFlags) override;

    Rectangle<int> getTooltipBounds(String const& tipText, Point<int> screenPos, Rectangle<int> parentArea) override;

    int getTreeViewIndentSize(TreeView&) override;

    void setColours(UnorderedMap<PlugDataColour, Colour>& colours);

    void setTheme(DynamicObject::Ptr themeTree);

    PlugDataColours const& getColours() const noexcept { return colours; }
    String const& getCurrentTheme() const noexcept { return currentTheme; }

    enum ConnectionStyle {
        ConnectionStyleDefault = 1,
        ConnectionStyleVanilla,
        ConnectionStyleThin
    };
    static inline StringArray selectedThemes = { "light", "dark" };

    static StringArray getAllThemes();
    static void setDefaultFont(String const& fontName);
    static void resetColours();
    static Colour getThemeColour(DynamicObject::Ptr themeTree, PlugDataColour colourId);
    void applyDefaultFont();

    ConnectionStyle getConnectionStyle() const noexcept { return useConnectionStyle; }
    bool getUseStraightConnections() const noexcept { return useStraightConnections; }
    bool getUseFlagOutline() const noexcept { return useFlagOutline; }
    bool getUseSyntaxHighlighting() const noexcept { return useSyntaxHighlighting; }
    bool getUseIoletSpacingEdge() const noexcept { return useIoletSpacingEdge; }
    bool getUseSquareIolets() const noexcept { return useSquareIolets; }
    bool getUseGradientConnectionLook() const noexcept { return useGradientConnectionLook; }
    bool isFixedIoletPosition() const noexcept { return useSquareIolets && useIoletSpacingEdge; }
    int getIoletSize() const noexcept { return useTouchMode ? 15 : 13; }
    float getObjectCornerRadius() const noexcept { return objectCornerRadius; }

#if JUCE_IOS
    void setMainComponent(Component* c) { mainComponent = c; }
    Component::SafePointer<Component> mainComponent;
#endif

    static String const defaultThemesJSON;

private:
    PlugDataColours colours;
    ConnectionStyle useConnectionStyle = ConnectionStyleDefault;
    bool useFlagOutline = false;
    bool useSyntaxHighlighting = false;
    bool useSquareIolets = false;
    bool useIoletSpacingEdge = false;
    bool useGradientConnectionLook = false;
    bool useStraightConnections = false;
    bool useTouchMode = false;
    float objectCornerRadius = 2.75f;
    String currentTheme = "light";
};

inline PlugDataLook& getPlugDataLook(Component& component) noexcept
{
    return static_cast<PlugDataLook&>(component.getLookAndFeel());
}

inline PlugDataLook const& getPlugDataLook(Component const& component) noexcept
{
    return static_cast<PlugDataLook const&>(component.getLookAndFeel());
}

inline PlugDataColours const& getThemeColours(Component const& component) noexcept
{
    return getPlugDataLook(component).getColours();
}
