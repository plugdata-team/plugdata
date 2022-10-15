/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

struct Console;
struct Inspector;
struct DocumentBrowser;
struct AutomationPanel;
struct PlugDataAudioProcessor;

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

// used by console for a more optimised calculation
static int getNumLines(int width, int stringWidth)
{
    // On startup, width might be zero, this is a large optimisation in that case
    if (width == 0)
        return 0;

    return (stringWidth / (width - 12)) + 1;
}
// Used by text objects for estimating best text height for a set width
static int getNumLines(String const& text, int width, Font font = Font(Font::getDefaultSansSerifFontName(), 13, 0))
{
    int numLines = 1;

    Array<int> glyphs;
    Array<float> xOffsets;
    font.getGlyphPositions(text, glyphs, xOffsets);

    for (int i = 0; i < xOffsets.size(); i++) {
        if ((xOffsets[i] + 12) >= static_cast<float>(width) || text.getCharPointer()[i] == '\n') {
            for (int j = i + 1; j < xOffsets.size(); j++) {
                xOffsets.getReference(j) -= xOffsets[i];
            }
            numLines++;
        }
    }

    return numLines;
}

struct Sidebar : public Component {
    explicit Sidebar(PlugDataAudioProcessor* instance);

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

    void updateConsole();

#if PLUGDATA_STANDALONE
    void updateAutomationParameters();
#endif

    static constexpr int dragbarWidth = 5;

private:
    PlugDataAudioProcessor* pd;
    ObjectParameters lastParameters;
    
    TextButton browserButton = TextButton(Icons::Documentation);
    TextButton automationButton = TextButton(Icons::Parameters);
    TextButton consoleButton = TextButton(Icons::Console);
    
    Console* console;
    Inspector* inspector;
    DocumentBrowser* browser;
    AutomationPanel* automationPanel;
    

    int dragStartWidth = 0;
    bool draggingSidebar = false;
    bool sidebarHidden = false;

    bool pinned = false;

    int lastWidth = 250;
};
