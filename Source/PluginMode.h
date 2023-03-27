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
        windowConstrainer = { c->getMinimumWidth(), c->getMinimumHeight(), c->getMaximumWidth(), c->getMaximumHeight() };

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

        settings->setProperty("plugin_mode", false);

        cnv->updatingBounds = false;

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

        // Restore the editor's size & resize limits with current position
        auto _mainWindow = std::make_shared<DocumentWindow*>(mainWindow);
        auto _editor = std::make_shared<PluginEditor*>(editor);
        auto _windowConstrainer = windowConstrainer;
        auto newBounds = ProjectInfo::isStandalone ? windowBounds.withPosition(mainWindow->getPosition()) : windowBounds.withPosition(editor->getPosition());

        MessageManager::callAsync([this, _mainWindow, _editor, _windowConstrainer, newBounds]() mutable {
            if (ProjectInfo::isStandalone) {
                auto* w = *_mainWindow;
                w->setResizeLimits(_windowConstrainer[0], _windowConstrainer[1], _windowConstrainer[2], _windowConstrainer[3]);
                w->setBounds(newBounds);
            }
            auto* e = *_editor;
            e->setResizeLimits(_windowConstrainer[0], _windowConstrainer[1], _windowConstrainer[2], _windowConstrainer[3]);
            e->setBounds(newBounds);
        });
    }

    void buttonClicked(Button* button) override
    {
        if (button == closeButton.get()) {
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
        float const editorWidth = editor->getLocalBounds().getWidth();

        float const scale = editorWidth / width;

        editor->setSize(editorWidth, (editorWidth * resizeRatio) + (titlebarHeight * scale));

        setTransform(getTransform().scale(scale));
    }

    void mouseDown(MouseEvent const& e) override
    {
        // No window dragging by TitleBar in plugin!
        if (!ProjectInfo::isStandalone)
            return;

        // Offset the start of the drag when dragging the window by Titlebar
        if (!mainWindow->isUsingNativeTitleBar()) {
            if (e.getPosition().getY() < titlebarHeight)
                windowDragger.startDraggingComponent(mainWindow, e.getEventRelativeTo(mainWindow));
        }
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
    std::vector<int> windowConstrainer;

    Component* cnvParent;

    Rectangle<int> windowBounds;
    Rectangle<int> viewportBounds;
    float const width = cnv->patchWidth.getValue();
    float const height = cnv->patchHeight.getValue();
    float const resizeRatio = height / width;

    bool infiniteCanvas;

    std::vector<Component*> children;
};