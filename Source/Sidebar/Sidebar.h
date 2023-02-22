/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

#include "LookAndFeel.h"

struct Console;
struct Inspector;
struct DocumentBrowser;
struct AutomationPanel;
struct SearchPanel;
struct PluginProcessor;

namespace pd {
struct Instance;
}

enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool,
    tCombo,
    tRange
};

enum ParameterCategory {
    cGeneral,
    cAppearance,
    cLabel,
    cExtra
};

using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, std::vector<String>>; // name, type and pointer to value, list of items only for combobox and bool

using ObjectParameters = std::vector<ObjectParameter>; // List of elements and update function

class Sidebar : public Component {

public:
    explicit Sidebar(PluginProcessor* instance, PluginEditor* parent);

    ~Sidebar() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;

    void mouseDown(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;

    void showParameters(String const& name, ObjectParameters& params);
    void showParameters();
    void hideParameters();

    bool isShowingBrowser();

    void showPanel(int panelToShow);

    bool isShowingConsole() const;
    void showSidebar(bool show);

    void pinSidebar(bool pin);
    bool isPinned();

    void clearConsole();
    void updateConsole();

    void tabChanged();

    void updateAutomationParameters();

    static constexpr int dragbarWidth = 5;

private:
    PluginProcessor* pd;
    ObjectParameters lastParameters;

    TextButton consoleButton = TextButton(Icons::Console);
    TextButton browserButton = TextButton(Icons::Documentation);
    TextButton automationButton = TextButton(Icons::Parameters);
    TextButton searchButton = TextButton(Icons::Search);

    Console* console;
    Inspector* inspector;
    DocumentBrowser* browser;
    AutomationPanel* automationPanel;
    SearchPanel* searchPanel;

    int currentPanel = 0;

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    bool pinned = false;

    int lastWidth = 250;
};
