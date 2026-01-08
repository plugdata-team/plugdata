/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

// Eyedropper will create a snapshot of the top level component,
// to allow the user to pick colours from anywhere in the app
class Eyedropper final : public Timer
    , public MouseListener {
    class EyedropperDisplayComponnent final : public Component {
        Colour colour;
        Image pixelImage;

    public:
        std::function<void(bool setColour)> onDismiss = [](bool) { };

        EyedropperDisplayComponnent()
        {
            setVisible(true);
            setAlwaysOnTop(true);
            setInterceptsMouseClicks(true, true);
            setWantsKeyboardFocus(true);
            setSize(130, 130);
            setMouseCursor(MouseCursor::NoCursor);
        }

        void show()
        {
            addToDesktop(ComponentPeer::windowIsTemporary);
            grabKeyboardFocus();
        }

        void hide()
        {
            removeFromDesktop();
        }

        void mouseDown(MouseEvent const&) override
        {
            onDismiss(true);
        }

        bool keyPressed(KeyPress const& key) override
        {
            if (bool const retPressed = key == KeyPress::returnKey; retPressed || key == KeyPress::escapeKey) {
                onDismiss(retPressed);
                return true;
            }
            return false;
        }

        void setROI(Image const& image, Point<int> const position)
        {
            pixelImage = image.getClippedImage(Rectangle<int>(0, 0, 11, 11).withCentre(position)).rescaled(getWidth() - 8 * 2, getHeight() - 8 * 2, Graphics::ResamplingQuality::lowResamplingQuality);
            colour = image.getPixelAt(position.x, position.y);
            repaint();
        }

        void paint(Graphics& g) override
        {
            auto const bounds = getLocalBounds().toFloat().reduced(8);

            Path shadowPath;
            shadowPath.addEllipse(bounds);
            StackShadow::renderDropShadow(hash("eyedropper"), g, shadowPath, Colours::black.withAlpha(0.85f), 8, { 0, 1 }, 0);

            auto const circleBounds = bounds;

            g.reduceClipRegion(shadowPath);

            g.drawImage(pixelImage, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, 0, bounds.getWidth(), bounds.getHeight());

            auto const rectSize = bounds.getWidth() / 12.0f;
            auto highlightRect = Rectangle<int>(0, 0, rectSize, rectSize);
            highlightRect.setCentre(bounds.getCentreX(), bounds.getCentreY());

            // Draw dark background for pixel highlight
            g.setColour(Colours::black);
            g.drawRect(highlightRect.expanded(2, 2), 3.0f);

            // Draw the pixel highlight
            g.setColour(Colour::greyLevel(0.9f));
            g.drawRect(highlightRect.expanded(1, 1), 1.0f);

            g.drawEllipse(circleBounds, 2.0f);
        }
    };

public:
    class EyedropperButton final : public TextButton {
        void paint(Graphics& g) override
        {
            TextButton::paint(g);
            Fonts::drawIcon(g, Icons::Eyedropper, getLocalBounds().reduced(2), findColour(TextButton::textColourOffId));
        }
    };

    Eyedropper()
    {
        colourDisplayer.onDismiss = [this](bool const setColour) {
            hideEyedropper(setColour);
        };
    }

    ~Eyedropper() override
    {
        if (topLevel) {
            topLevel->removeMouseListener(this);
        }
    }

    void showEyedropper(PluginEditor* targetEditor, Component* topLevelComponent, std::function<void(Colour)> cb)
    {
        callback = std::move(cb);
        colourDisplayer.show();
        topLevel = topLevelComponent;
        topLevel->addMouseListener(this, true);
        editor = targetEditor;

        timerCount = 0;
        timerCallback();
        startTimerHz(60);
    }

    void hideEyedropper(bool const setColour)
    {
        if (setColour) {
            callback(currentColour);
        }
        callback = [](Colour) { };
        colourDisplayer.hide();
        stopTimer();
        topLevel->removeMouseListener(this);
        topLevel = nullptr;
    }

private:
    void setColour(Colour const colour)
    {
        currentColour = colour;
    }

    void timerCallback() override
    {
        auto const position = topLevel->getMouseXYRelative();
        auto const surfaceMousePosition = editor->nvgSurface.getLocalPoint(topLevel, position);
        auto const mouseOverSurface = editor->nvgSurface.getLocalBounds().contains(surfaceMousePosition);

        if (mouseOverSurface) {
            editor->nvgSurface.setRenderThroughImage(true);
            editor->nvgSurface.render();
        }

        componentImage = topLevel->createComponentSnapshot(topLevel->getLocalBounds(), false, 1.0f);
        colourDisplayer.setCentrePosition(topLevel->localPointToGlobal(position));
        colourDisplayer.setROI(componentImage, position);
        setColour(componentImage.getPixelAt(position.x, position.y));

        if (mouseOverSurface) {
            editor->nvgSurface.setRenderThroughImage(false);
        }
    }

    std::function<void(Colour)> callback;
    int timerCount = 0;
    Component* topLevel = nullptr;

    EyedropperDisplayComponnent colourDisplayer;
    Image componentImage;
    Colour currentColour;
    PluginEditor* editor;
};

class ColourPicker final : public Component {
    class SelectorHolder final : public Component {
    public:
        explicit SelectorHolder(ColourPicker* parent)
            : colourPicker(parent)
        {
            addAndMakeVisible(parent);
            setBounds(parent->getLocalBounds());
        }

        ~SelectorHolder() override
        {
            colourPicker->runCallback();
        }

    private:
        ColourPicker* colourPicker;
    };

    struct ColourComponentSlider final : public Slider {
        explicit ColourComponentSlider(String const& name)
            : Slider(name)
        {
            setTextBoxStyle(TextBoxLeft, false, 35, 20);
            setRange(0.0, 255.0, 1.0);
        }

        String getTextFromValue(double const value) override
        {
            return String(static_cast<int>(value));
        }

        double getValueFromText(String const& text) override
        {
            return text.getIntValue();
        }
    };

public:
    void show(PluginEditor* editorComponent, Component* topLevelComponent, bool const onlySendCallbackOnClose, Colour const currentColour, Rectangle<int> const bounds, std::function<void(Colour)> const& colourCallback)
    {
        callback = colourCallback;
        onlyCallBackOnClose = onlySendCallbackOnClose;
        editor = editorComponent;

        _topLevelComponent = topLevelComponent;

        setCurrentColour(currentColour);

        // we need to put the selector into a holder, as launchAsynchronously will delete the component when its done
        auto selectorHolder = std::make_unique<SelectorHolder>(this);

        if (calloutBox) {
            calloutBox->dismiss();
        }

        calloutBox = &CallOutBox::launchAsynchronously(std::move(selectorHolder), bounds, nullptr);
    }

    ColourPicker()
        : colour(Colours::white)
        , colourSpace(*this, h, s)
        , brightnessSelector(*this, v)
        , edgeGap(2)
    {
        updateHSV();

        addAndMakeVisible(sliders[0]);
        addAndMakeVisible(sliders[1]);
        addAndMakeVisible(sliders[2]);

        addAndMakeVisible(colourSpace);
        addAndMakeVisible(brightnessSelector);

        showRgb.setRadioGroupId(hash("colour_picker"));
        showHex.setRadioGroupId(hash("colour_picker"));

        showRgb.setClickingTogglesState(true);
        showHex.setClickingTogglesState(true);

        addAndMakeVisible(showRgb);
        addAndMakeVisible(showHex);
        addAndMakeVisible(showEyedropper);

        hexEditor.setJustificationType(Justification::centred);
        hexEditor.setEditable(true);
        hexEditor.onEditorShow = [this] {
            if (auto* editor = hexEditor.getCurrentTextEditor()) {
                editor->setInputRestrictions(6, "ABCDEFabcdef0123456789");
                editor->setJustification(Justification::centred);
            }
        };
        hexEditor.onTextChange = [this] { changeColour(); };

        addChildComponent(hexEditor);

        showRgb.onClick = [this] {
            updateMode();
        };

        showHex.onClick = [this] {
            updateMode();
        };

        _topLevelComponent = getTopLevelComponent();

        showEyedropper.onClick = [this]() mutable {
            eyedropper.showEyedropper(editor, _topLevelComponent, [this](Colour const pickedColour) {
                setCurrentColour(pickedColour);
            });
        };

        showRgb.setToggleState(true, dontSendNotification);

        showRgb.setConnectedEdges(Button::ConnectedOnLeft);
        showHex.setConnectedEdges(Button::ConnectedOnRight);

        update(dontSendNotification);

        updateMode();

        lookAndFeelChanged();
    }

    static ColourPicker& getInstance()
    {
        static ColourPicker instance;
        return instance;
    }

    ~ColourPicker() override { }

    void lookAndFeelChanged() override
    {
        for (auto* slider : sliders) {
            slider->onValueChange = [this] { changeColour(); };
            slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            slider->setColour(Slider::textBoxBackgroundColourId, findColour(PlugDataColour::popupMenuBackgroundColourId));
            slider->setColour(Slider::textBoxTextColourId, findColour(PlugDataColour::popupMenuTextColourId));
        }

        showRgb.setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::toolbarHoverColourId));
        showHex.setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::toolbarHoverColourId));

        hexEditor.setColour(Label::outlineWhenEditingColourId, Colours::transparentBlack);
    }

    void runCallback()
    {
        if (onlyCallBackOnClose)
            callback(getCurrentColour());
    }

    void updateMode()
    {
        auto const hex = showHex.getToggleState();
        for (auto const& slider : sliders) {
            slider->setVisible(!hex);
        }

        hexEditor.setVisible(hex);
        update(dontSendNotification);
        repaint();

        if (hex) {
            setSize(200, 256);
        } else {
            setSize(200, 300);
        }

        if (auto* parent = getParentComponent()) {
            parent->setSize(getWidth(), getHeight()); // Set size of SelectorHolder
        }
    }

    Colour getCurrentColour() const
    {
        return colour.withAlpha(static_cast<uint8>(0xff));
    }

    void setCurrentColour(Colour const c, NotificationType const notification = sendNotification)
    {
        if (c != colour) {
            colour = c.withAlpha(static_cast<uint8>(0xff));

            updateHSV();
            update(notification);
        }
    }

private:
    void setBrightness(float newV)
    {
        newV = jlimit(0.0f, 1.0f, newV);

        if (!approximatelyEqual(v, newV)) {
            v = newV;
            colour = Colour(h, s, v, colour.getFloatAlpha());
            update(sendNotification);
        }
    }

    auto getHS() const
    {
        struct HS {
            float hue, sat;
        };

        return HS { h, s };
    }

    void setHS(float newH, float newS)
    {
        newH = jlimit(0.0f, 1.0f, newH);
        newS = jlimit(0.0f, 1.0f, newS);

        if (!approximatelyEqual(h, newH) || !approximatelyEqual(s, newS)) {
            h = newH;
            s = newS;

            colour = Colour(h, s, v, colour.getFloatAlpha());
            update(sendNotification);
        }
    }

    void updateHSV()
    {
        colour.getHSB(h, s, v);
    }

    void update(NotificationType const notification)
    {
        if (sliders[0] != nullptr) {
            sliders[0]->setValue(colour.getRed(), dontSendNotification);
            sliders[1]->setValue(colour.getGreen(), dontSendNotification);
            sliders[2]->setValue(colour.getBlue(), dontSendNotification);
        }

        hexEditor.setText(colour.toString().substring(2), dontSendNotification);

        colourSpace.updateIfNeeded();
        brightnessSelector.updateIfNeeded();

        if (notification != dontSendNotification && !onlyCallBackOnClose)
            callback(getCurrentColour());
    }

    void changeColour()
    {
        if (hexEditor.isVisible()) {
            setCurrentColour(Colour::fromString(hexEditor.getText()));
        } else {
            setCurrentColour(Colour(static_cast<uint8>(sliders[0]->getValue()),
                static_cast<uint8>(sliders[1]->getValue()),
                static_cast<uint8>(sliders[2]->getValue())));
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(findColour(PlugDataColour::popupMenuBackgroundColourId));

        g.setColour(findColour(PlugDataColour::popupMenuTextColourId));
        g.setFont(14.0f);

        for (auto const& slider : sliders) {
            if (slider->isVisible())
                g.drawText(slider->getName() + ":",
                    0, slider->getY(),
                    slider->getX() - 8, slider->getHeight(),
                    Justification::centredRight, false);
        }

        if (hexEditor.isVisible()) {
            g.drawText("HEX:",
                8, hexEditor.getY() + 1,
                hexEditor.getX() - 8, hexEditor.getHeight(),
                Justification::centredRight, false);
        }
    }

    void resized() override
    {
        auto const numSliders = hexEditor.isVisible() ? 1 : 3;
        constexpr int hueWidth = 16;

        auto bounds = getLocalBounds();

        auto sliderBounds = bounds.removeFromBottom(22 * numSliders + edgeGap);

        auto const heightLeft = bounds.getHeight() - bounds.getWidth();

        auto controlSelectBounds = bounds.removeFromBottom(heightLeft).reduced(10, 6).translated(0, -12);
        auto const colourSpaceBounds = bounds.removeFromLeft(bounds.getWidth() - hueWidth);

        colourSpace.setBounds(colourSpaceBounds);
        brightnessSelector.setBounds(bounds.withTrimmedBottom(heightLeft).translated(-4, 8).expanded(0, 2));

        showEyedropper.setBounds(controlSelectBounds.removeFromRight(24).translated(2, 0));
        controlSelectBounds.removeFromRight(6);
        showHex.setBounds(controlSelectBounds.removeFromLeft(controlSelectBounds.getWidth() / 2));
        showRgb.setBounds(controlSelectBounds.withTrimmedLeft(-1));

        sliderBounds.removeFromLeft(30);

        auto const sliderHeight = sliderBounds.proportionOfHeight(0.33333f);
        for (int i = 0; i < numSliders; ++i) {
            if (sliders[i]->isVisible()) {
                sliders[i]->setBounds(sliderBounds.removeFromTop(sliderHeight));
            }
        }

        hexEditor.setBounds(sliderBounds.reduced(5, 2).translated(4, -4));
    }

    class ColourSpaceView final : public Component {

    public:
        ColourSpaceView(ColourPicker& cp, float& hue, float& sat)
            : owner(cp)
            , h(hue)
            , s(sat)
            , marker(cp)
        {
            addAndMakeVisible(marker);
            setMouseCursor(MouseCursor::CrosshairCursor);
        }

        void updateImage()
        {
            int const circleRadius = imageSize / 2;

            colourWheelHSV = Image(Image::PixelFormat::ARGB, imageSize, imageSize, true);

            Graphics g(colourWheelHSV);

            for (int y = 0; y < imageSize; ++y) {
                for (int x = 0; x < imageSize; ++x) {
                    // calculate the distance of this pixel from the center
                    float const dx = x - imageSize / 2.0f;
                    float const dy = y - imageSize / 2.0f;
                    float const distance = std::sqrt(dx * dx + dy * dy);

                    // only draw within the circle
                    if (distance <= circleRadius) {
                        constexpr float antiAliasingRadius = 2.0f;
                        // calculate the color at this pixel using HSV color space
                        float const hue = std::atan2(dy, dx);
                        float const saturation = distance / circleRadius;
                        constexpr float value = 1.0f;

                        // convert the HSV color to RGB
                        Colour colour = Colour::fromHSV(hue / MathConstants<float>::twoPi, saturation, value, 1.0f);

                        // calculate the alpha for anti-aliasing
                        float const alpha = std::min(1.0f, (circleRadius - distance) / antiAliasingRadius);

                        // apply the alpha to the color
                        colour = colour.withAlpha(alpha);

                        // set the pixel to the calculated color
                        colourWheelHSV.setPixelAt(x, y, colour);
                    }
                }
            }
        }

        void paint(Graphics& g) override
        {
            // draw the image
            g.drawImageAt(colourWheelHSV, margin, margin);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawEllipse(imageBounds.toFloat().reduced(0.5f), 1.0f);
        }

        void mouseDown(MouseEvent const& e) override
        {
            mouseDrag(e);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            auto const area = imageBounds;
            auto constexpr markerSize = 20;
            auto const centerX = area.getX() + area.getWidth() * 0.5f;
            auto const centerY = area.getY() + area.getHeight() * 0.5f;

            // Calculate the distance from the center of the wheel to the specified position
            float const dx = e.x - centerX;
            float const dy = e.y - centerY;
            float const dist = std::sqrt(dx * dx + dy * dy);

            // Calculate the maximum distance from the center of the wheel to the edge of the color wheel
            float const maxDist = jmin(area.getWidth(), area.getHeight()) * 0.5f - markerSize * 0.5f;

            // Calculate the hue as an angle in radians
            float const hueAngle = std::atan2(dy, dx);

            // Normalize the hue angle to the range [0, 1]
            float hue = hueAngle / MathConstants<float>::twoPi;
            if (hue < 0.0f)
                hue += 1.0f;

            // Calculate the saturation as a percentage of the maximum distance from the center
            float const saturation = dist / maxDist;

            owner.setHS(hue, saturation);

            owner.brightnessSelector.repaint();
        }

        void updateIfNeeded()
        {
            if (lastHue != h) {
                lastHue = h;
                repaint();
            }

            updateMarker();
            owner.brightnessSelector.repaint();
        }

        void resized() override
        {
            imageSize = getWidth() - margin * 2;
            imageBounds = Rectangle<int>(margin, margin, imageSize, imageSize);
            updateImage();
            updateMarker();
        }

    private:
        ColourPicker& owner;
        float& h;
        float& s;
        float lastHue = 0;

        static constexpr int margin = 10;
        int imageSize;
        Image colourWheelHSV;
        Rectangle<int> imageBounds;

        struct ColourSpaceMarker final : public Component {
            ColourPicker& owner;

            explicit ColourSpaceMarker(ColourPicker& parent)
                : owner(parent)
            {
                setInterceptsMouseClicks(false, false);
            }

            void paint(Graphics& g) override
            {
                auto const bounds = getLocalBounds().reduced(4).toFloat();

                Path shadowPath;
                shadowPath.addEllipse(bounds);
                StackShadow::renderDropShadow(hash("colour_space_marker"), g, shadowPath, Colours::black.withAlpha(0.75f), 6, { 0, 0 }, 0);

                auto [hue, sat] = owner.getHS();
                auto const colour = Colour::fromHSV(hue, sat, 1.0f, 1.0f);

                g.setColour(colour);
                g.fillEllipse(bounds);

                g.setColour(Colour::greyLevel(0.9f));
                g.drawEllipse(bounds, 2.0f);
            }
        };

        ColourSpaceMarker marker;

        void updateMarker()
        {
            auto constexpr markerSize = 20;
            auto const area = imageBounds;

            // Calculate the position of the marker based on the current hue and saturation values
            float const centerX = area.getX() + area.getWidth() * 0.5f;
            float const centerY = area.getY() + area.getHeight() * 0.5f;
            float const radius = jmin(area.getWidth(), area.getHeight()) * 0.5f - markerSize * 0.5f;
            float const angle = h * MathConstants<float>::twoPi;
            float const x = centerX + std::cos(angle) * radius * s;
            float const y = centerY + std::sin(angle) * radius * s;

            // Set the position of the marker
            marker.setBounds(static_cast<int>(x) - markerSize / 2, static_cast<int>(y) - markerSize / 2, markerSize, markerSize);
        }

        JUCE_DECLARE_NON_COPYABLE(ColourSpaceView)
    };

    class BrightnessSelectorComp final : public Component {
    public:
        BrightnessSelectorComp(ColourPicker& cp, float& value)
            : owner(cp)
            , v(value)
            , edge(5)
            , marker(cp)
        {
            addAndMakeVisible(marker);
        }

        void paint(Graphics& g) override
        {
            auto [hue, sat] = owner.getHS();
            auto const colour = Colour::fromHSV(hue, sat, 1.0f, 1.0f);

            auto const bounds = getLocalBounds().toFloat().reduced(edge);
            auto const radius = jmin(Corners::defaultCornerRadius, bounds.getWidth() / 2.0f);

            g.setGradientFill(ColourGradient(colour, 0.0f, 0.0f, Colours::black, bounds.getHeight() / 2, bounds.getHeight() / 2, false));
            g.fillRoundedRectangle(bounds, radius);

            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        }

        void resized() override
        {
            auto constexpr markerSize = 20;
            auto const area = getLocalBounds().reduced(edge);

            marker.setBounds(Rectangle<int>(markerSize, markerSize)
                    .withCentre(area.getRelativePoint(0.5f, 1.0f - v)));
        }

        void mouseDown(MouseEvent const& e) override
        {
            mouseDrag(e);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            owner.setBrightness(1.0f - static_cast<float>(e.y - edge) / static_cast<float>(getHeight() - edge * 2));
        }

        void updateIfNeeded()
        {
            resized();
        }

    private:
        ColourPicker& owner;
        float& v;
        int const edge;

        struct BrightnessSelectorMarker final : public Component {
            ColourPicker& owner;

            explicit BrightnessSelectorMarker(ColourPicker& parent)
                : owner(parent)
            {
                setInterceptsMouseClicks(false, false);
            }

            void paint(Graphics& g) override
            {
                auto const bounds = getLocalBounds().reduced(4).toFloat();

                Path shadowPath;
                shadowPath.addEllipse(bounds.reduced(2));
                StackShadow::renderDropShadow(hash("colour_picker"), g, shadowPath, Colours::black.withAlpha(0.75f), 6, { 0, 1 }, 0);

                g.setColour(owner.getCurrentColour());
                g.fillEllipse(bounds);

                g.setColour(Colour::greyLevel(0.9f));
                g.drawEllipse(bounds, 2.0f);
            }
        };

        BrightnessSelectorMarker marker;

        JUCE_DECLARE_NON_COPYABLE(BrightnessSelectorComp)
    };

    Colour colour;
    float h, s, v;

    OwnedArray<Slider> sliders = { new ColourComponentSlider("R"), new ColourComponentSlider("G"), new ColourComponentSlider("B") };

    ColourSpaceView colourSpace;
    BrightnessSelectorComp brightnessSelector;
    Label hexEditor;

    int edgeGap;

    TextButton showHex = TextButton("HEX"), showRgb = TextButton("RGB");
    Eyedropper::EyedropperButton showEyedropper;

    Eyedropper eyedropper;

    SafePointer<CallOutBox> calloutBox = nullptr;

    Component* _topLevelComponent;
    PluginEditor* editor;

    bool onlyCallBackOnClose;

    std::function<void(Colour)> callback = [](Colour) { };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColourPicker)
};
