/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/MarkupDisplay.h"
#include "Components/BouncingViewport.h"

class HelpDialog : public TopLevelWindow
    , public MarkupDisplay::FileSource {
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> resizer;

    static inline File const manualPath = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Manual");

    class IndexComponent : public Component {
    public:
        explicit IndexComponent(std::function<void(File const&)> loadFile)
        {
            for (auto const& file : OSUtils::iterateDirectory(manualPath, false, true)) {
                if (file.hasFileExtension(".md")) {
                    auto* button = buttons.add(new TextButton(file.getFileNameWithoutExtension()));
                    button->onClick = [loadFile, file]() {
                        loadFile(file);
                    };

                    button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::sidebarActiveBackgroundColourId));
                    button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::sidebarBackgroundColourId));
                    button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
                    contentComponent.addAndMakeVisible(button);
                }
            }

            viewport.setViewedComponent(&contentComponent, false);
            addAndMakeVisible(viewport);
        }

        void resized() override
        {
            contentComponent.setBounds(getLocalBounds().withHeight(std::max(getHeight(), buttons.size() * 28)));
            viewport.setBounds(getLocalBounds());

            auto b = contentComponent.getBounds().withTrimmedTop(8);
            for (auto* button : buttons) {
                button->setBounds(b.removeFromTop(28).reduced(6, 2));
            }
        }

        String pascalCaseToSpaced(String const& pascalCase)
        {
            String spacedString;

            // Iterate through each character in the input string
            for (int i = 0; i < pascalCase.length(); ++i) {
                // If the current character is uppercase and not the first character,
                // add a space before appending the character to the result string
                if (CharacterFunctions::isUpperCase(pascalCase[i]) && i > 0 && (i == pascalCase.length() - 1 || !CharacterFunctions::isUpperCase(pascalCase[i + 1]))) {
                    spacedString += ' ';
                }

                // Append the current character to the result string
                spacedString += pascalCase[i];
            }

            return spacedString;
        }

        BouncingViewport viewport;
        Component contentComponent;
        OwnedArray<TextButton> buttons;
    };

    // IndexComponent index;

public:
    std::function<void()> onClose;
    PluginProcessor* pd;
    MarkupDisplay::MarkupDisplayComponent markupDisplay;
    ComponentBoundsConstrainer constrainer;
    int margin;

    explicit HelpDialog(PluginProcessor* instance)
        : TopLevelWindow("Help", true)
        , pd(instance)
        , margin(ProjectInfo::canUseSemiTransparentWindows() ? 15 : 0)
    //, index([this](File const& file) { markupDisplay.setMarkdownString(file.loadFileAsString()); })
    {
        markupDisplay.setFileSource(this);
        markupDisplay.setFont(Fonts::getVariableFont());
        markupDisplay.setMarkdownString(manualPath.getChildFile("CompilingPatches.md").loadFileAsString());
        addAndMakeVisible(&markupDisplay);

        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        setVisible(true);
        setOpaque(false);
        setUsingNativeTitleBar(false);
        setDropShadowEnabled(false);

        closeButton->onClick = [this]() {
            MessageManager::callAsync([this]() {
                onClose();
            });
        };

        setVisible(true);

        setTopLeftPosition(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.getCentre() - Point<int>(350, 250));
        constrainer.setSizeLimits(500, 300, 1400, 1000);
        constrainer.setFixedAspectRatio(0.0f);

        resizer = std::make_unique<MouseRateReducedComponent<ResizableBorderComponent>>(this, &constrainer);
        // resizer->setAllowHostManagedResize(false);
        resizer->setAlwaysOnTop(true);
        addAndMakeVisible(resizer.get());

        setSize(700, 500);
        // addAndMakeVisible(index);
    }

    Image getImageForFilename(String filename) override
    {
        return ImageFileFormat::loadFrom(manualPath.getChildFile(filename));
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(margin);
        resizer->setBounds(bounds);
        bounds.removeFromTop(40);

        auto closeButtonBounds = getLocalBounds().reduced(margin).removeFromTop(30).removeFromRight(30).translated(-5, 5);
        closeButton->setBounds(closeButtonBounds);

        // index.setBounds(bounds.removeFromLeft(200));
        markupDisplay.setBounds(bounds.reduced(2, 0));
    }

    int getDesktopWindowStyleFlags() const override
    {
        int styleFlags = TopLevelWindow::getDesktopWindowStyleFlags();
        styleFlags |= ComponentPeer::windowIsResizable;
        return styleFlags;
    }

    void mouseDown(MouseEvent const& e) override
    {
        auto dragHitBox = getLocalBounds().reduced(margin).removeFromTop(38).reduced(4);
        if (dragHitBox.contains(e.x, e.y)) {
            windowDragger.startDraggingComponent(this, e);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        auto dragHitBox = getLocalBounds().reduced(margin).removeFromTop(38).reduced(4);
        if (dragHitBox.contains(e.getMouseDownX(), e.getMouseDownY())) {
            windowDragger.dragComponent(this, e, nullptr);
        }
    }

    void paint(Graphics& g) override
    {
        auto toolbarHeight = 38;
        auto totalBounds = getLocalBounds().reduced(margin);
        auto b = totalBounds;
        auto titlebarBounds = b.removeFromTop(toolbarHeight).toFloat();
        auto bgBounds = b.toFloat();
        // auto sidebarBounds = b.removeFromLeft(200);

        if (ProjectInfo::canUseSemiTransparentWindows()) {
            auto shadowPath = Path();
            shadowPath.addRoundedRectangle(getLocalBounds().reduced(20), Corners::windowCornerRadius);
            StackShadow::renderDropShadow(g, shadowPath, Colour(0, 0, 0).withAlpha(0.6f), 13.0f);
        }

        float cornerRadius = ProjectInfo::canUseSemiTransparentWindows() ? Corners::windowCornerRadius : 0.0f;

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), cornerRadius, cornerRadius, true, true, false, false);
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(toolbarPath);

        Path backgroundPath;
        backgroundPath.addRoundedRectangle(bgBounds.getX(), bgBounds.getY(), bgBounds.getWidth(), bgBounds.getHeight(), cornerRadius, cornerRadius, false, false, true, true);
        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillPath(backgroundPath);

        /*
        Path sidebarPath;
        backgroundPath.addRoundedRectangle(sidebarBounds.getX(), sidebarBounds.getY(), sidebarBounds.getWidth(), sidebarBounds.getHeight(), cornerRadius, cornerRadius, false, false, true, false);
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId)); */
        g.fillPath(backgroundPath);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(b.getY() + toolbarHeight, b.getX(), b.getWidth());

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawRoundedRectangle(totalBounds.toFloat().reduced(0.5f), cornerRadius, 1.f);

        // g.drawVerticalLine(b.getX() + 200, b.getY() + 40, g.getHeight());

        Fonts::drawStyledText(g, "Help", Rectangle<float>(totalBounds.getX(), totalBounds.getY() + 4.0f, b.getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
    }
};
