/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_extra/juce_gui_extra.h>

#include "LookAndFeel.h"
#include "Utility/StackShadow.h"
#include "Utility/SettingsFile.h"
#include "Constants.h"
#include "Utility/Fonts.h"
#include "Tabbar/TabBarButtonComponent.h"

class PlugData_DocumentWindowButton_macOS : public Button
    , public FocusChangeListener {
public:
    explicit PlugData_DocumentWindowButton_macOS(int buttonType)
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
        default:
            break;
        }
        setName(name);
        setButtonText(name);

        buttonColour = bgColour;
    }

    ~PlugData_DocumentWindowButton_macOS()
    {
        Desktop::getInstance().removeFocusChangeListener(this);
    }

    void setWindow(DocumentWindow* window)
    {
        owner = window;
    }

    void globalFocusChanged(Component* focusedComponent)
    {
        buttonColour = getParentComponent()->hasKeyboardFocus(true) ? bgColour : Colours::lightgrey;
        repaint();
    }

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto rect = Justification(Justification::centred).appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds()).toFloat();
        auto reducedRect = rect.reduced(getHeight() * 0.22f);
        auto reducedRectShape = reducedRect.reduced(getHeight() * 0.15f);

        for (auto* button : getAllButtons()) {
            if (button->isMouseOver())
                shouldDrawButtonAsHighlighted = true;
        }

        auto finalColour = shouldDrawButtonAsDown ? buttonColour.darker(0.4f) : buttonColour;
        if (!isEnabled()) {
            finalColour = finalColour.interpolatedWith(Colours::black, 0.5f);
        }

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

    void mouseEnter(MouseEvent const& e)
    {
        for (auto* button : getAllButtons())
            button->repaint();
        Button::mouseEnter(e);
    }

    void mouseExit(MouseEvent const& e)
    {
        for (auto* button : getAllButtons())
            button->repaint();
        Button::mouseExit(e);
    }

    void mouseDrag(MouseEvent const& e)
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
    DocumentWindow* owner = nullptr;
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
    {
        auto crossThickness = 0.2f;
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

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto circleColour = findColour(PlugDataColour::toolbarHoverColourId);
        if (shouldDrawButtonAsHighlighted)
            circleColour = circleColour.contrasting(0.04f);

        if (!isEnabled()) {
            circleColour = circleColour.interpolatedWith(Colours::black, 0.5f);
        }

        g.setColour(circleColour);
        g.fillEllipse(getLocalBounds().withSizeKeepingCentre(getWidth() - 8, getWidth() - 8).toFloat());

        auto colour = findColour(TextButton::textColourOffId);
        g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f) : colour);

        auto& p = getToggleState() ? toggledShape : shape;

        auto reducedRect = Justification(Justification::centred).appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds()).toFloat().reduced(getHeight() * 0.35f);

        g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
    }

private:
    Colour colour;
    Path shape, toggledShape;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugData_DocumentWindowButton)
};

PlugDataLook::PlugDataLook()
{
    setDefaultSansSerifTypeface(Fonts::getCurrentFont().getTypefacePtr());
}

void PlugDataLook::fillResizableWindowBackground(Graphics& g, int w, int h, BorderSize<int> const& border, ResizableWindow& window)
{
    if (dynamic_cast<FileChooserDialogBox*>(&window)) {
        g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));
    }
}

void PlugDataLook::drawTextButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto backgroundColour = findColour(shouldDrawButtonAsDown || button.getToggleState() ? PlugDataColour::dataColourId : PlugDataColour::canvasTextColourId);
    if (shouldDrawButtonAsHighlighted)
        backgroundColour = backgroundColour.brighter(0.5f);
    auto cornerSize = Corners::defaultCornerRadius;
    g.setColour(backgroundColour);
    fillSmoothedRectangle(g, button.getLocalBounds().toFloat(), cornerSize);
    jassertfalse; // I think we don't use this anymore?
}

void PlugDataLook::drawToolbarButtonBackground(Graphics& g, Button& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    bool active = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown || button.getToggleState();

    auto cornerSize = Corners::defaultCornerRadius;
    auto flatOnLeft = button.isConnectedOnLeft();
    auto flatOnRight = button.isConnectedOnRight();
    auto flatOnTop = button.isConnectedOnTop();
    auto flatOnBottom = button.isConnectedOnBottom();

    if (flatOnLeft || flatOnRight || flatOnTop || flatOnBottom) {

        auto backgroundColour = findColour(active ? PlugDataColour::toolbarHoverColourId : PlugDataColour::toolbarBackgroundColourId).contrasting((shouldDrawButtonAsHighlighted && !button.getToggleState()) ? 0.0f : 0.05f);

        auto bounds = button.getLocalBounds().toFloat();
        bounds = bounds.reduced(0.0f, bounds.proportionOfHeight(0.17f));

        g.setColour(backgroundColour);
        fillSmoothedRectangle(g, bounds, Corners::defaultCornerRadius,
            !(flatOnLeft || flatOnTop),
            !(flatOnRight || flatOnTop),
            !(flatOnLeft || flatOnBottom),
            !(flatOnRight || flatOnBottom));
    } else {
        auto backgroundColour = active ? findColour(PlugDataColour::toolbarHoverColourId) : Colours::transparentBlack;
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f, 4.0f);

        g.setColour(backgroundColour);
        fillSmoothedRectangle(g, bounds, cornerSize);
        // g.fillRoundedRectangle(bounds, cornerSize);
    }
}

void PlugDataLook::drawCallOutBoxBackground(CallOutBox& box, Graphics& g, Path const& path, Image& cachedImage)
{
    if (cachedImage.isNull()) {
        cachedImage = { Image::ARGB, box.getWidth(), box.getHeight(), true };
        Graphics g2(cachedImage);

        StackShadow::renderDropShadow(g2, path, Colour(0, 0, 0).withAlpha(0.3f), 8, { 0, 1 });
    }

    g.setColour(Colours::black);
    g.drawImageAt(cachedImage, 0, 0);

    g.setColour(box.findColour(PlugDataColour::popupMenuBackgroundColourId));
    g.fillPath(path);

    g.setColour(box.findColour(PlugDataColour::outlineColourId));
    g.strokePath(path, PathStrokeType(1.0f));
}

int PlugDataLook::getCallOutBoxBorderSize(CallOutBox const& c)
{
    return 20;
}

void PlugDataLook::drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
    Font font(getTextButtonFont(button, button.getHeight()));
    g.setFont(font);
    auto colour = button.findColour(button.getToggleState() ? TextButton::textColourOnId
                                                            : TextButton::textColourOffId)
                      .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (!button.getClickingTogglesState() && button.isMouseOver()) {
        colour = button.findColour(TextButton::textColourOnId);
    }
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

Font PlugDataLook::getTextButtonFont(TextButton& but, int buttonHeight)
{
    return { buttonHeight / 1.7f };
}

void PlugDataLook::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider::SliderStyle const style, Slider& slider)
{
    if (slider.getProperties()["Style"] == "SliderObject") {
        drawGUIObjectSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, slider);
    } else {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

Button* PlugDataLook::createDocumentWindowButton(int buttonType)
{
    if (buttonType == -1)
        return new PlugData_DocumentWindowButton(DocumentWindow::closeButton);
    else if (SettingsFile::getInstance()->getProperty<bool>("macos_buttons"))
        return new PlugData_DocumentWindowButton_macOS(buttonType);
    else
        return new PlugData_DocumentWindowButton(buttonType);

    jassertfalse;
    return nullptr;
}

void PlugDataLook::positionDocumentWindowButtons(DocumentWindow& window,
    int titleBarX, int titleBarY, int titleBarW, int titleBarH,
    Button* minimiseButton,
    Button* maximiseButton,
    Button* closeButton,
    bool positionTitleBarButtonsOnLeft)
{
    if (window.isUsingNativeTitleBar())
        return;

    auto areButtonsLeft = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");

    // heuristic to offset the buttons when positioned left, as we are drawing larger to provide a shadow
    // we check if the system is drawing with a dropshadow- hence semi transparent will be true
#if JUCE_LINUX || JUCE_BSD
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

// ==================== LookAndFeel TabBarButton ====================

Rectangle<int> PlugDataLook::getTabButtonExtraComponentBounds(TabBarButton const& button, Rectangle<int>& textArea, Component& comp)
{
    Rectangle<int> extraComp;

    auto area = textArea.reduced(4);

    auto orientation = button.getTabbedButtonBar().getOrientation();

    if (button.getExtraComponentPlacement() == TabBarButton::beforeText) {
        switch (orientation) {
        case TabbedButtonBar::TabsAtBottom:
        case TabbedButtonBar::TabsAtTop:
            extraComp = area.removeFromLeft(comp.getWidth());
            break;
        case TabbedButtonBar::TabsAtLeft:
            extraComp = area.removeFromBottom(comp.getHeight());
            break;
        case TabbedButtonBar::TabsAtRight:
            extraComp = area.removeFromTop(comp.getHeight());
            break;
        default:
            jassertfalse;
            break;
        }
    } else {
        switch (orientation) {
        case TabbedButtonBar::TabsAtBottom:
        case TabbedButtonBar::TabsAtTop:
            extraComp = area.removeFromRight(comp.getWidth());
            break;
        case TabbedButtonBar::TabsAtLeft:
            extraComp = area.removeFromTop(comp.getHeight());
            break;
        case TabbedButtonBar::TabsAtRight:
            extraComp = area.removeFromBottom(comp.getHeight());
            break;
        default:
            jassertfalse;
            break;
        }
    }

    return extraComp;
}

int PlugDataLook::getTabButtonBestWidth(TabBarButton& button, int tabDepth)
{
    auto& buttonBar = button.getTabbedButtonBar();
    return std::max((buttonBar.getWidth() / buttonBar.getNumTabs()) + 1, 120);
}

int PlugDataLook::getTabButtonOverlap(int tabDepth)
{
    return 0;
}

void PlugDataLook::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    drawTabButton(button, g, isMouseOver, isMouseDown, false);
}

void PlugDataLook::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown, bool isForceDrawn)
{
    auto dragged = button.getProperties()["dragged"];
    if (!isForceDrawn && !dragged.isVoid() && static_cast<bool>(dragged))
        return;

    bool isActive = button.getToggleState();

    if (isActive) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId));
    } else if (isMouseOver) {
        g.setColour(findColour(PlugDataColour::activeTabBackgroundColourId).interpolatedWith(findColour(PlugDataColour::tabBackgroundColourId), 0.4f));
    } else {
        g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
    }

    fillSmoothedRectangle(g, button.getLocalBounds().reduced(4).toFloat(), Corners::defaultCornerRadius);
    drawTabButtonText(button, g, isMouseOver, isMouseDown);
}

void PlugDataLook::drawTabButtonText(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    auto area = button.getLocalBounds().reduced(4, 1).toFloat();

    Font font(getTabButtonFont(button, area.getHeight()));
    font.setUnderline(button.hasKeyboardFocus(false));

    Colour col;

    if (button.isFrontTab() && (button.isColourSpecified(TabbedButtonBar::frontTextColourId) || isColourSpecified(TabbedButtonBar::frontTextColourId)))
        col = findColour(TabbedButtonBar::frontTextColourId);
    else if (button.isColourSpecified(TabbedButtonBar::tabTextColourId)
        || isColourSpecified(TabbedButtonBar::tabTextColourId))
        col = findColour(TabbedButtonBar::tabTextColourId);
    else
        col = button.getTabBackgroundColour().contrasting();

    // Use a gradient to make it fade out when it gets near to the close button
    auto fadeX = (isMouseOver || button.getToggleState()) ? area.getRight() - 25 : area.getRight() - 8;
    g.setGradientFill(ColourGradient(col, fadeX - 18, area.getY(), Colours::transparentBlack, fadeX, area.getY(), false));

    g.setFont(font);

    g.drawText(button.getButtonText().trim(), area.reduced(4, 0), Justification::centred, false);
}

Button* PlugDataLook::createTabBarExtrasButton()
{

    class TabBarExtrasButton : public TextButton {
    public:
        TabBarExtrasButton()
        {
            setButtonText(Icons::ThinDown);
            setTriggeredOnMouseDown(true);
        }

        void moved()
        {
            static bool insideMove = false;
            if (insideMove)
                return;

            if (auto* parent = getParentComponent()) {
                insideMove = true;
                auto position = parent->getLocalBounds().getTopRight() - Point<int>(28, 0);
                setTopLeftPosition(position);
                insideMove = false;
            }
        }

        void resized()
        {
            // Try to force the size to 28x28, while also not allowing any recursion
            // JUCE gives us very little control over the position/size otherwise...
            static bool insideResize = false;
            if (insideResize)
                return;
            insideResize = true;
            setSize(28, 28);
            insideResize = false;
        }

        void paint(Graphics& g)
        {
            bool hiddenTabSelected = false;
            if (auto* tabbar = findParentComponentOfClass<TabbedButtonBar>()) {

                auto currentTabIndex = tabbar->getCurrentTabIndex();
                if (currentTabIndex >= 0) {
                    auto* currentTab = tabbar->getTabButton(currentTabIndex);
                    hiddenTabSelected = !currentTab->isVisible();
                }
            }

            if (isMouseOverOrDragging() || hiddenTabSelected) {
                g.setColour(findColour(PlugDataColour::toolbarHoverColourId));
                fillSmoothedRectangle(g, getLocalBounds().reduced(3).toFloat(), Corners::defaultCornerRadius);
            } else {
                g.setColour(findColour(PlugDataColour::tabBackgroundColourId));
            }

            g.setFont(Fonts::getIconFont().withHeight(15));
            g.setColour(findColour(PlugDataColour::tabTextColourId));

            g.drawText(getButtonText(), getLocalBounds().reduced(3), Justification::centred);

            return;
        }

        void mouseDown(MouseEvent const& e)
        {
            class HiddenTabMenuItem : public PopupMenu::CustomComponent {

                String tabTitle;

            public:
                int index;
                TabbedButtonBar& tabbar;

                HiddenTabMenuItem(String text, int idx, TabbedButtonBar& buttonBar)
                    : tabTitle(text)
                    , index(idx)
                    , tabbar(buttonBar)
                {
                    closeTabButton.setButtonText(Icons::Clear);
                    closeTabButton.getProperties().set("Style", "Icon");
                    closeTabButton.getProperties().set("FontScale", 0.44f);
                    closeTabButton.setColour(TextButton::buttonColourId, Colour());
                    closeTabButton.setColour(TextButton::buttonOnColourId, Colour());
                    closeTabButton.setColour(ComboBox::outlineColourId, Colour());
                    closeTabButton.setConnectedEdges(12);
                    closeTabButton.setSize(26, 26);
                    closeTabButton.addMouseListener(this, false);
                    closeTabButton.onClick = [this]() mutable {
                        dynamic_cast<TabBarButtonComponent*>(tabbar.getTabButton(index))->closeTab();
                    };

                    addChildComponent(closeTabButton);
                }

                void resized()
                {
                    closeTabButton.setTopLeftPosition(getWidth() - 26, -2);
                }

                void getIdealSize(int& idealWidth, int& idealHeight)
                {
                    idealWidth = 150;
                    idealHeight = 24;
                }

                void mouseDown(MouseEvent const& e)
                {
                    if (e.originalComponent == &closeTabButton)
                        return;

                    tabbar.setCurrentTabIndex(index);
                    triggerMenuItem();
                }

                void mouseEnter(MouseEvent const& e)
                {
                    closeTabButton.setVisible(true);
                }

                void mouseExit(MouseEvent const& e)
                {
                    closeTabButton.setVisible(false);
                }

                void paint(Graphics& g)
                {
                    bool isActive = tabbar.getCurrentTabIndex() == index;

                    if (isActive) {
                        g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
                    } else if (isItemHighlighted()) {
                        g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId).interpolatedWith(findColour(PlugDataColour::popupMenuBackgroundColourId), 0.4f));
                    } else {
                        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
                    }

                    fillSmoothedRectangle(g, getLocalBounds().reduced(1).toFloat(), Corners::defaultCornerRadius);

                    auto area = getLocalBounds().reduced(4, 1).toFloat();

                    Font font = Font(14);

                    g.setColour(findColour(TabbedButtonBar::tabTextColourId));
                    g.setFont(font);
                    g.drawText(tabTitle.trim(), area.reduced(4, 0), Justification::centred, false);
                }

                TextButton closeTabButton;
            };

            if (auto* parent = findParentComponentOfClass<TabbedButtonBar>()) {
                PopupMenu m;

                auto tabNames = parent->getTabNames();
                for (int i = 0; i < parent->getNumTabs(); ++i) {
                    auto* tab = parent->getTabButton(i);

                    if (!tab->isVisible()) {
                        m.addCustomItem(i + 1, std::make_unique<HiddenTabMenuItem>(tabNames[i], i, *parent), nullptr, tabNames[i]);
                    }
                    /*
                     m.addItem (PopupMenu::Item (tabNames[i])
                                  .setTicked (i == parent->getCurrentTabIndex())
                                  .setAction ([this, i, parent] { parent->setCurrentTabIndex (i); })); */
                }

                m.showMenuAsync(PopupMenu::Options()
                                    .withDeletionCheck(*this)
                                    .withTargetComponent(this));
            }
        }
    };

    return new TabBarExtrasButton();
}

Font PlugDataLook::getTabButtonFont(TabBarButton&, float height)
{
    return Fonts::getCurrentFont().withHeight(13.5f);
}

void PlugDataLook::drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height,
    bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, [[maybe_unused]] bool isMouseDown)
{
    Rectangle<int> thumbBounds;

    if (isScrollbarVertical)
        thumbBounds = { x, thumbStartPosition, width, thumbSize };
    else
        thumbBounds = { thumbStartPosition, y, thumbSize, height };

    auto c = scrollbar.findColour(ScrollBar::ColourIds::thumbColourId);
    g.setColour(isMouseOver ? c.brighter(0.25f) : c);

    auto thumbRadius = isScrollbarVertical ? (thumbBounds.getWidth() - 2.0f) / 2.0f : (thumbBounds.getHeight() - 2.0f) / 2.0f;
    g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), thumbRadius);
}

void PlugDataLook::getIdealPopupMenuItemSize(String const& text, bool const isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight)
{
    if (isSeparator) {
        idealWidth = 50;
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
    } else {
        auto font = getPopupMenuFont();

        if (standardMenuItemHeight > 0 && font.getHeight() > (float)standardMenuItemHeight / 1.3f)
            font.setHeight((float)standardMenuItemHeight / 1.3f);

        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : roundToInt(font.getHeight() * 1.3f);
        idealWidth = font.getStringWidth(text) + idealHeight;

#if JUCE_LINUX || JUCE_WINDOWS
        // Dumb check to see if there is a keyboard shortcut after the text.
        // On Linux and Windows, it seems to reserve way to much space for those.
        if (text.contains("  ")) {
            idealWidth -= 36;
        }
#endif
    }
}

void PlugDataLook::drawPopupMenuBackgroundWithOptions(Graphics& g, int width, int height, PopupMenu::Options const& options)
{
    auto background = findColour(PlugDataColour::popupMenuBackgroundColourId);

    // TODO: some popup menus are added to a component and some to desktop,
    // which makes it really hard to decide whether they can be transparent or not!
    // We can check it in this function by checking options.getParentComponent, but unfortunately not everywhere
    if (Desktop::canUseSemiTransparentWindows()) {
        Path shadowPath;
        shadowPath.addRoundedRectangle(Rectangle<float>(0.0f, 0.0f, width, height).reduced(10.0f), Corners::defaultCornerRadius);
        StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 11, { 0, 1 });

        g.setColour(background);

        auto bounds = Rectangle<float>(5, 6, width - 10, height - 12);
        fillSmoothedRectangle(g, bounds, Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        drawSmoothedRectangle(g, PathStrokeType(1.0f), bounds, Corners::largeCornerRadius);
    } else {
        auto bounds = Rectangle<float>(0, 0, width, height);

        g.setColour(background);
        g.fillRect(bounds);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRect(bounds, 1.0f);
    }
}

Path PlugDataLook::getTickShape(float height)
{
    Path path;
    path.startNewSubPath(0.4f * height, 0.6f * height);
    path.lineTo(0.475f * height, 0.7f * height);
    path.lineTo(0.65f * height, 0.475f * height);

    Path strokedPath;
    PathStrokeType(height / 15.0f, PathStrokeType::curved, PathStrokeType::rounded).createStrokedPath(strokedPath, path, AffineTransform(), 5.0f);

    return strokedPath;
}

void PlugDataLook::drawPopupMenuItem(Graphics& g, Rectangle<int> const& area,
    bool const isSeparator, bool const isActive,
    bool const isHighlighted, bool const isTicked,
    bool const hasSubMenu, String const& text,
    String const& shortcutKeyText,
    Drawable const* icon, Colour const* const textColourToUse)
{
    int margin = Desktop::canUseSemiTransparentWindows() ? 7 : 2;

    if (isSeparator) {
        auto r = area.reduced(margin + 8, 0);
        r.removeFromTop(roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

        g.setColour(findColour(PlugDataColour::outlineColourId).withAlpha(0.7f));
        g.fillRect(r.removeFromTop(1));
    } else {
        auto r = area.reduced(margin, 0);

        auto colour = findColour(PopupMenu::textColourId).withMultipliedAlpha(isActive ? 1.0f : 0.5f);
        if (isHighlighted && isActive) {
            g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            fillSmoothedRectangle(g, r.toFloat().reduced(4, 0), Corners::defaultCornerRadius);
            // g.fillRoundedRectangle(r.toFloat().reduced(4, 0), Corners::defaultCornerRadius);
            colour = findColour(PlugDataColour::popupMenuActiveTextColourId);
        }

        g.setColour(colour);

        r.reduce(jmin(5, area.getWidth() / 20), 0);

        auto font = getPopupMenuFont();

        auto maxFontHeight = (float)r.getHeight() / 1.3f;

        if (font.getHeight() > maxFontHeight)
            font.setHeight(maxFontHeight);

        g.setFont(font);

        if (icon != nullptr) {
            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();

            icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));
        } else if (isTicked) {
            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();
            auto tick = getTickShape(1.0f);
            g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
        } else {
            r.removeFromLeft(8);
        }

        if (hasSubMenu) {
            auto arrowH = 0.6f * getPopupMenuFont().getAscent();

            auto x = static_cast<float>(r.removeFromRight((int)arrowH + 3).getX());
            auto halfH = static_cast<float>(r.getCentreY());

            Path path;
            path.startNewSubPath(x, halfH - arrowH * 0.5f);
            path.lineTo(x + arrowH * 0.5f, halfH);
            path.lineTo(x, halfH + arrowH * 0.5f);

            g.strokePath(path, PathStrokeType(1.5f));
        }

        r.removeFromRight(3);
        Fonts::drawFittedText(g, text, r, colour);
        
        auto shortcutBounds = r.translated(-4, 0);
        
#if JUCE_MAC
        for(int i = shortcutKeyText.length() - 1; i >= 0; i--)
        {
            auto font = Fonts::getSemiBoldFont().withHeight(10.5f);
            auto text = shortcutKeyText.substring(i, i+1);
            auto width = std::max(font.getStringWidth(text), 15);
            auto b = shortcutBounds.removeFromRight(width).reduced(1, 5);
            
            g.setColour(findColour(PlugDataColour::popupMenuTextColourId).withAlpha(isActive ? 0.8f : 0.5f));
            g.fillRoundedRectangle(b.toFloat(), 3.0f);
            
            g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
            
            g.setFont(Fonts::getSemiBoldFont().withHeight(11));
            g.drawText(text, b, Justification::centred);
        }
#else
        auto keys = StringArray::fromTokens(shortcutKeyText, "+", "");
        for(int i = keys.size() - 1; i >= 0; i--)
        {
            auto font = Fonts::getSemiBoldFont().withHeight(10.5f);
            auto width = std::max(font.getStringWidth(keys[i].trim()) + 8, 15);
            auto b = shortcutBounds.removeFromRight(width).reduced(1, 5);
            
            g.setColour(findColour(PlugDataColour::popupMenuTextColourId).withAlpha(isActive ? 0.8f : 0.5f));
            g.fillRoundedRectangle(b.toFloat(), 3.0f);
            
            g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
            
            g.setFont(font);
            g.drawText(keys[i], b, Justification::centred);
        }
#endif
    }
}

int PlugDataLook::getMenuWindowFlags()
{
    return ComponentPeer::windowIsTemporary;
}

int PlugDataLook::getPopupMenuBorderSize()
{
    if (Desktop::canUseSemiTransparentWindows()) {
        return 12;
    } else {
        return 6;
    }
}

void PlugDataLook::drawTreeviewPlusMinusBox(Graphics& g, Rectangle<float> const& area, Colour, bool isOpen, bool isMouseOver)
{
    Path p;
    p.startNewSubPath(0.0f, 0.0f);
    p.lineTo(0.5f, 0.5f);
    p.lineTo(isOpen ? 1.0f : 0.0f, isOpen ? 0.0f : 1.0f);

    auto size = std::min(area.getWidth(), area.getHeight()) * 0.5f;
    g.setColour(findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
    g.strokePath(p, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(area.withSizeKeepingCentre(size, size), true));
}

void PlugDataLook::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& object)
{
    bool inspectorElement = object.getProperties()["Style"] == "Inspector";

    Rectangle<int> boxBounds(0, 0, width, height);

    if (!inspectorElement) {

        g.setColour(object.findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), Corners::defaultCornerRadius);

        g.setColour(object.findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), Corners::defaultCornerRadius, 1.0f);
    }

    Rectangle<int> arrowZone(width - 22, 9, 14, height - 18);
    Path path;
    path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
    path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 2.0f);
    path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);
    g.setColour(object.findColour(ComboBox::arrowColourId).withAlpha((object.isEnabled() ? 0.9f : 0.2f)));

    g.strokePath(path, PathStrokeType(2.0f));
}

PopupMenu::Options PlugDataLook::getOptionsForComboBoxPopupMenu(ComboBox& box, Label& label)
{
    return PopupMenu::Options().withTargetComponent(&box).withItemThatMustBeVisible(box.getSelectedId()).withInitiallySelectedItem(box.getSelectedId()).withMinimumWidth(box.getWidth()).withMaximumNumColumns(1).withStandardItemHeight(22);
}

void PlugDataLook::drawGUIObjectSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, Slider& slider)
{
    auto sliderBounds = slider.getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(findColour(Slider::backgroundColourId));
    g.fillRect(sliderBounds);

    constexpr auto thumbSize = 4.0f;
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

void PlugDataLook::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& textEditor)
{
    if (textEditor.getProperties()["NoBackground"].isVoid()) {
        g.setColour(textEditor.findColour(TextEditor::backgroundColourId));
        g.fillRoundedRectangle(2, 3, width - 4, height - 6, Corners::defaultCornerRadius);
    }
}

void PlugDataLook::drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& textEditor)
{
    if (textEditor.getProperties()["NoOutline"].isVoid()) {
        if (textEditor.isEnabled()) {
            if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly()) {
                g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                g.drawRoundedRectangle(2, 3, width - 4, height - 6, Corners::defaultCornerRadius, 2.0f);
            } else {
                g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                g.drawRoundedRectangle(2, 3, width - 4, height - 6, Corners::defaultCornerRadius, 2.0f);
            }
        }
    }
}

void PlugDataLook::drawCornerResizer(Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    Path triangle;
    triangle.addTriangle(Point<float>(0, h), Point<float>(w, h), Point<float>(w, 0));

    g.saveState();
    g.setColour(findColour(PlugDataColour::resizeableCornerColourId).withAlpha(isMouseOver ? 1.0f : 0.6f));
    g.fillPath(triangle);
    g.restoreState();
}

void PlugDataLook::drawTooltip(Graphics& g, String const& text, int width, int height)
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

    auto lines = StringArray::fromLines(convertURLtoUTF8(text));

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

Font PlugDataLook::getComboBoxFont(ComboBox& box)
{
    return Fonts::getDefaultFont().withHeight(std::clamp(box.getHeight() * 0.85f, 13.5f, 15.0f));
}

void PlugDataLook::drawLasso(Graphics& g, Component& lassoComp)
{
    float outlineThickness = 0.75f;

    // Apply inverted scaling of the canvas to the outline of the lasso, so the lasso outline doesn't grow larger as you zoom in
    if (auto* parent = lassoComp.getParentComponent()) {
        auto transform = parent->getTransform();
        auto transformScale = std::sqrt(std::abs(transform.getDeterminant()));
        outlineThickness = 0.75f / std::max(transformScale, std::numeric_limits<float>::epsilon());
    }

    Path p;
    p.addRectangle(lassoComp.getLocalBounds().reduced(1));

    g.setColour(lassoComp.findColour(0x1000440 /*lassoFillColourId*/));
    g.fillPath(p);

    g.setColour(lassoComp.findColour(0x1000441 /*lassoOutlineColourId*/));
    g.strokePath(p, PathStrokeType(outlineThickness));
}

void PlugDataLook::drawLabel(Graphics& g, Label& label)
{
    g.fillAll(label.findColour(Label::backgroundColourId));

    if (!label.isBeingEdited()) {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        Font const font = label.getFont();

        auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

        g.setFont(font);
        g.setColour(label.findColour(Label::textColourId));

        g.drawFittedText(label.getText(), textArea, label.getJustificationType(), 1, 1.0f);

        g.setColour(label.findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
    } else if (label.isEnabled()) {
        g.setColour(label.findColour(Label::outlineColourId));
    }

    g.drawRect(label.getLocalBounds());
}

Path PlugDataLook::getSquircle(Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft, bool const curveTopRight, bool const curveBottomLeft, bool const curveBottomRight)
{
    Path path;

    float x = bounds.getX();
    float y = bounds.getY();
    float width = bounds.getWidth();
    float height = bounds.getHeight();

    float radius = cornerRadius;
    if (radius > width * 0.5f)
        radius = width * 0.5f;
    if (radius > height * 0.5f)
        radius = height * 0.5f;

    float controlOffset = radius * 0.45f;

    path.startNewSubPath(x + radius, y);

    if (curveTopRight) {
        path.lineTo(x + width - radius, y);
        path.cubicTo(x + width - radius + controlOffset, y, x + width, y + radius - controlOffset, x + width, y + radius);
    } else {
        path.lineTo(x + width, y);
    }

    if (curveBottomRight) {
        path.lineTo(x + width, y + height - radius);
        path.cubicTo(x + width, y + height - radius + controlOffset, x + width - radius + controlOffset, y + height, x + width - radius, y + height);
    } else {
        path.lineTo(x + width, y + height);
    }

    if (curveBottomLeft) {
        path.lineTo(x + radius, y + height);
        path.cubicTo(x + radius - controlOffset, y + height, x, y + height - radius + controlOffset, x, y + height - radius);
    } else {
        path.lineTo(x, y + height);
    }

    if (curveTopLeft) {
        path.lineTo(x, y + radius);
        path.cubicTo(x, y + radius - controlOffset, x + radius - controlOffset, y, x + radius, y);
    } else {
        path.lineTo(x, y);
    }

    path.closeSubPath();

    return path;
}

void PlugDataLook::fillSmoothedRectangle(Graphics& g, Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft, bool const curveTopRight, bool const curveBottomLeft, bool const curveBottomRight)
{
    g.fillPath(getSquircle(bounds, cornerRadius, curveTopLeft, curveTopRight, curveBottomLeft, curveBottomRight));
}

void PlugDataLook::drawSmoothedRectangle(Graphics& g, PathStrokeType strokeType, Rectangle<float> const& bounds, float cornerRadius, bool const curveTopLeft, bool const curveTopRight, bool const curveBottomLeft, bool const curveBottomRight)
{
    g.strokePath(getSquircle(bounds, cornerRadius, curveTopLeft, curveTopRight, curveBottomLeft, curveBottomRight), strokeType);
}

void PlugDataLook::drawPropertyComponentLabel(Graphics& g, int width, int height, PropertyComponent& component)
{
    auto indent = jmin(10, component.getWidth() / 10);

    auto colour = component.findColour(PropertyComponent::labelTextColourId)
                      .withMultipliedAlpha(component.isEnabled() ? 1.0f : 0.6f);

    auto textW = jmin(300, component.getWidth() / 2);
    auto r = Rectangle<float>(textW, 0, component.getWidth() - textW, component.getHeight() - 1);

    Fonts::drawFittedText(g, component.getName(), indent, r.getY(), r.getX(), r.getHeight(), colour, 1, 1.0f, (float)jmin(height, 24) * 0.65f, Justification::centredLeft);
}

void PlugDataLook::drawPropertyPanelSectionHeader(Graphics& g, String const& name, bool isOpen, int width, int height)
{
    auto buttonSize = (float)height * 0.75f;
    auto buttonIndent = ((float)height - buttonSize) * 0.5f;

    drawTreeviewPlusMinusBox(g, { buttonIndent, buttonIndent, buttonSize, buttonSize }, findColour(ResizableWindow::backgroundColourId), isOpen, false);

    auto textX = static_cast<int>((buttonIndent * 2.0f + buttonSize + 2.0f));

    Fonts::drawStyledText(g, name, textX, 0, std::max(width - textX - 4, 0), height, findColour(PropertyComponent::labelTextColourId), Bold, height * 0.6f);
}

Rectangle<int> PlugDataLook::getTooltipBounds(String const& tipText, Point<int> screenPos, Rectangle<int> parentArea)
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

int PlugDataLook::getTreeViewIndentSize(TreeView&)
{
    return 36;
}

void PlugDataLook::setColours(std::map<PlugDataColour, Colour> colours)
{
    for (auto colourId = 0; colourId < PlugDataColour::numberOfColours; colourId++) {
        setColour(colourId, colours.at(static_cast<PlugDataColour>(colourId)));
    }

    setColour(PopupMenu::highlightedBackgroundColourId,
        colours.at(PlugDataColour::panelActiveBackgroundColourId));
    setColour(TextButton::textColourOnId,
        colours.at(PlugDataColour::toolbarTextColourId));
    setColour(Slider::thumbColourId,
        colours.at(PlugDataColour::sliderThumbColourId));
    setColour(ScrollBar::thumbColourId,
        colours.at(PlugDataColour::scrollbarThumbColourId));
    setColour(DirectoryContentsDisplayComponent::highlightColourId,
        colours.at(PlugDataColour::panelActiveBackgroundColourId));
    setColour(CaretComponent::caretColourId,
        colours.at(PlugDataColour::caretColourId));
    setColour(TextEditor::focusedOutlineColourId,
        colours.at(PlugDataColour::caretColourId));

    setColour(TextButton::buttonColourId,
        colours.at(PlugDataColour::toolbarBackgroundColourId));
    setColour(TextButton::buttonOnColourId,
        colours.at(PlugDataColour::toolbarBackgroundColourId));
    setColour(ComboBox::backgroundColourId,
        colours.at(PlugDataColour::toolbarBackgroundColourId));
    setColour(ListBox::backgroundColourId,
        colours.at(PlugDataColour::toolbarBackgroundColourId));

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

    setColour(AlertWindow::textColourId, colours.at(PlugDataColour::panelTextColourId));
    
    setColour(Slider::textBoxOutlineColourId,
        Colours::transparentBlack);
    setColour(TreeView::backgroundColourId,
        Colours::transparentBlack);
}

void PlugDataLook::drawAlertBox (Graphics& g, AlertWindow& alert,
                   const Rectangle<int>& textArea, TextLayout& textLayout)
{
    auto cornerSize = Corners::largeCornerRadius;
    
    g.setColour (alert.findColour (PlugDataColour::outlineColourId));
    g.drawRoundedRectangle (alert.getLocalBounds().toFloat(), cornerSize, 1.0f);
    
    auto bounds = alert.getLocalBounds().reduced (1);
    g.reduceClipRegion (bounds);
    
    g.setColour (alert.findColour (PlugDataColour::dialogBackgroundColourId));
    g.fillRoundedRectangle (bounds.toFloat(), cornerSize);
    
    auto iconSpaceUsed = 0;
    
    auto iconWidth = 80;
    auto iconSize = jmin (iconWidth + 50, bounds.getHeight() + 20);
    
    if (alert.containsAnyExtraComponents() || alert.getNumButtons() > 2)
        iconSize = jmin (iconSize, textArea.getHeight() + 50);
    
    Rectangle<int> iconRect (iconSize / -10, iconSize / -10,
                             iconSize, iconSize);
    
    if (alert.getAlertType() != MessageBoxIconType::NoIcon)
    {
        Path icon;
        char character;
        uint32 colour;
        
        if (alert.getAlertType() == MessageBoxIconType::WarningIcon)
        {
            character = '!';
            
            icon.addTriangle ((float) iconRect.getX() + (float) iconRect.getWidth() * 0.5f, (float) iconRect.getY(),
                              static_cast<float> (iconRect.getRight()), static_cast<float> (iconRect.getBottom()),
                              static_cast<float> (iconRect.getX()), static_cast<float> (iconRect.getBottom()));
            
            icon = icon.createPathWithRoundedCorners (5.0f);
            colour = 0x66ff2a00;
        }
        else
        {
            colour = Colour (0xff00b0b9).withAlpha (0.4f).getARGB();
            character = alert.getAlertType() == MessageBoxIconType::InfoIcon ? 'i' : '?';
            
            icon.addEllipse (iconRect.toFloat());
        }
        
        GlyphArrangement ga;
        ga.addFittedText ({ (float) iconRect.getHeight() * 0.9f, Font::bold },
                          String::charToString ((juce_wchar) (uint8) character),
                          static_cast<float> (iconRect.getX()), static_cast<float> (iconRect.getY()),
                          static_cast<float> (iconRect.getWidth()), static_cast<float> (iconRect.getHeight()),
                          Justification::centred, false);
        ga.createPath (icon);
        
        icon.setUsingNonZeroWinding (false);
        g.setColour (Colour (colour));
        g.fillPath (icon);
        
        iconSpaceUsed = iconWidth;
    }
    
    g.setColour (alert.findColour (AlertWindow::textColourId));
    
    Rectangle<int> alertBounds (bounds.getX() + iconSpaceUsed, 30,
                                bounds.getWidth(), bounds.getHeight() - getAlertWindowButtonHeight() - 20);
    
    textLayout.draw (g, alertBounds.toFloat());
}

void PlugDataLook::setDefaultFont(String fontName)
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
const String PlugDataLook::defaultThemesXml = "<ColourThemes> "
"   <Theme theme=\"max\" toolbar_background=\"ff333333\" toolbar_text=\"ffe4e4e4\"\n"
"           toolbar_active=\"ff72aedf\" toolbar_hover=\"ff424242\" tabbar_background=\"ff333333\"\n"
"           tab_text=\"ffe4e4e4\" selected_tab_background=\"ff494949\" selected_tab_text=\"ff72aedf\"\n"
"           canvas_background=\"ffe5e5e5\" canvas_text=\"ffeeeeee\" canvas_dots=\"ff7f7f7f\"\n"
"           default_object_background=\"ff333333\" object_outline_colour=\"ff696969\"\n"
"           selected_object_outline_colour=\"ff72aedf\" gui_internal_outline_colour=\"ff696969\"\n"
"           toolbar_outline_colour=\"ff393939\" outline_colour=\"ff393939\" data_colour=\"ff72aedf\"\n"
"           connection_colour=\"ffb3b3b3\" signal_colour=\"ffe1ef00\" dialog_background=\"ff3b3b3b\"\n"
"           sidebar_colour=\"ff3e3e3e\" sidebar_text=\"ffe4e4e4\" sidebar_background_active=\"ff4f4f4f\"\n"
"           sidebar_active_text=\"ffe4e4e4\" levelmeter_active=\"ff72aedf\" levelmeter_background=\"ff494949\"\n"
"           levelmeter_thumb=\"ffe4e4e4\" panel_background=\"ff4f4f4f\" panel_foreground=\"ff3f3f3f\"\n"
"           panel_text=\"ffe4e4e4\" panel_background_active=\"ff525252\" panel_active_text=\"ffe4e4e4\"\n"
"           popup_background=\"ff333333\" popup_background_active=\"ff72aedf\"\n"
"           popup_text=\"ffe4e4e4\" popup_active_text=\"ffe4e4e4\" slider_thumb=\"ff72aedf\"\n"
"           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff72aedf\" grid_colour=\"ff72aedf\"\n"
"           caret_colour=\"ff72aedf\" iolet_area_colour=\"ff808080\" iolet_outline_colour=\"ff696969\"\n"
"           text_object_background=\"ff333333\" comment_text_colour=\"ff111111\"\n"
"           dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
"           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"1\"/>\n"
"    <Theme theme=\"classic\" toolbar_background=\"ffffffff\" toolbar_text=\"ff000000\"\n"
"           toolbar_active=\"ff787878\" toolbar_hover=\"ffaaaaaa\" tabbar_background=\"ffffffff\"\n"
"           tab_text=\"ff000000\" selected_tab_background=\"ffa0a0a0\" selected_tab_text=\"ff000000\"\n"
"           canvas_background=\"ffffffff\" canvas_text=\"ff000000\" canvas_dots=\"ffffffff\"\n"
"           default_object_background=\"ffffffff\" text_object_background=\"ffffffff\"\n"
"           object_outline_colour=\"ff000000\" selected_object_outline_colour=\"ff000000\"\n"
"           gui_internal_outline_colour=\"ff000000\" toolbar_outline_colour=\"ff000000\"\n"
"           outline_colour=\"ff000000\" iolet_area_colour=\"ffffffff\" iolet_outline_colour=\"ff000000\"\n"
"           data_colour=\"ff000000\" connection_colour=\"ff000000\" signal_colour=\"ff000000\"\n"
"           dialog_background=\"ffffffff\" sidebar_colour=\"ffefefef\" sidebar_text=\"ff000000\"\n"
"           sidebar_background_active=\"ff000000\" sidebar_active_text=\"ffffffff\"\n"
"           levelmeter_active=\"ff000000\" levelmeter_background=\"ffa0a0a0\"\n"
"           levelmeter_thumb=\"ff000000\" panel_background=\"ffffffff\" panel_foreground=\"ffffffff\"\n"
"           panel_text=\"ff000000\" panel_background_active=\"ff000000\" panel_active_text=\"ffffffff\"\n"
"           popup_background=\"ffffffff\" popup_background_active=\"ff000000\"\n"
"           popup_text=\"ff000000\" popup_active_text=\"ffffffff\" slider_thumb=\"ff000000\"\n"
"           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff000000\" grid_colour=\"ff000000\"\n"
"           caret_colour=\"ff000000\" comment_text_colour=\"ff000000\"\n"
"           dashed_signal_connections=\"0\" straight_connections=\"1\" thin_connections=\"1\"\n"
"           square_iolets=\"1\" square_object_corners=\"1\"/>\n"
"    <Theme theme=\"classic_dark\" toolbar_background=\"ff000000\" toolbar_text=\"ffffffff\"\n"
"           toolbar_active=\"ff787878\" toolbar_hover=\"ff888888\" tabbar_background=\"ff000000\"\n"
"           tab_text=\"ffffffff\" selected_tab_background=\"ff808080\" selected_tab_text=\"ffffffff\"\n"
"           canvas_background=\"ff000000\" canvas_text=\"ffffffff\" canvas_dots=\"ff000000\"\n"
"           default_object_background=\"ff000000\" object_outline_colour=\"ffffffff\"\n"
"           selected_object_outline_colour=\"ffffffff\" gui_internal_outline_colour=\"ffffffff\"\n"
"           toolbar_outline_colour=\"ffffffff\" outline_colour=\"ffffffff\" data_colour=\"ffffffff\"\n"
"           connection_colour=\"ffffffff\" signal_colour=\"ffffffff\" dialog_background=\"ff000000\"\n"
"           sidebar_colour=\"ff000000\" sidebar_text=\"ffffffff\" sidebar_background_active=\"ffffffff\"\n"
"           sidebar_active_text=\"ff000000\" levelmeter_active=\"ffffffff\" levelmeter_background=\"ff808080\"\n"
"           levelmeter_thumb=\"ffffffff\" panel_background=\"ff0e0e0e\" panel_foreground=\"ff000000\"\n"
"           panel_text=\"ffffffff\" panel_background_active=\"ffffffff\" panel_active_text=\"ff000000\"\n"
"           popup_background=\"ff000000\" popup_background_active=\"ffffffff\"\n"
"           popup_text=\"ffffffff\" popup_active_text=\"ff000000\" slider_thumb=\"ffffffff\"\n"
"           scrollbar_thumb=\"ff7f7f7f\" graph_resizer=\"ffffffff\" grid_colour=\"ffffffff\"\n"
"           caret_colour=\"ffffffff\" iolet_area_colour=\"ff000000\" iolet_outline_colour=\"ffffffff\"\n"
"           text_object_background=\"ff000000\" comment_text_colour=\"ffffffff\"\n"
"           dashed_signal_connections=\"0\" straight_connections=\"1\"\n"
"           thin_connections=\"1\" square_iolets=\"1\" square_object_corners=\"1\"/>\n"
"    <Theme theme=\"dark\" toolbar_background=\"ff191919\" toolbar_text=\"ffe1e1e1\"\n"
"           toolbar_active=\"ff42a2c8\" toolbar_hover=\"ff282828\" tabbar_background=\"ff191919\"\n"
"           tab_text=\"ffe1e1e1\" selected_tab_background=\"ff2e2e2e\" selected_tab_text=\"ffe1e1e1\"\n"
"           canvas_background=\"ff232323\" canvas_text=\"ffe1e1e1\" canvas_dots=\"ff7f7f7f\"\n"
"           default_object_background=\"ff191919\" object_outline_colour=\"ff696969\"\n"
"           selected_object_outline_colour=\"ff42a2c8\" gui_internal_outline_colour=\"ff696969\"\n"
"           toolbar_outline_colour=\"ff2f2f2f\" outline_colour=\"ff393939\" data_colour=\"ff42a2c8\"\n"
"           connection_colour=\"ffe1e1e1\" signal_colour=\"ffff8500\" dialog_background=\"ff191919\"\n"
"           sidebar_colour=\"ff191919\" sidebar_text=\"ffe1e1e1\" sidebar_background_active=\"ff282828\"\n"
"           sidebar_active_text=\"ffe1e1e1\" levelmeter_active=\"ff42a2c8\" levelmeter_background=\"ff2e2e2e\"\n"
"           levelmeter_thumb=\"ffe3e3e3\" panel_background=\"ff2c2c2c\" panel_foreground=\"ff1f1f1f\"\n"
"           panel_text=\"ffe1e1e1\" panel_background_active=\"ff373737\" panel_active_text=\"ffe1e1e1\"\n"
"           popup_background=\"ff191919\" popup_background_active=\"ff282828\"\n"
"           popup_text=\"ffe1e1e1\" popup_active_text=\"ffe1e1e1\" slider_thumb=\"ff42a2c8\"\n"
"           scrollbar_thumb=\"ff7f7f7f\" graph_resizer=\"ff42a2c8\" grid_colour=\"ff42a2c8\"\n"
"           caret_colour=\"ff42a2c8\" text_object_background=\"ff232323\" iolet_area_colour=\"ff232323\"\n"
"           iolet_outline_colour=\"ff696969\" comment_text_colour=\"ffe1e1e1\"\n"
"           dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
"           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"0\"/>\n"
"    <Theme theme=\"light\" toolbar_background=\"ffebebeb\" toolbar_text=\"ff373737\"\n"
"           toolbar_active=\"ff007aff\" toolbar_hover=\"ffe0e0e0\" tabbar_background=\"ffebebeb\"\n"
"           tab_text=\"ff373737\" selected_tab_background=\"ffe0e0e0\" selected_tab_text=\"ff373737\"\n"
"           canvas_background=\"fffafafa\" canvas_text=\"ff4d4d4d\" canvas_dots=\"ff909090\"\n"
"           default_object_background=\"ffe4e4e4\" object_outline_colour=\"ffb7b7b7\"\n"
"           selected_object_outline_colour=\"ff007aff\" gui_internal_outline_colour=\"ffb7b7b7\"\n"
"           toolbar_outline_colour=\"ffdfdfdf\" outline_colour=\"ffd0d0d0\" data_colour=\"ff007aff\"\n"
"           connection_colour=\"ffb3b3b3\" signal_colour=\"ffff8500\" dialog_background=\"ffebebeb\"\n"
"           sidebar_colour=\"ffefefef\" sidebar_text=\"ff373737\" sidebar_background_active=\"ffe4e4e4\"\n"
"           sidebar_active_text=\"ff373737\" levelmeter_active=\"ff007aff\" levelmeter_background=\"ffe1e1e1\"\n"
"           levelmeter_thumb=\"ff9a9a9a\" panel_background=\"fff7f7f7\" panel_foreground=\"fffdfdfd\"\n"
"           panel_text=\"ff373737\" panel_background_active=\"ffececec\" panel_active_text=\"ff373737\"\n"
"           popup_background=\"ffe8e8e8\" popup_background_active=\"ffdcdcdc\"\n"
"           popup_text=\"ff373737\" popup_active_text=\"ff373737\" slider_thumb=\"ff007aff\"\n"
"           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff007aff\" grid_colour=\"ff007aff\"\n"
"           caret_colour=\"ff007aff\" square_object_corners=\"0\" text_object_background=\"fffafafa\"\n"
"           iolet_area_colour=\"fffafafa\" iolet_outline_colour=\"ffc2c2c2\"\n"
"           comment_text_colour=\"ff373737\" dashed_signal_connections=\"1\"\n"
"           straight_connections=\"0\" thin_connections=\"0\" square_iolets=\"0\"/>"
"    <Theme theme=\"warm\" toolbar_background=\"ffd2cdc4\" toolbar_text=\"ff5a5a5a\"\n"
"           toolbar_active=\"ff5da0c4\" toolbar_hover=\"ffc0bbb2\" tabbar_background=\"ffd2cdc4\"\n"
"           tab_text=\"ff5a5a5a\" selected_tab_background=\"ffc0bbb2\" selected_tab_text=\"ff5a5a5a\"\n"
"           canvas_background=\"ffe3dfd9\" canvas_text=\"ff5a5a5a\" canvas_dots=\"ff909090\"\n"
"           default_object_background=\"ffe3dfd9\" object_outline_colour=\"ff968e82\"\n"
"           selected_object_outline_colour=\"ff5da0c4\" gui_internal_outline_colour=\"ff968e82\"\n"
"           toolbar_outline_colour=\"ffbdb3a4\" outline_colour=\"ff968e82\" data_colour=\"ff5da0c4\"\n"
"           connection_colour=\"ffb3b3b3\" signal_colour=\"ffff8502\" dialog_background=\"ffd2cdc4\"\n"
"           sidebar_colour=\"ffdedad3\" sidebar_text=\"ff5a5a5a\" sidebar_background_active=\"ffefefef\"\n"
"           sidebar_active_text=\"ff5a5a5a\" levelmeter_active=\"ff5da0c4\" levelmeter_background=\"ffc0bbb2\"\n"
"           levelmeter_thumb=\"ff7a7a7a\" panel_background=\"ffd2cdc4\" panel_foreground=\"ffe7e2d8\"\n"
"           panel_text=\"ff5a5a5a\" panel_background_active=\"ffebebeb\" panel_active_text=\"ff5a5a5a\"\n"
"           popup_background=\"ffd2cdc4\" popup_background_active=\"ffc0bbb2\"\n"
"           popup_text=\"ff5a5a5a\" popup_active_text=\"ff5a5a5a\" slider_thumb=\"ff5da0c4\"\n"
"           scrollbar_thumb=\"ffa9a9a9\" graph_resizer=\"ff5da0c4\" grid_colour=\"ff5da0c4\"\n"
"           caret_colour=\"ff5da0c4\" iolet_area_colour=\"ffe3dfd9\" iolet_outline_colour=\"ff968e82\"\n"
"           text_object_background=\"ffe3dfd9\" comment_text_colour=\"ff5a5a5a\"\n"
"           dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
"           thin_connections=\"0\" square_iolets=\"0\" square_object_corners=\"0\"/>\n"
"    <Theme theme=\"fangs\" toolbar_background=\"ff232323\" toolbar_text=\"ffffffff\"\n"
"           toolbar_active=\"ff5bcefa\" toolbar_hover=\"ff383838\" tabbar_background=\"ff232323\"\n"
"           tab_text=\"ffffffff\" selected_tab_background=\"ff3a3a3a\" selected_tab_text=\"ffffffff\"\n"
"           canvas_background=\"ff383838\" canvas_text=\"ffffffff\" canvas_dots=\"ffa0a0a0\"\n"
"           default_object_background=\"ff191919\" object_outline_colour=\"ff383838\"\n"
"           selected_object_outline_colour=\"ffffacab\" gui_internal_outline_colour=\"ff626262\"\n"
"           toolbar_outline_colour=\"ff343434\" outline_colour=\"ff383838\" data_colour=\"ff5bcefa\"\n"
"           connection_colour=\"ffa0a0a0\" signal_colour=\"ffffacab\" dialog_background=\"ff191919\"\n"
"           sidebar_colour=\"ff232323\" sidebar_text=\"ffffffff\" sidebar_background_active=\"ff383838\"\n"
"           sidebar_active_text=\"ffffffff\" levelmeter_active=\"ff5bcefa\" levelmeter_background=\"ff3a3a3a\"\n"
"           levelmeter_thumb=\"fff5f5f5\" panel_background=\"ff2c2c2c\" panel_foreground=\"ff1f1f1f\"\n"
"           panel_text=\"ffffffff\" panel_background_active=\"ff232323\" panel_active_text=\"ffffffff\"\n"
"           popup_background=\"ff232323\" popup_background_active=\"ff383838\"\n"
"           popup_text=\"ffffffff\" popup_active_text=\"ffffffff\" scrollbar_thumb=\"ff8e8e8e\"\n"
"           graph_resizer=\"ff5bcefa\" grid_colour=\"ff5bcefa\" caret_colour=\"ffffacab\"\n"
"           text_object_background=\"ff232323\" iolet_area_colour=\"ff232323\"\n"
"           iolet_outline_colour=\"ff696969\" slider_thumb=\"ff8e8e8e\" comment_text_colour=\"ffffffff\"\n"
"           dashed_signal_connections=\"1\" straight_connections=\"0\"\n"
"           thin_connections=\"1\" square_iolets=\"1\" square_object_corners=\"0\"/>\n"
"  </ColourThemes>";

// clang-format on

void PlugDataLook::resetColours(ValueTree themesTree)
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

Colour PlugDataLook::getThemeColour(ValueTree themeTree, PlugDataColour colourId)
{
    return Colour::fromString(themeTree.getProperty(std::get<1>(PlugDataColourNames.at(colourId))).toString());
}

void PlugDataLook::setTheme(ValueTree themeTree)
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

StringArray PlugDataLook::getAllThemes()
{
    auto themeTree = SettingsFile::getInstance()->getColourThemesTree();
    StringArray allThemes;
    for (auto theme : themeTree) {
        allThemes.add(theme.getProperty("theme").toString());
    }

    return allThemes;
}

bool PlugDataLook::getUseDashedConnections()
{
    return useDashedConnections;
}
bool PlugDataLook::getUseStraightConnections()
{
    return useStraightConnections;
}
bool PlugDataLook::getUseThinConnections()
{
    return useThinConnections;
}
bool PlugDataLook::getUseSquareIolets()
{
    return useSquareIolets;
}
