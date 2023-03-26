#pragma once

#include "PluginEditor.h"
// #include "Standalone/PlugDataWindow.h"

class PluginMode : public Component
    , public Button::Listener {
public:
    PluginMode(Canvas* cnv)
        : cnv(cnv)
        , editor(cnv->editor)
        , settings(SettingsFile::getInstance())
        , cnvParent(cnv->getParentComponent())
        , viewportBounds(cnv->viewport->getBounds())
        , infiniteCanvas(settings->getProperty<int>("infinite_canvas"))
    {
        if (ProjectInfo::isStandalone) {
            mainWindow = editor->findParentComponentOfClass<DocumentWindow>();
        }

        auto c = editor->getConstrainer();
        windowConstrainer.setSizeLimits(c->getMinimumWidth(), c->getMinimumHeight(), c->getMaximumWidth(), c->getMaximumHeight());

        // Hide all of the editor's content
        for (auto* child : editor->getChildren()) {
            if (child->isVisible()) {
                child->setVisible(false);
                children.emplace_back(child);
            }
        }

        // Reset zoom level
        editor->zoomScale = 1.0f;

        // Set window bounds
        if (ProjectInfo::isStandalone) {
            windowBounds.setBounds(mainWindow->getX(), mainWindow->getY(), mainWindow->getWidth(), mainWindow->getHeight());
            mainWindow->getMaximiseButton()->setVisible(false);
            mainWindow->setResizeLimits(width / 2, height / 2 + titlebarHeight, width * 2, height * 2 + titlebarHeight);
            mainWindow->setSize(width, height + titlebarHeight);

            editor->setResizeLimits(width / 2, height / 2 + titlebarHeight, width * 2, height * 2 + titlebarHeight);
            editor->setSize(width, height + titlebarHeight);

        } else {
            windowBounds.setBounds(editor->getX(), editor->getY(), editor->getWidth(), editor->getHeight());
            editor->setResizeLimits(width / 2, height / 2 + titlebarHeight, width * 2, height * 2 + titlebarHeight);
            editor->setSize(width, height + titlebarHeight);
        }

        // Add this view to the editor
        setBounds(0, 0, width, height + titlebarHeight);
        editor->addAndMakeVisible(this);

        // Titlebar
        titleBar.setBounds(0, 0, width, titlebarHeight);
        titleBar.addMouseListener(this, true);

        closeButton = std::make_unique<TextButton>(Icons::Edit);
        closeButton->getProperties().set("Style", "LargeIcon");
        closeButton->setTooltip("Show Editor..");
        if (ProjectInfo::isStandalone && !settings->getProperty<bool>("macos_buttons")) {
            closeButton->setBounds(0, 0, titlebarHeight, titlebarHeight);
        } else {
            closeButton->setBounds(getWidth() - titlebarHeight, 0, titlebarHeight, titlebarHeight);
        }
        closeButton->addListener(this);
        titleBar.addAndMakeVisible(*closeButton);

        addAndMakeVisible(titleBar);

        // Viewed Content (canvas)
        content.setBounds(0, titlebarHeight, width, height);

        if (infiniteCanvas)
            settings->setProperty("infinite_canvas", false); // Temporarily disable infinte canvas

        cnv->updatingBounds = true;
        cnv->viewport->setViewPosition(cnv->canvasOrigin);
        cnv->updatingBounds = false;

        content.addAndMakeVisible(cnv);
        cnv->viewport->setSize(width + cnv->viewport->getScrollBarThickness(), height + cnv->viewport->getScrollBarThickness());
        cnv->locked = true;
        cnv->presentationMode = true;

        MessageManager::callAsync([this, cnv] {
            // Called async to make sure viewport pos has updated
            cnv->updatingBounds = true;
            cnv->setBounds(0, 0, width, height);
        });

        addAndMakeVisible(content);
    }

    ~PluginMode()
    {
        if (infiniteCanvas)
            settings->setProperty("infinite_canvas", true);
    }

    void buttonClicked(Button* button) override
    {
        if (button == closeButton.get()) {
            cnv->updatingBounds = false;
            settings->setProperty("plugin_mode", false);

            // Restore the original editor content
            for (auto* child : children) {
                child->setVisible(true);
            }

            // Reset the canvas properties
            cnv->viewport->setBounds(viewportBounds);
            cnv->locked = false;
            cnv->presentationMode = false;

            // Add the canvas to it's original parent component
            cnvParent->addAndMakeVisible(cnv);

            // Restore the editor's resize limits
            if (ProjectInfo::isStandalone) {
                mainWindow->getMaximiseButton()->setVisible(true);
                mainWindow->setResizeLimits(windowConstrainer.getMinimumWidth(), windowConstrainer.getMinimumHeight(), windowConstrainer.getMaximumWidth(), windowConstrainer.getMaximumHeight());
                mainWindow->setSize(windowBounds.getWidth(), windowBounds.getHeight());
            }

            editor->setResizeLimits(windowConstrainer.getMinimumWidth(), windowConstrainer.getMinimumHeight(), windowConstrainer.getMaximumWidth(), windowConstrainer.getMaximumHeight());
            editor->setSize(windowBounds.getWidth(), windowBounds.getHeight());

            // Destroy this view
            editor->pluginMode.reset(nullptr);
        }
    }

    void paint(Graphics& g) override
    {

        auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);
        if (editor->wantsRoundedCorners()) {
            // TitleBar background
            g.setColour(baseColour);
            g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), titlebarHeight, Corners::windowCornerRadius);
        } else {
            // TitleBar background
            g.setColour(baseColour);
            g.fillRect(0, 0, getWidth(), titlebarHeight);
        }

        // TitleBar outline
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.0f, titlebarHeight, static_cast<float>(getWidth()), titlebarHeight, 1.0f);

        // TitleBar text
        g.setColour(findColour(PlugDataColour::panelTextColourId));
        g.drawText(cnv->patch.getTitle().trimCharactersAtEnd(".pd"), titleBar.getBounds(), Justification::centred);
    }

    void parentSizeChanged() override
    {
        float editorWidth = editor->getLocalBounds().getWidth();

        float scale = editorWidth / float(width);

        editor->setSize(editorWidth, (editorWidth + titlebarHeight) * resizeRatio);

        setTransform(getTransform().scale(scale));
    }

    void mouseDrag(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Drag window by TitleBar
        if (!mainWindow->isUsingNativeTitleBar())
            windowDragger.dragComponent(mainWindow, e.getEventRelativeTo(mainWindow), nullptr);
    }

    bool keyPressed(KeyPress const& key) override
    {
        // Block keypresses to editor
        return true;
    }

private:
    Canvas* cnv;
    PluginEditor* editor;
    DocumentWindow* mainWindow = nullptr;
    SettingsFile* settings;

    Component titleBar;
    int const titlebarHeight = 40;
    std::unique_ptr<TextButton> closeButton;

    Component content;

    ComponentDragger windowDragger;
    ComponentBoundsConstrainer windowConstrainer;

    Component* cnvParent;

    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    int const width = cnv->patchWidth.getValue();
    int const height = cnv->patchHeight.getValue();
    float const resizeRatio = float(height) / float(width);

    bool infiniteCanvas;

    std::vector<Component*> children;
};