/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <Utility/MarkupDisplay.h>

class HelpDialog : public Component
    , public MarkupDisplay::FileSource {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;

    static inline File const manualPath = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Manual");

    class IndexComponent : public Component {
    public:
        IndexComponent(std::function<void(File const&)> loadFile)
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

    //IndexComponent index;

public:
    std::function<void()> onClose;
    PluginProcessor* pd;
    MarkupDisplay::MarkupDisplayComponent markupDisplay;

    HelpDialog(PluginProcessor* instance)
        : resizer(this, &constrainer)
        //, index([this](File const& file) { markupDisplay.setMarkdownString(file.loadFileAsString()); })
        , pd(instance)
    {
        markupDisplay.setFileSource(this);
        markupDisplay.setFont(Fonts::getVariableFont());
        markupDisplay.setMarkdownString(manualPath.getChildFile("CompilingPatches.md").loadFileAsString());
        addAndMakeVisible(markupDisplay);

        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 300);

        closeButton->onClick = [this]() {
            MessageManager::callAsync([this]() {
                onClose();
            });
        };

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(700, 500));

        addAndMakeVisible(resizer);
        //addAndMakeVisible(index);
    }

    Image getImageForFilename(String filename) override
    {
        return ImageFileFormat::loadFrom(manualPath.getChildFile(filename));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        resizer.setBounds(bounds);
        bounds.removeFromTop(40);

        auto closeButtonBounds = getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5);
        closeButton->setBounds(closeButtonBounds);

        //index.setBounds(bounds.removeFromLeft(200));
        markupDisplay.setBounds(bounds);
    }

    void mouseDown(MouseEvent const& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }

    void paint(Graphics& g) override
    {
        auto toolbarHeight = 38;
        auto b = getLocalBounds();
        auto titlebarBounds = b.removeFromTop(toolbarHeight).toFloat();
        auto bgBounds = b.toFloat();
        //auto sidebarBounds = b.removeFromLeft(200);

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(toolbarPath);

        Path backgroundPath;
        backgroundPath.addRoundedRectangle(bgBounds.getX(), bgBounds.getY(), bgBounds.getWidth(), bgBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);
        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillPath(backgroundPath);
        
        /*
        Path sidebarPath;
        backgroundPath.addRoundedRectangle(sidebarBounds.getX(), sidebarBounds.getY(), sidebarBounds.getWidth(), sidebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, false);
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId)); */
        g.fillPath(backgroundPath);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(toolbarHeight, 0, getWidth());
        g.drawVerticalLine(200, 40, getHeight());

        Fonts::drawStyledText(g, "Help", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
    }
};
