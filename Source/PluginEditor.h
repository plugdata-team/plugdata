/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include <map>

#include "Utility/ModifierKeyListener.h"
#include "Components/CheckedTooltip.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/WindowDragger.h"
#include "Canvas.h"
#include "Components/Buttons.h"
#include "Components/SearchEditor.h"
#include "Sidebar/Sidebar.h"
#include "TabComponent.h"

#include "Utility/ObjectThemeManager.h"
#include "NVGSurface.h"

class ConnectionMessageDisplay;
class Sidebar;
class Statusbar;
class AudioToolbar;
class Dialog;
class Canvas;
class TabComponent;
class PluginProcessor;
class Autosave;
class PluginMode;
class TouchSelectionHelper;
class WelcomePanel;
class CalloutArea;
class NVGGraphicsContext;
class ConsoleMessageDisplay;
class Console;
class DocumentationBrowser;
class AutomationPanel;
class SearchPanel;
class Palettes;
class Inspector;
class CommandInput;

class PluginEditor final : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
    , public ModifierKeyListener
    , public DragAndDropContainer
    , public AsyncUpdater
    , public Timer
    , public SettingsFileListener {
public:
    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void renderArea(NVGcontext* nvg, Rectangle<int> area);

    bool isActiveWindow() override;

    void resized() override;
    void parentSizeChanged() override;
    void parentHierarchyChanged() override;
    void broughtToFront() override;

    void timerCallback() override;

    void lookAndFeelChanged() override;

    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void handleTouchGesture();

    void showWelcomePanel(bool shouldShow);

    void quit(bool askToSave);

    SmallArray<Canvas*> getCanvases();
    Canvas* getCurrentCanvas();

    float getRenderScale() const;

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;
    void settingsChanged(String const& name, var const& value) override;

    void updateCommandStatus();
    void handleAsyncUpdate() override;

    void updateSelection(Canvas* cnv);
    void setCommandButtonObject(Object const* obj);

    void installPackage(File const& file);

    void updateConsole(SmallString const& message, bool isWarning, int numMessages, bool newWarning);

    bool isInterestedInFileDrag(StringArray const& files) override;
    void filesDropped(StringArray const& files, int x, int y) override;
    void fileDragEnter(StringArray const&, int, int) override;
    void fileDragMove(StringArray const& files, int x, int y) override;
    void fileDragExit(StringArray const&) override;
    void dragOperationEnded(DragAndDropTarget::SourceDetails const& details) override;

    TabComponent& getTabComponent();

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(InvocationInfo const& info) override;

    bool wantsRoundedCorners() const;
    bool usesFloatingPanels() const;

    bool keyPressed(KeyPress const& key) override;

    CallOutBox& showCalloutBox(std::unique_ptr<Component> content, Rectangle<int> screenBounds);

    static void updateIoletGeometryForAllObjects(PluginProcessor const* pd);

    void commandKeyChanged(bool isHeld) override;
    void setUseBorderResizer(bool shouldUse);

    Sidebar* getLeftSidebar() const { return leftSidebar.get(); }
    Sidebar* getRightSidebar() const { return rightSidebar.get(); }

    Sidebar* getSidebarForPanel(Sidebar::SidePanel panel) const;

    void movePanelToSide(Sidebar::SidePanel panel, Sidebar::Side targetSide);

    void showCalloutArea(bool shouldBeVisible);
    Component* getCalloutAreaComponent();

    Object* highlightSearchTarget(void* target, bool openNewTabIfNeeded);

    SmallArray<pd::WeakReference> openTextEditors;

    PluginProcessor* pd;

    std::unique_ptr<ConnectionMessageDisplay> connectionMessageDisplay;

    // Sidebar panels
    std::unique_ptr<Console> consolePanel;
    std::unique_ptr<DocumentationBrowser> browserPanel;
    std::unique_ptr<AutomationPanel> automationPanel;
    std::unique_ptr<SearchPanel> searchPanel;
    std::unique_ptr<Palettes> palettePanel;
    std::unique_ptr<Inspector> inspectorPanel;
    std::unique_ptr<CommandInput> commandInput;

    std::unique_ptr<Sidebar> leftSidebar;
    std::unique_ptr<Sidebar> rightSidebar;

    std::unique_ptr<Statusbar> statusbar;
    std::unique_ptr<AudioToolbar> audioToolbar;

    Value theme;
    Value autoconnect;

    NVGSurface nvgSurface;

    std::unique_ptr<Dialog> openedDialog;

    ComponentBoundsConstrainer constrainer;
    ComponentBoundsConstrainer& pluginConstrainer;

    ApplicationCommandManager commandManager;

    std::unique_ptr<WelcomePanel> welcomePanel;

    CheckedTooltip tooltipWindow;

    int editorIndex;

    bool isInPluginMode() const;

    // Return the canvas currently in plugin mode, otherwise return nullptr
    Canvas* getPluginModeCanvas() const;

    NVGGraphicsContext* getNanoLLGC()
    {
        return nvgCtx.get();
    }

private:
    TabComponent tabComponent;

public:
    std::unique_ptr<PluginMode> pluginMode;
    std::unique_ptr<ConsoleMessageDisplay> consoleMessageDisplay;

private:
    std::unique_ptr<TouchSelectionHelper> touchSelectionHelper;

    // Used by standalone to handle dragging the window
    WindowDragger windowDragger;

    int const toolbarHeight = 32;

    MainToolbarButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, welcomePanelSearchButton, sidebarToggleButton;
    SettingsToolbarButton recentlyOpenedPanelSelector, libraryPanelSelector;

    SearchEditor welcomePanelSearchInput;

#if JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    static inline int numEditors = 0;

    std::unique_ptr<CalloutArea> calloutArea;

    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    std::unique_ptr<NVGGraphicsContext> nvgCtx;

    OSUtils::KeyboardLayout keyboardLayout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
