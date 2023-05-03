/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "Utility/StackShadow.h"
#include "Utility/SettingsFile.h"
#include "Constants.h"

inline const std::map<PlugDataColour, std::tuple<String, String, String>> PlugDataColourNames = {

    { toolbarBackgroundColourId, { "Toolbar background", "toolbar_background", "Toolbar" } },
    { toolbarTextColourId, { "Toolbar text", "toolbar_text", "Toolbar" } },
    { toolbarHoverColourId, { "Toolbar hover", "toolbar_hover", "Toolbar" } },
    { toolbarActiveColourId, { "Toolbar active text", "toolbar_active", "Toolbar" } },
    { toolbarOutlineColourId, { "Toolbar outline colour", "toolbar_outline_colour", "Toolbar" } },

    { tabBackgroundColourId, { "Tab background", "tabbar_background", "Tabbar" } },

    { tabTextColourId, { "Tab text", "tab_text", "Tabbar" } },
    { activeTabBackgroundColourId, { "Selected tab background", "selected_tab_background", "Tabbar" } },
    { activeTabTextColourId, { "Selected tab text", "selected_tab_text", "Tabbar" } },

    { canvasBackgroundColourId, { "Canvas background", "canvas_background", "Canvas" } },
    { canvasTextColourId, { "Canvas text", "canvas_text", "Canvas" } },
    { canvasDotsColourId, { "Canvas dots colour", "canvas_dots", "Canvas" } },
    { outlineColourId, { "Outline colour", "outline_colour", "Canvas" } },

    { guiObjectBackgroundColourId, { "GUI object background", "default_object_background", "Object" } },
    { guiObjectInternalOutlineColour, { "GUI Object internal outline colour", "gui_internal_outline_colour", "Object" } },
    { textObjectBackgroundColourId, { "Text object background", "text_object_background", "Object" } },
    { commentTextColourId, { "Comment text colour", "comment_text_colour", "Object" } },
    { objectOutlineColourId, { "Object outline colour", "object_outline_colour", "Object" } },
    { objectSelectedOutlineColourId, { "Selected object outline colour", "selected_object_outline_colour", "Object" } },

    { ioletAreaColourId, { "Inlet/Outlet area colour", "iolet_area_colour", "Inlet/Outlet" } },
    { ioletOutlineColourId, { "Inlet/Outlet outline colour", "iolet_outline_colour", "Inlet/Outlet" } },

    { dataColourId, { "Data colour", "data_colour", "Canvas" } },
    { connectionColourId, { "Connection colour", "connection_colour", "Canvas" } },
    { signalColourId, { "Signal colour", "signal_colour", "Canvas" } },
    { resizeableCornerColourId, { "Graph resizer", "graph_resizer", "Canvas" } },
    { gridLineColourId, { "Grid line colour", "grid_colour", "Canvas" } },

    { popupMenuBackgroundColourId, { "Popup menu background", "popup_background", "Popup Menu" } },
    { popupMenuActiveBackgroundColourId, { "Popup menu background active", "popup_background_active", "Popup Menu" } },
    { popupMenuTextColourId, { "Popup menu text", "popup_text", "Popup Menu" } },
    { popupMenuActiveTextColourId, { "Popup menu active text", "popup_active_text", "Popup Menu" } },

    { dialogBackgroundColourId, { "Dialog background", "dialog_background", "Other" } },
    { caretColourId, { "Text editor caret", "caret_colour", "Other" } },

    { levelMeterActiveColourId, { "Level meter active", "levelmeter_active", "Level Meter" } },
    { levelMeterInactiveColourId, { "Level meter inactive", "levelmeter_inactive", "Level Meter" } },

    { levelMeterTrackColourId, { "Level meter track", "levelmeter_track", "Level Meter" } },
    { levelMeterThumbColourId, { "Level meter thumb", "levelmeter_thumb", "Level Meter" } },

    { sliderThumbColourId, { "Slider thumb", "slider_thumb", "Other" } },
    { scrollbarThumbColourId, { "Scrollbar thumb", "scrollbar_thumb", "Other" } },

    { panelBackgroundColourId, { "Panel background", "panel_colour", "Panel" } },
    { panelTextColourId, { "Panel text", "panel_text", "Panel" } },
    { panelActiveBackgroundColourId, { "Panel background active", "panel_background_active", "Panel" } },
    { panelActiveTextColourId, { "Panel active text", "panel_active_text", "Panel" } },
    { searchBarColourId, { "Searchbar colour", "searchbar_colour", "Panel" } },

    { sidebarBackgroundColourId, { "Sidebar background", "sidebar_colour", "Sidebar" } },
    { sidebarTextColourId, { "Sidebar text", "sidebar_text", "Sidebar" } },
    { sidebarActiveBackgroundColourId, { "Sidebar background active", "sidebar_background_active", "Sidebar" } },
    { sidebarActiveTextColourId, { "Sidebar active text", "sidebar_active_text", "Sidebar" } },
};

struct PlugDataLook : public LookAndFeel_V4 {

    // Makes sure fonts get initialised
    SharedResourcePointer<Fonts> fonts;

    PlugDataLook()
    {
        setDefaultSansSerifTypeface(Fonts::getCurrentFont().getTypefacePtr());
    }

    class PlugData_DocumentWindowButton_macOS : public Button
        , public FocusChangeListener {
    public:
        PlugData_DocumentWindowButton_macOS(int buttonType)
            : Button("")
            , buttonType(buttonType)
        {
            Desktop::getInstance().addFocusChangeListener(this);

            auto crossThickness = 0.25f;
            String name;

            switch (buttonType) {
            case DocumentWindow::closeButton: {
                name = "close";
                bgColour = Colour(0xFFFF605C); // Sunset Orange (#FF605C)

                shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
                shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);
                toggledShape = shape;
                break;
            }
            case DocumentWindow::minimiseButton: {
                name = "minimise";
                bgColour = Colour(0xFFFFBD44); // Pastel Orange (#FFBD44)

                shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
                toggledShape = shape;
                break;
            }
            case DocumentWindow::maximiseButton: {
                name = "maximise";
                bgColour = Colour(0xFF00CA4E); // Malachite (#00CA4E)

                // we add a rectangle, and make it two triangles by drawing an oblique line on top
                shape.addRectangle(0.0f, 0.0f, 1.0f, 1.0f);

                // top triangle
                auto point_a_a = Point<float>(0.5f, 0.0f);
                auto point_a_b = Point<float>(0.5f, 0.5f);
                auto point_a_c = Point<float>(0.0f, 0.5f);
                // bottom triangle
                auto point_b_a = Point<float>(0.5f, 0.5f);
                auto point_b_b = Point<float>(1.0f, 0.5f);
                auto point_b_c = Point<float>(0.5f, 1.0f);

                toggledShape.addTriangle(point_a_a, point_a_b, point_a_c);
                toggledShape.addTriangle(point_b_a, point_b_b, point_b_c);
                break;
            }
            }
            setName(name);
            setButtonText(name);

            buttonColour = bgColour;
        }

        ~PlugData_DocumentWindowButton_macOS() override
        {
            Desktop::getInstance().removeFocusChangeListener(this);
        }

        void setWindow(DocumentWindow* window)
        {
            owner = window;
        }

        void globalFocusChanged(Component* focusedComponent) override
        {
            buttonColour = getParentComponent()->hasKeyboardFocus(true) ? bgColour : Colours::lightgrey;
            repaint();
        }

        void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto rect = Justification(Justification::centred).appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds()).toFloat();
            auto reducedRect = rect.reduced(getHeight() * 0.22f);
            auto reducedRectShape = reducedRect.reduced(getHeight() * 0.15f);

            for (auto* button : getAllButtons()) {
                if (button->isMouseOver())
                    shouldDrawButtonAsHighlighted = true;
            }

            auto finalColour = shouldDrawButtonAsDown ? buttonColour.darker(0.4f) : buttonColour;

            // draw macOS filled background circle
            g.setColour(finalColour);
            g.fillEllipse(reducedRect);

            // draw macOS circle border
            g.setColour(finalColour.darker(0.1f));
            g.drawEllipse(reducedRect, 1.0f);

            // draw icons on mouse hover
            if (shouldDrawButtonAsHighlighted) {
                auto p = shape;
                auto s = reducedRectShape;
                if (getToggleState()) {
                    p = toggledShape;
                    s = rect.reduced(getHeight() * 0.26f);
                }
                g.setColour(finalColour.darker(0.8f));
                g.fillPath(p, p.getTransformToScaleToFit(s, true));

                // perfectly fine hack to draw maximise macOS style button
                if (buttonType == DocumentWindow::maximiseButton && !getToggleState()) {
                    g.setColour(finalColour);
                    auto bar = Line<float>({ 0.0f, 1.0f, 1.0f, 0.0f });
                    Path barPath;
                    barPath.addLineSegment(bar, 0.3f);
                    auto rectBarSegment = rect.reduced(getHeight() * 0.3f);
                    g.fillPath(barPath, barPath.getTransformToScaleToFit(rectBarSegment, true));
                }
            }
        }

        void mouseEnter(MouseEvent const& e) override
        {
            for (auto* button : getAllButtons())
                button->repaint();
            Button::mouseEnter(e);
        }

        void mouseExit(MouseEvent const& e) override
        {
            for (auto* button : getAllButtons())
                button->repaint();
            Button::mouseExit(e);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            for (auto* button : getAllButtons())
                button->repaint();
            Button::mouseDrag(e);
        }

        std::vector<Button*> getAllButtons()
        {
            std::vector<Button*> allButtons;

            if (!owner)
                return allButtons;

            if (auto* minButton = owner->getMinimiseButton()) {
                allButtons.push_back(minButton);
            }
            if (auto* maxButton = owner->getMaximiseButton()) {
                allButtons.push_back(maxButton);
            }
            if (auto* closeButton = owner->getCloseButton()) {
                allButtons.push_back(closeButton);
            }

            return allButtons;
        }

    private:
        DocumentWindow* owner;
        Colour bgColour;
        Colour buttonColour;
        Path shape, toggledShape;
        int buttonType;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugData_DocumentWindowButton_macOS)
    };

    class PlugData_DocumentWindowButton : public Button {
    public:
        PlugData_DocumentWindowButton(int buttonType)
            : Button("")
            , buttonType(buttonType)
        {
            auto crossThickness = 0.15f;
            String name;

            switch (buttonType) {
            case DocumentWindow::closeButton: {
                name = "close";
                shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
                shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);
                toggledShape = shape;
                break;
            }
            case DocumentWindow::minimiseButton: {
                name = "minimise";
                shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
                toggledShape = shape;
                break;
            }
            case DocumentWindow::maximiseButton: {
                name = "maximise";
                shape.addLineSegment({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
                shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

                toggledShape.startNewSubPath(45.0f, 100.0f);
                toggledShape.lineTo(0.0f, 100.0f);
                toggledShape.lineTo(0.0f, 0.0f);
                toggledShape.lineTo(100.0f, 0.0f);
                toggledShape.lineTo(100.0f, 45.0f);
                toggledShape.addRectangle(45.0f, 45.0f, 100.0f, 100.0f);
                PathStrokeType(30.0f).createStrokedPath(toggledShape, toggledShape);
                break;
            }
            }
            setName(name);
            setButtonText(name);
        }

        void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto colour = findColour(TextButton::textColourOffId);

            g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f) : colour);

            if (shouldDrawButtonAsHighlighted) {
                g.setColour(findColour(Slider::thumbColourId));
            }

            auto& p = getToggleState() ? toggledShape : shape;

            auto reducedRect = Justification(Justification::centred).appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds()).toFloat().reduced(getHeight() * 0.3f);

            g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
        }

    private:
        Colour colour;
        Path shape, toggledShape;
        int buttonType;

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

    void drawTextButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto backgroundColour = findColour(shouldDrawButtonAsDown || button.getToggleState() ? PlugDataColour::dataColourId : PlugDataColour::canvasTextColourId);
        if (shouldDrawButtonAsHighlighted)
            backgroundColour = backgroundColour.brighter(0.5f);
        auto cornerSize = Corners::defaultCornerRadius;
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(button.getLocalBounds().toFloat(), cornerSize);
    }

    void drawToolbarButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        bool active = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown || button.getToggleState();
        

        auto cornerSize = Corners::defaultCornerRadius;
        auto flatOnLeft = button.isConnectedOnLeft();
        auto flatOnRight = button.isConnectedOnRight();
        auto flatOnTop = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {
            auto backgroundColour = findColour(active ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarBackgroundColourId);
            
            auto bounds = button.getLocalBounds().toFloat();
            bounds = bounds.reduced(0.0f, bounds.proportionOfHeight(0.17f));

            backgroundColour = backgroundColour.contrasting(0.05f);

            Path path;
            path.addRoundedRectangle(bounds.getX(), bounds.getY(),
                bounds.getWidth(), bounds.getHeight(),
                cornerSize, cornerSize,
                !(flatOnLeft || flatOnTop),
                !(flatOnRight || flatOnTop),
                !(flatOnLeft || flatOnBottom),
                !(flatOnRight || flatOnBottom));

            g.setColour(backgroundColour);
            g.fillPath(path);
        } else {
            auto backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
            auto bounds = button.getLocalBounds().toFloat().reduced(2.0f, 4.0f);

            g.setColour(backgroundColour);
            g.fillRoundedRectangle(bounds, cornerSize);
        }
    }

    void drawCallOutBoxBackground(CallOutBox& box, Graphics& g, Path const& path, Image& cachedImage) override
    {
        if (cachedImage.isNull()) {
            cachedImage = { Image::ARGB, box.getWidth(), box.getHeight(), true };
            Graphics g2(cachedImage);

            StackShadow::renderDropShadow(g2, path, Colour(0, 0, 0).withAlpha(0.3f), 7, { 0, 2 });
        }

        g.setColour(Colours::black);
        g.drawImageAt(cachedImage, 0, 0);

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillPath(path);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.strokePath(path, PathStrokeType(1.0f));
    }

    int getCallOutBoxBorderSize(CallOutBox const& c) override
    {
        return 20;
    }

    void drawButtonBackground(Graphics& g, Button& button, Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (button.getProperties()["Style"] == "LargeIcon") {
            drawToolbarButtonBackground(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties()["Style"].toString().contains("Icon")) {
            return;
        } else {
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown) override
    {
        if (button.getProperties()["Style"] == "LargeIcon") {
            button.setColour(TextButton::textColourOnId, findColour(PlugDataColour::toolbarTextColourId));
            button.setColour(TextButton::textColourOffId, findColour(PlugDataColour::toolbarTextColourId));
            
            g.saveState();
            g.addTransform(AffineTransform::translation(0.25f, 0.0f)); // This heuristic makes sure the icon appears exactly centred
            LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);
            g.restoreState();
        }
        else if (button.getProperties()["Style"] == "SmallIcon") {
            Font font(getTextButtonFont(button, button.getHeight()));
            g.setFont(font);

            if (!button.isEnabled()) {
                g.setColour(Colours::grey);
            } else if (button.getToggleState()) {
                g.setColour(button.findColour(PlugDataColour::toolbarActiveColourId));
            } else if (isMouseOverButton) {
                g.setColour(button.findColour(PlugDataColour::toolbarTextColourId).brighter(0.8f));
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
            
            auto flatOnLeft = but.isConnectedOnLeft();
            auto flatOnRight = but.isConnectedOnRight();
            auto flatOnTop = but.isConnectedOnTop();
            auto flatOnBottom = but.isConnectedOnBottom();
            
            if(flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
            {
                return Fonts::getIconFont().withHeight(buttonHeight * 0.345f);
            }
            
            return Fonts::getIconFont().withHeight(buttonHeight * 0.375f);
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

        Fonts::drawText(g, window.getName(), 0, 0, w, h, getCurrentColourScheme().getUIColour(ColourScheme::defaultText), h * 0.65f);
    }

    Button* createDocumentWindowButton(int buttonType) override
    {
        // For dialogs
        if (buttonType == 5)
            return new PlugData_DocumentWindowButton(4);

        if (SettingsFile::getInstance()->getProperty<bool>("macos_buttons"))
            return new PlugData_DocumentWindowButton_macOS(buttonType);
        else
            return new PlugData_DocumentWindowButton(buttonType);

        jassertfalse;
        return nullptr;
    }

    void positionDocumentWindowButtons(DocumentWindow& window,
        int titleBarX, int titleBarY, int titleBarW, int titleBarH,
        Button* minimiseButton,
        Button* maximiseButton,
        Button* closeButton,
        bool positionTitleBarButtonsOnLeft) override
    {
        if (window.isUsingNativeTitleBar())
            return;

        auto areButtonsLeft = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");

        // heuristic to offset the buttons when positioned left, as we are drawing larger to provide a shadow
        // we check if the system is drawing with a dropshadow- hence semi transparent will be true
#if JUCE_LINUX
        auto leftOffset = titleBarX;
        if (maximiseButton != nullptr && areButtonsLeft && ProjectInfo::canUseSemiTransparentWindows()) {
            if (maximiseButton->getToggleState())
                leftOffset += 8;
            else
                leftOffset += 25;
        }
#else
        auto leftOffset = areButtonsLeft && ProjectInfo::canUseSemiTransparentWindows() ? titleBarX + 12 : titleBarX;
#endif

        if (areButtonsLeft) {
            titleBarY += 3;
            titleBarH -= 4;
        }

        auto buttonW = static_cast<int>(titleBarH * 1.2);

        auto x = areButtonsLeft ? leftOffset : leftOffset + titleBarW - buttonW;

        auto setWindow = [](Button* button, DocumentWindow& window) {
            if (auto* b = dynamic_cast<PlugData_DocumentWindowButton_macOS*>(button))
                b->setWindow(&window);
        };

        if (closeButton != nullptr) {
            setWindow(closeButton, window);
            closeButton->setBounds(x, titleBarY, buttonW, titleBarH);
            x += areButtonsLeft ? titleBarH * 1.1 : -buttonW;
        }

        if (areButtonsLeft)
            std::swap(minimiseButton, maximiseButton);

        if (maximiseButton != nullptr) {
            setWindow(maximiseButton, window);
            maximiseButton->setBounds(x, titleBarY, buttonW, titleBarH);
            x += areButtonsLeft ? titleBarH * 1.1 : -buttonW;
        }

        if (minimiseButton != nullptr) {
            setWindow(minimiseButton, window);
            minimiseButton->setBounds(x, titleBarY, buttonW, titleBarH);
        }
    }
    
    Rectangle<int> getTabButtonExtraComponentBounds (const TabBarButton& button, Rectangle<int>& textArea, Component& comp) override
    {
        Rectangle<int> extraComp;
        
        auto area = textArea.reduced(4);

        auto orientation = button.getTabbedButtonBar().getOrientation();

        if (button.getExtraComponentPlacement() == TabBarButton::beforeText)
        {
            switch (orientation)
            {
                case TabbedButtonBar::TabsAtBottom:
                case TabbedButtonBar::TabsAtTop:     extraComp = area.removeFromLeft   (comp.getWidth()); break;
                case TabbedButtonBar::TabsAtLeft:    extraComp = area.removeFromBottom (comp.getHeight()); break;
                case TabbedButtonBar::TabsAtRight:   extraComp = area.removeFromTop    (comp.getHeight()); break;
                default:                             jassertfalse; break;
            }
        }
        else
        {
            switch (orientation)
            {
                case TabbedButtonBar::TabsAtBottom:
                case TabbedButtonBar::TabsAtTop:     extraComp = area.removeFromRight  (comp.getWidth()); break;
                case TabbedButtonBar::TabsAtLeft:    extraComp = area.removeFromTop    (comp.getHeight()); break;
                case TabbedButtonBar::TabsAtRight:   extraComp = area.removeFromBottom (comp.getHeight()); break;
                default:                             jassertfalse; break;
            }
        }

        return extraComp;
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
        
        if(isActive)
        {
            g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
        }
        else if(isMouseOver)
        {
            g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId).interpolatedWith(findColour(PlugDataColour::tabBackgroundColourId), 0.4f));
        }
        else {
            g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
        }

        g.fillRoundedRectangle(button.getLocalBounds().reduced(4).toFloat(), Corners::defaultCornerRadius);

        int w = button.getWidth();
        int h = button.getHeight();
        
        drawTabButtonText(button, g, false, false);
    }

    void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, int const w, int const h) override
    {
    }

    Font getTabButtonFont(TabBarButton&, float height) override
    {
        return Fonts::getCurrentFont().withHeight(height * 0.44f);
    }
    
    void getIdealPopupMenuItemSize (const String& text, const bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override
    {
        if (isSeparator)
        {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        }
        else
        {
            auto font = getPopupMenuFont();

            if (standardMenuItemHeight > 0 && font.getHeight() > (float) standardMenuItemHeight / 1.3f)
                font.setHeight ((float) standardMenuItemHeight / 1.3f);

            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : roundToInt (font.getHeight() * 1.3f);
            idealWidth = font.getStringWidth (text) + idealHeight * 2;
            
            idealHeight += 2;
        }
    }

    void drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options) override
    {
        auto background = findColour(PlugDataColour::popupMenuBackgroundColourId);
        
        // TODO: some popup menus are added to a component and some to desktop,
        // which makes it really hard to decide whether they can be transparent or not!
        // We can check it in this function by checking options.getParentComponent, but unfortunately not everywhere
        if (Desktop::canUseSemiTransparentWindows()) {
            Path shadowPath;
            shadowPath.addRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, height).reduced(10.0f), Corners::defaultCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 10, { 0, 2 });

            // Add a bit of alpha to disable the opaque flag

            g.setColour(background);

            auto bounds = Rectangle<float>(0, 0, width, height).reduced(7);
            g.fillRoundedRectangle(bounds, Corners::largeCornerRadius);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(bounds, Corners::largeCornerRadius, 1.0f);
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

            g.setColour(findColour(PlugDataColour::outlineColourId).withAlpha(0.7f));
            g.fillRect(r.removeFromTop(1));
        } else {
            auto r = area.reduced(margin, 1);

            auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
            if (isHighlighted && isActive) {
                g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                g.fillRoundedRectangle(r.toFloat().reduced(4, 0), Corners::smallCornerRadius);
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

                auto x = static_cast<float>(r.removeFromRight((int)arrowH + 3).getX());
                auto halfH = static_cast<float>(r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            Fonts::drawFittedText(g, text, r, colour);

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
        p.startNewSubPath(0.0f, 0.0f);
        p.lineTo(0.5f, 0.5f);
        p.lineTo(isOpen ? 1.0f : 0.0f, isOpen ? 0.0f : 1.0f);

        auto size = std::min(area.getWidth(), area.getHeight()) * 0.5f;
        g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
        g.strokePath(p, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(area.withSizeKeepingCentre(size, size), true));
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

        Rectangle<int> arrowZone(width - 22, 9, 14, height - 18);
        Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 2.0f);
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
        auto cornerSize = Corners::objectCornerRadius / 2.0f;

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
        corner = corner.createPathWithRoundedCorners(Corners::windowCornerRadius);
        corner.lineTo(0, h);

        g.setColour(findColour(PlugDataColour::resizeableCornerColourId).withAlpha(isMouseOver ? 1.0f : 0.6f));
        g.fillPath(corner);
    }

    void drawTooltip(Graphics& g, String const& text, int width, int height) override
    {
        auto bounds = Rectangle<float>(0, 0, width, height);
        auto cornerSize = Corners::defaultCornerRadius;

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

        Fonts::drawFittedText(g, component.getName(), indent, r.getY(), r.getX() - 5, r.getHeight(), colour, 1, 1.0f, (float)jmin(height, 24) * 0.65f, Justification::centredLeft);
    }

    void drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height) override
    {
        auto buttonSize = (float)height * 0.75f;
        auto buttonIndent = ((float)height - buttonSize) * 0.5f;

        drawTreeviewPlusMinusBox(g, { buttonIndent, buttonIndent, buttonSize, buttonSize }, findColour(ResizableWindow::backgroundColourId), isOpen, false);

        auto textX = static_cast<int>((buttonIndent * 2.0f + buttonSize + 2.0f));

        Fonts::drawStyledText(g, name, textX, 0, std::max(width - textX - 4, 0), height, findColour(PropertyComponent::labelTextColourId), Bold, height * 0.6f);
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
            colours.at(PlugDataColour::toolbarHoverColourId));
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
            colours.at(PlugDataColour::panelTextColourId));
        setColour(TableListBox::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(Label::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(Label::textWhenEditingColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(ListBox::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(TextEditor::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(PropertyComponent::labelTextColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(PopupMenu::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(KeyMappingEditorComponent::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(TabbedButtonBar::frontTextColourId,
            colours.at(PlugDataColour::activeTabTextColourId));
        setColour(TabbedButtonBar::tabTextColourId,
            colours.at(PlugDataColour::tabTextColourId));
        setColour(ToggleButton::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(ToggleButton::tickColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(ToggleButton::tickDisabledColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(ComboBox::arrowColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(DirectoryContentsDisplayComponent::textColourId,
            colours.at(PlugDataColour::panelTextColourId));
        setColour(Slider::textBoxTextColourId,
            colours.at(PlugDataColour::panelTextColourId));
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

    // clang-format off
    static inline const String defaultThemesXml = "<ColourThemes>\n"
    "    <Theme theme=\"max\" toolbar_background=\"ff333333\" toolbar_text=\"ffe4e4e4\"\n"
    "           toolbar_active=\"ff72aedf\" toolbar_hover=\"ff72aedf\" tabbar_background=\"ff333333\"\n"
    "           tab_text=\"ffe4e4e4\" selected_tab_background=\"ff494949\" selected_tab_text=\"ff72aedf\"\n"
    "           canvas_background=\"ffe5e5e5\" canvas_text=\"ffeeeeee\" canvas_dots=\"ff7f7f7f\"\n"
    "           default_object_background=\"ff333333\" object_outline_colour=\"ff696969\"\n"
    "           selected_object_outline_colour=\"ff72aedf\" gui_internal_outline_colour=\"ff696969\" toolbar_outline_colour=\"ff393939\" outline_colour=\"ff393939\"\n"
    "           data_colour=\"ff72aedf\" connection_colour=\"ffb3b3b3\" signal_colour=\"ffe1ef00\"\n"
    "           dialog_background=\"ff333333\" sidebar_colour=\"ff3e3e3e\" sidebar_text=\"ffe4e4e4\"\n"
    "           sidebar_background_active=\"ff72aedf\" sidebar_active_text=\"ffe4e4e4\"\n"
    "           levelmeter_active=\"ff72aedf\" levelmeter_inactive=\"ff5d5d5d\" levelmeter_track=\"ff333333\"\n"
    "           levelmeter_thumb=\"ffe4e4e4\" panel_colour=\"ff232323\" panel_text=\"ffe4e4e4\"\n"
    "           panel_background_active=\"ff72aedf\" panel_active_text=\"ffe4e4e4\"\n"
    "           popup_background=\"ff333333\" popup_background_active=\"ff72aedf\"\n"
    "           popup_text=\"ffe4e4e4\" popup_active_text=\"ffe4e4e4\" slider_thumb=\"ff72aedf\"\n"
    "           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff72aedf\" grid_colour=\"ff72aedf\"\n"
    "           caret_colour=\"ff72aedf\" iolet_area_colour=\"ff808080\" iolet_outline_colour=\"ff696969\"\n"
    "           text_object_background=\"ff333333\" comment_text_colour=\"ff111111\"\n"
    "           searchbar_colour=\"ff3e3e3e\" dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
    "           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"1\"/>\n"
    "    <Theme theme=\"classic\" toolbar_background=\"ffffffff\" toolbar_text=\"ff000000\"\n"
    "           toolbar_active=\"ff787878\" toolbar_hover=\"ffaaaaaa\" tabbar_background=\"ffffffff\"\n"
    "           tab_text=\"ff000000\" selected_tab_background=\"ffa0a0a0\" selected_tab_text=\"ff000000\"\n"
    "           canvas_background=\"ffffffff\" canvas_text=\"ff000000\" canvas_dots=\"ffffffff\"\n"
    "           default_object_background=\"ffffffff\" text_object_background=\"ffffffff\"\n"
    "           object_outline_colour=\"ff000000\" selected_object_outline_colour=\"ff000000\"\n"
    "           gui_internal_outline_colour=\"ff000000\" toolbar_outline_colour=\"ff000000\" outline_colour=\"ff000000\" iolet_area_colour=\"ffffffff\" iolet_outline_colour=\"ff000000\"\n"
    "           data_colour=\"ff000000\" connection_colour=\"ff000000\" signal_colour=\"ff000000\"\n"
    "           dialog_background=\"ffffffff\" sidebar_colour=\"ffffffff\" sidebar_text=\"ff000000\"\n"
    "           sidebar_background_active=\"ff000000\" sidebar_active_text=\"ffffffff\"\n"
    "           levelmeter_active=\"ff000000\" levelmeter_inactive=\"ffffffff\" levelmeter_track=\"ff000000\"\n"
    "           levelmeter_thumb=\"ff000000\" panel_colour=\"ffffffff\" panel_text=\"ff000000\"\n"
    "           panel_background_active=\"ff000000\" panel_active_text=\"ffffffff\"\n"
    "           popup_background=\"ffffffff\" popup_background_active=\"ff000000\"\n"
    "           popup_text=\"ff000000\" popup_active_text=\"ffffffff\" slider_thumb=\"ff000000\"\n"
    "           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff000000\" grid_colour=\"ff000000\"\n"
    "           caret_colour=\"ff000000\" comment_text_colour=\"ff000000\" searchbar_colour=\"ffffffff\"\n"
    "           dashed_signal_connections=\"0\" straight_connections=\"1\"\n"
    "           thin_connections=\"1\" square_iolets=\"1\" square_object_corners=\"1\"/>\n"
    "    <Theme theme=\"classic_dark\" toolbar_background=\"ff000000\" toolbar_text=\"ffffffff\"\n"
    "           toolbar_active=\"ff787878\" toolbar_hover=\"ff888888\" tabbar_background=\"ff000000\"\n"
    "           tab_text=\"ffffffff\" selected_tab_background=\"ff808080\" selected_tab_text=\"ffffffff\"\n"
    "           canvas_background=\"ff000000\" canvas_text=\"ffffffff\" canvas_dots=\"ff000000\"\n"
    "           default_object_background=\"ff000000\" object_outline_colour=\"ffffffff\"\n"
    "           selected_object_outline_colour=\"ffffffff\" gui_internal_outline_colour=\"ffffffff\" toolbar_outline_colour=\"ffffffff\" outline_colour=\"ffffffff\"\n"
    "           data_colour=\"ffffffff\" connection_colour=\"ffffffff\" signal_colour=\"ffffffff\"\n"
    "           dialog_background=\"ff000000\" sidebar_colour=\"ff000000\" sidebar_text=\"ffffffff\"\n"
    "           sidebar_background_active=\"ffffffff\" sidebar_active_text=\"ff000000\"\n"
    "           levelmeter_active=\"ffffffff\" levelmeter_inactive=\"ff000000\" levelmeter_track=\"ffffffff\"\n"
    "           levelmeter_thumb=\"ffffffff\" panel_colour=\"ff000000\" panel_text=\"ffffffff\"\n"
    "           panel_background_active=\"ffffffff\" panel_active_text=\"ff000000\"\n"
    "           popup_background=\"ff000000\" popup_background_active=\"ffffffff\"\n"
    "           popup_text=\"ffffffff\" popup_active_text=\"ff000000\" slider_thumb=\"ffffffff\"\n"
    "           scrollbar_thumb=\"ff7f7f7f\" graph_resizer=\"ffffffff\" grid_colour=\"ffffffff\"\n"
    "           caret_colour=\"ffffffff\" iolet_area_colour=\"ff000000\" iolet_outline_colour=\"ffffffff\"\n"
    "           text_object_background=\"ff000000\" comment_text_colour=\"ffffffff\"\n"
    "           searchbar_colour=\"ff000000\" dashed_signal_connections=\"0\" straight_connections=\"1\"\n"
    "           thin_connections=\"1\" square_iolets=\"1\" square_object_corners=\"1\"/>\n"
    "    <Theme theme=\"dark\" toolbar_background=\"ff191919\" toolbar_text=\"ffe1e1e1\"\n"
    "           toolbar_active=\"ff42a2c8\" toolbar_hover=\"ff282828\" tabbar_background=\"ff191919\"\n"
    "           tab_text=\"ffe1e1e1\" selected_tab_background=\"ff2e2e2e\" selected_tab_text=\"ffe1e1e1\"\n"
    "           canvas_background=\"ff232323\" canvas_text=\"ffe1e1e1\" canvas_dots=\"ff7f7f7f\"\n"
    "           default_object_background=\"ff191919\" object_outline_colour=\"ff696969\"\n"
    "           selected_object_outline_colour=\"ff42a2c8\" gui_internal_outline_colour=\"ff696969\" toolbar_outline_colour=\"ff2f2f2f\" outline_colour=\"ff393939\"\n"
    "           data_colour=\"ff42a2c8\" connection_colour=\"ffe1e1e1\" signal_colour=\"ffff8500\"\n"
    "           dialog_background=\"ff191919\" sidebar_colour=\"ff191919\" sidebar_text=\"ffe1e1e1\"\n"
    "           sidebar_background_active=\"ff282828\" sidebar_active_text=\"ffe1e1e1\"\n"
    "           levelmeter_active=\"ff42a2c8\" levelmeter_inactive=\"ff2d2d2d\" levelmeter_track=\"ffe3e3e3\"\n"
    "           levelmeter_thumb=\"ffe3e3e3\" panel_colour=\"ff232323\" panel_text=\"ffe1e1e1\"\n"
    "           panel_background_active=\"ff373737\" panel_active_text=\"ffe1e1e1\"\n"
    "           popup_background=\"ff191919\" popup_background_active=\"ff282828\"\n"
    "           popup_text=\"ffe1e1e1\" popup_active_text=\"ffe1e1e1\" slider_thumb=\"ff42a2c8\"\n"
    "           scrollbar_thumb=\"ff7f7f7f\" graph_resizer=\"ff42a2c8\" grid_colour=\"ff42a2c8\"\n"
    "           caret_colour=\"ff42a2c8\" text_object_background=\"ff232323\" iolet_area_colour=\"ff232323\"\n"
    "           iolet_outline_colour=\"ff696969\" comment_text_colour=\"ffe1e1e1\"\n"
    "           searchbar_colour=\"ff232323\" dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
    "           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"0\"/>\n"
    "    <Theme theme=\"light\" toolbar_background=\"ffebebeb\" toolbar_text=\"ff4d4d4d\"\n"
    "           toolbar_active=\"ff007aff\" toolbar_hover=\"ffd9d9d9\" tabbar_background=\"ffebebeb\"\n"
    "           tab_text=\"ff4d4d4d\" selected_tab_background=\"ffdedede\" selected_tab_text=\"ff4d4d4d\"\n"
    "           canvas_background=\"fffafafa\" canvas_text=\"ff4d4d4d\" canvas_dots=\"ff909090\"\n"
    "           default_object_background=\"ffe4e4e4\" object_outline_colour=\"ffa8a8a8\"\n"
    "           selected_object_outline_colour=\"ff007aff\" gui_internal_outline_colour=\"ffa8a8a8\" toolbar_outline_colour=\"ffdbdbdb\" outline_colour=\"ffc8c8c8\"\n"
    "           data_colour=\"ff007aff\" connection_colour=\"ffb3b3b3\" signal_colour=\"ffff8500\"\n"
    "           dialog_background=\"ffe4e4e4\" sidebar_colour=\"ffefefef\" sidebar_text=\"ff4d4d4d\"\n"
    "           sidebar_background_active=\"ffdfdfdf\" sidebar_active_text=\"ff4d4d4d\"\n"
    "           levelmeter_active=\"ff007aff\" levelmeter_inactive=\"fff6f6f6\" levelmeter_track=\"ff4d4d4d\"\n"
    "           levelmeter_thumb=\"ff7a7a7a\" panel_colour=\"fffafafa\" panel_text=\"ff4d4d4d\"\n"
    "           panel_background_active=\"ffebebeb\" panel_active_text=\"ff4d4d4d\"\n"
    "           popup_background=\"ffe6e6e6\" popup_background_active=\"ffd5d5d5\"\n"
    "           popup_text=\"ff4d4d4d\" popup_active_text=\"ff4d4d4d\" slider_thumb=\"ff007aff\"\n"
    "           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff007aff\" grid_colour=\"ff007aff\"\n"
    "           caret_colour=\"ff007aff\" square_object_corners=\"0\"\n"
    "           text_object_background=\"fffafafa\" iolet_area_colour=\"fffafafa\"\n"
    "           iolet_outline_colour=\"ffa8a8a8\" comment_text_colour=\"ff4d4d4d\"\n"
    "           searchbar_colour=\"fffafafa\" dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
    "           thin_connections=\"0\" square_iolets=\"0\"/>\n"
    "    <Theme theme=\"warm\" toolbar_background=\"ffd2cdc4\" toolbar_text=\"ff5a5a5a\"\n"
    "           toolbar_active=\"ff5da0c4\" toolbar_hover=\"ffc0bbb2\" tabbar_background=\"ffd2cdc4\"\n"
    "           tab_text=\"ff5a5a5a\" selected_tab_background=\"ffc0bbb2\" selected_tab_text=\"ff5a5a5a\"\n"
    "           canvas_background=\"ffe3dfd9\" canvas_text=\"ff5a5a5a\" canvas_dots=\"ff909090\"\n"
    "           default_object_background=\"ffe3dfd9\" object_outline_colour=\"ff968e82\"\n"
    "           selected_object_outline_colour=\"ff5da0c4\" gui_internal_outline_colour=\"ff968e82\" toolbar_outline_colour=\"ffbdb3a4\" outline_colour=\"ff968e82\"\n"
    "           data_colour=\"ff5da0c4\" connection_colour=\"ffb3b3b3\" signal_colour=\"ffff8502\"\n"
    "           dialog_background=\"ffd2cdc4\" sidebar_colour=\"ffdedad3\" sidebar_text=\"ff5a5a5a\"\n"
    "           sidebar_background_active=\"ffefefef\" sidebar_active_text=\"ff5a5a5a\"\n"
    "           levelmeter_active=\"ff5da0c4\" levelmeter_inactive=\"ffd2cdc4\" levelmeter_track=\"ff5a5a5a\"\n"
    "           levelmeter_thumb=\"ff7a7a7a\" panel_colour=\"ffe3dfd9\" panel_text=\"ff5a5a5a\"\n"
    "           panel_background_active=\"ffebebeb\" panel_active_text=\"ff5a5a5a\"\n"
    "           popup_background=\"ffd2cdc4\" popup_background_active=\"ffc0bbb2\"\n"
    "           popup_text=\"ff5a5a5a\" popup_active_text=\"ff5a5a5a\" slider_thumb=\"ff5da0c4\"\n"
    "           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff5da0c4\" grid_colour=\"ff5da0c4\"\n"
    "           caret_colour=\"ff5da0c4\" iolet_area_colour=\"ffe3dfd9\" iolet_outline_colour=\"ff968e82\"\n"
    "           text_object_background=\"ffe3dfd9\" comment_text_colour=\"ff5a5a5a\"\n"
    "           searchbar_colour=\"ffe3dfd9\" dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
    "           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"0\"/>\n"
    "    <Theme theme=\"fangs\" toolbar_background=\"ff232323\" toolbar_text=\"ffffffff\"\n"
    "           toolbar_active=\"ff5bcefa\" toolbar_hover=\"ff383838\" tabbar_background=\"ff232323\"\n"
    "           tab_text=\"ffffffff\" selected_tab_background=\"ff3a3a3a\" selected_tab_text=\"ffffffff\"\n"
    "           canvas_background=\"ff383838\" canvas_text=\"ffffffff\" canvas_dots=\"ffa0a0a0\"\n"
    "           default_object_background=\"ff191919\" object_outline_colour=\"ff383838\"\n"
    "           selected_object_outline_colour=\"ffffacab\" gui_internal_outline_colour=\"ff626262\" toolbar_outline_colour=\"ff343434\" outline_colour=\"ff383838\"\n"
    "           data_colour=\"ff5bcefa\" connection_colour=\"ffa0a0a0\" signal_colour=\"ffffacab\"\n"
    "           dialog_background=\"ff191919\" sidebar_colour=\"ff232323\" sidebar_text=\"ffffffff\"\n"
    "           sidebar_background_active=\"ff383838\" sidebar_active_text=\"ffffffff\"\n"
    "           levelmeter_active=\"ff5bcefa\" levelmeter_inactive=\"ff2d2d2d\" levelmeter_track=\"fff5f5f5\"\n"
    "           levelmeter_thumb=\"fff5f5f5\" panel_colour=\"ff383838\" panel_text=\"ffffffff\"\n"
    "           panel_background_active=\"ff232323\" panel_active_text=\"ffffffff\"\n"
    "           popup_background=\"ff232323\" popup_background_active=\"ff383838\"\n"
    "           popup_text=\"ffffffff\" popup_active_text=\"ffffffff\" scrollbar_thumb=\"ff8e8e8e\"\n"
    "           graph_resizer=\"ff5bcefa\" grid_colour=\"ff5bcefa\" caret_colour=\"ffffacab\" text_object_background=\"ff232323\"\n"
    "           iolet_area_colour=\"ff232323\" iolet_outline_colour=\"ff696969\"\n"
    "           slider_thumb=\"ff8e8e8e\" comment_text_colour=\"ffffffff\" searchbar_colour=\"ff383838\"\n"
    "           dashed_signal_connections=\"1\" straight_connections=\"0\" thin_connections=\"1\"\n"
    "           square_iolets=\"1\" square_object_corners=\"0\"/>\n"
    "  </ColourThemes>";
    // clang-format on

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

        Corners::objectCornerRadius = themeTree.getProperty("square_object_corners") ? 0.0f : 2.75f;
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
};
