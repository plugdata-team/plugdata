/*
 // Copyright (c) 2023 Alex Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Dialogs.h"
#include "Components/BouncingViewport.h"
#include "Components/ObjectDragAndDrop.h"
#include "Components/SearchEditor.h"

#define DEBUG_PRINT_OBJECT_LIST 0

// Replaces placeholders in a stored patch string with values that can only be resolved at drop time
static String resolveObjectPatch(PluginEditor* editor, String patch)
{
    if (patch.contains("@arrName")) {
        editor->pd->setThis();
        patch = patch.replace("@arrName", String::fromUTF8(pd::Interface::getUnusedArrayName()->s_name));
    }

    return patch;
}

class ObjectItem final : public Component
    , public SettableTooltipClient {
public:
    ObjectItem(PluginEditor* e, String const& text, String const& icon, String const& tooltip, String const& patch, ObjectIDs const objectID, std::function<void(bool)> const& dismissCalloutBox)
        : titleText(text)
        , iconText(icon)
        , objectPatch(patch)
        , dismissMenu(dismissCalloutBox)
        , editor(e)
    {
        setTooltip(tooltip.replace("(@keypress) ", getKeyboardShortcutDescription(objectID)));
    }

    String getKeyboardShortcutDescription(ObjectIDs const objectID) const
    {
        auto keyPresses = editor->commandManager.getKeyMappings()->getKeyPressesAssignedToCommand(objectID);
        if (keyPresses.size()) {
            return "(" + keyPresses.getReference(0).getTextDescription() + ") ";
        }

        return { };
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        auto const highlight = colours.popupMenuActiveBackgroundColour;

        auto const iconBounds = getLocalBounds().reduced(14).translated(0, -7);
        auto const textBounds = getLocalBounds().removeFromBottom(14);

        if (isHovering) {
            g.setColour(highlight);
            g.fillRoundedRectangle(iconBounds.toFloat(), Corners::defaultCornerRadius);
        }

        Fonts::drawText(g, titleText, textBounds, colours.popupMenuTextColour, 13.0f, Justification::centred);
        Fonts::drawIcon(g, iconText, iconBounds.reduced(2), colours.popupMenuTextColour, 30);
    }

    bool hitTest(int const x, int const y) override
    {
        return getLocalBounds().reduced(16).translated(0, -7).contains(x, y);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

    String getPatchString()
    {
        return resolveObjectPatch(editor, objectPatch);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.getDistanceFromDragStart() > 3) {
            ObjectDragAndDrop::attachToMouse(editor, getPatchString());
            dismissMenu(true);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) {
            dismissMenu(false);
        } else {
            if (!SettingsFile::getInstance()->isUsingTouchMode()) {
                ObjectDragAndDrop::attachToMouse(editor, getPatchString());
                dismissMenu(false);
            }
        }
    }

private:
    String titleText;
    String iconText;
    String objectPatch;
    bool isHovering = false;
    std::function<void(bool)> dismissMenu;
    PluginEditor* editor;
};

class ObjectSearchItem final : public Component {
public:
    ObjectSearchItem(PluginEditor* e, String const& text, String const& icon, String const& description, String const& patch, std::function<void(bool)> const& dismissCalloutBox)
        : titleText(text)
        , iconText(icon)
        , descriptionText(description)
        , objectPatch(patch)
        , dismissMenu(dismissCalloutBox)
        , editor(e)
    {
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        auto bounds = getLocalBounds();

        if (isHovering) {
            g.setColour(colours.popupMenuActiveBackgroundColour);
            g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);
        }

        auto const colour = colours.popupMenuTextColour;
        Fonts::drawIcon(g, iconText, bounds.removeFromLeft(30).reduced(4), colour, 17);

        auto const titleBounds = bounds.removeFromLeft(Fonts::getStringWidthInt(titleText, 14.0f) + 12);
        Fonts::drawStyledText(g, titleText, titleBounds, colour, Semibold, 14);
        Fonts::drawFittedText(g, descriptionText, bounds, colour.withAlpha(0.6f), 1, 1.0f, 14);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (e.getDistanceFromDragStart() > 3) {
            ObjectDragAndDrop::attachToMouse(editor, resolveObjectPatch(editor, objectPatch));
            dismissMenu(true);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) {
            dismissMenu(false);
        } else if (!SettingsFile::getInstance()->isUsingTouchMode()) {
            ObjectDragAndDrop::attachToMouse(editor, resolveObjectPatch(editor, objectPatch));
            dismissMenu(false);
        }
    }

private:
    String titleText;
    String iconText;
    String descriptionText;
    String objectPatch;
    bool isHovering = false;
    std::function<void(bool)> dismissMenu;
    PluginEditor* editor;
};

class ObjectList final : public Component {

    struct Section {
        explicit Section(String const& sectionName, bool const grid = true)
            : name(sectionName)
            , isGrid(grid)
        {
        }

        String name;
        bool isGrid;
        Rectangle<int> headerBounds;
        OwnedArray<Component> items;
    };

public:
    ObjectList(PluginEditor* e, std::function<void(bool)> const& dismissCalloutBox)
        : editor(e)
        , dismissMenu(dismissCalloutBox)
    {
#if DEBUG_PRINT_OBJECT_LIST == 1
        printAllObjects();
#endif
    }

    static auto const& getObjectsToShow()
    {
        return getValue<bool>(SettingsFile::getInstance()->getPropertyAsValue("hvcc_mode")) ? heavyObjectList : defaultObjectList;
    }

    void resized() override
    {
        static constexpr int itemSize = 64;
        
        int const maxColumns = std::max(1, (getWidth() - margin * 2) / itemSize);
        int y = margin;

        for (auto* section : sections) {
            section->headerBounds = { margin + 4, y, getWidth() - margin * 2, section->name.isEmpty() ? 0 : headerHeight };
            y += section->headerBounds.getHeight();

            if (section->isGrid) {
                int column = 0;
                for (auto* item : section->items) {
                    item->setBounds(margin + column * itemSize, y, itemSize, itemSize);
                    if (++column >= maxColumns) {
                        column = 0;
                        y += itemSize;
                    }
                }
                if (column)
                    y += itemSize;
            } else {
                for (auto* item : section->items) {
                    item->setBounds(margin, y, getWidth() - margin * 2, rowHeight);
                    y += rowHeight;
                }
            }
            y += margin * 2;
        }

        // The height depends on our width, so we need to update it here
        if (y != getHeight())
            setSize(getWidth(), y);
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        for (auto const* section : sections) {
            if (section->name.isEmpty())
                continue;

            if (section != sections.getFirst()) {
                g.setColour(colours.outlineColour);
                g.drawHorizontalLine(section->headerBounds.getY() - margin, margin + 4, getWidth() - margin - 4);
            }
            Fonts::drawStyledText(g, section->name, section->headerBounds, colours.popupMenuTextColour, Semibold, 15);
        }
    }

    void showAllCategories()
    {
        sections.clear();

        for (auto const& [categoryName, objectCategory] : getObjectsToShow()) {
            auto* section = sections.add(new Section(categoryName));
            for (auto const& [icon, patch, tooltip, name, objectID] : objectCategory) {
                addObjectItem(section, icon, patch, tooltip, name, objectID);
            }
        }

        resized();
    }

    // Objects from the menu itself come first, followed by a full search through the object library
    void showSearchResults(String const& query)
    {
        sections.clear();

        auto* section = sections.add(new Section({ }, false));
        auto& library = *editor->pd->objectLibrary;
        StringArray menuObjects;

        for (bool const matchPrefix : { true, false }) {
            for (auto const& [categoryName, objectCategory] : getObjectsToShow()) {
                for (auto const& [icon, patch, tooltip, name, objectID] : objectCategory) {
                    auto const objectPatch = getObjectPatch(patch, name);
                    auto const objectName = getObjectNameFromPatch(objectPatch);

                    // Apart from the icon, these behave like any other search result: the object's
                    // own name, and no creation arguments when dropped. Atom boxes, comments and
                    // arrays have no object name, so they keep their menu entry as-is.
                    auto const title = objectName.isNotEmpty() ? objectName : name;

                    if (title.startsWithIgnoreCase(query) != matchPrefix)
                        continue;
                    if (!matchPrefix && !(title + " " + name + " " + tooltip).containsIgnoreCase(query))
                        continue;

                    menuObjects.add(objectName);

                    // Objects that aren't in the documentation (atom boxes, arrays, ...) fall back to their tooltip
                    auto description = library.getObjectInfo(objectName).description;
                    if (description.isEmpty())
                        description = tooltip.replace("(@keypress) ", "");

                    addItem(section, new ObjectSearchItem(editor, title, icon,
                        description.equalsIgnoreCase(title) ? String() : description,
                        objectName.isNotEmpty() ? "#X obj 0 0 " + objectName : objectPatch, dismissMenu));
                }
            }
        }

        for (auto const& object : library.searchObjectDocumentation(query)) {
            if (section->items.size() >= maxSearchResults)
                break;
            if (menuObjects.contains(object))
                continue;

            addItem(section, new ObjectSearchItem(editor, object, Icons::GlyphEmptyObject, library.getObjectInfo(object).description, ObjectThemeManager::get()->getCompleteFormat(object), dismissMenu));
        }

        resized();
    }

    int getCategoryY(String const& category) const
    {
        for (auto const* section : sections) {
            if (section->name == category)
                return section->headerBounds.getY() - margin;
        }

        return 0;
    }

    String getCategoryAt(int const y) const
    {
        String category;
        for (auto const* section : sections) {
            if (section->headerBounds.getY() > y + headerHeight)
                break;
            category = section->name;
        }

        return category;
    }

    String getLastCategory() const
    {
        return sections.isEmpty() ? String() : sections.getLast()->name;
    }


    static void printAllObjects()
    {
        static bool hasRun = false;
        if (hasRun)
            return;

        hasRun = true;

        std::cout << "==== object icon list in CSV format ====" << std::endl;

        for (auto& [categoryName, objectCategory] : defaultObjectList) {
            String cat = categoryName;
            for (auto& [icon, patch, tooltip, name, objectID] : objectCategory) {
                std::cout << cat << ", " << name << ", " << icon << std::endl;
            }
        }

        std::cout << "==== end of list ====" << std::endl;
    }

    static inline HeapArray<std::pair<String, HeapArray<std::tuple<String, String, String, String, ObjectIDs>>>> const defaultObjectList = {
        { "Essentials",
            {
                { Icons::GlyphEmptyObject, "#X obj 0 0", "(@keypress) Empty object", "Object", NewObject },
                { Icons::GlyphMessage, "#X msg 0 0", "(@keypress) Message", "Message", NewMessage },
                { Icons::GlyphFloatBox, "#X floatatom 0 0 5 0 0 0 - - - 0", "(@keypress) Float box", "Float", NewFloatAtom },
                { Icons::GlyphSymbolBox, "#X symbolatom 0 0 10 0 0 0 - - - 0", "Symbol box", "Symbol", NewSymbolAtom },
                { Icons::GlyphListBox, "#X listbox 0 0 20 0 0 0 - - - 0", "(@keypress) List box", "List", NewListAtom },
                { Icons::GlyphComment, "#X text 0 0 comment", "(@keypress) Comment", "Comment", NewComment },
                { Icons::GlyphArray, "#N canvas 0 0 450 250 (subpatch) 0;\n#X array @arrName 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(@keypress) Array", "Array", NewArray },
                { Icons::GlyphGOP, "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 0 0 graph;", "(@keypress) Graph on parent", "Graph", NewGraphOnParent },
                { Icons::GlyphSubpatch, "#X obj 0 0 pd", "Subpatch", "Subpatch", OtherObject },
                { Icons::GlyphInlet, "#X obj 0 0 inlet", "Control inlet", "Inlet", OtherObject },
                { Icons::GlyphOutlet, "#X obj 0 0 outlet", "Control outlet", "Outlet", OtherObject },
                { Icons::GlyphSignalInlet, "#X obj 0 0 inlet~", "Signal inlet", "Inlet~", OtherObject },
                { Icons::GlyphSignalOutlet, "#X obj 0 0 outlet~", "Signal outlet", "Outlet~", OtherObject },
                { Icons::GlyphClone, "#X obj 0 0 clone", "Multiple copies of an abstraction", "Clone", OtherObject },
                { Icons::GlyphBlock, "#X obj 0 0 block~ 1024", "Set block size for DSP", "Block", OtherObject },
                { Icons::GlyphSwitch, "#X obj 0 0 switch~", "Block size and DSP on/off control", "Switch", OtherObject }
            } },
        { "User Interface",
            {
                { Icons::GlyphBang, "bng", "(@keypress) Bang", "Bang", NewBang },
                { Icons::GlyphToggle, "tgl", "(@keypress) Toggle", "Toggle", NewToggle },
                { Icons::GlyphButton, "button", "Button", "Button", OtherObject },
                { Icons::GlyphKnob, "knob", "Knob", "Knob", OtherObject },
                { Icons::GlyphVSlider, "vsl", "(@keypress) Vertical slider", "V. Slider", NewVerticalSlider },
                { Icons::GlyphHSlider, "hsl", "(@keypress) Horizontal slider", "H. Slider", NewHorizontalSlider },
                { Icons::GlyphVRadio, "vradio", "(@keypress) Vertical radio box", "V. Radio", NewVerticalRadio },
                { Icons::GlyphHRadio, "hradio", "(@keypress) Horizontal radio box", "H. Radio", NewHorizontalRadio },
                { Icons::GlyphNumber, "nbx", "(@keypress) Number box", "Number", NewNumbox },
                { Icons::GlyphCanvas, "cnv", "(@keypress) Canvas", "Canvas", NewCanvas },
                { Icons::GlyphFunction, "function", "Function", "Function", OtherObject },
                { Icons::GlyphOscilloscope, "scope~", "Oscilloscope", "Scope", OtherObject },
                { Icons::GlyphKeyboard, "keyboard", "Piano keyboard", "Keyboard", OtherObject },
                { Icons::GlyphMessbox, "messbox", "ELSE Message box", "Messbox", OtherObject },
                { Icons::GlyphBicoeff, "#X obj 0 0 bicoeff 450 150 peaking", "Bicoeff generator", "Bicoeff", OtherObject },
                { Icons::GlyphVUMeter, "vu", "(@keypress) VU meter", "VU Meter", NewVUMeter },
                { Icons::GlyphCircleSlider, "#X obj 0 0 circle", "Circular slider", "Circle", OtherObject },
                { Icons::GlyphIncdec, "#X obj 0 0 incdec", "Increment and decrement buttons", "Incdec", OtherObject },
                { Icons::GlyphTabSelect, "#X obj 0 0 tab", "Tab selector", "Tab", OtherObject },
                { Icons::GlyphGuiCanvas, "#X obj 0 0 guicanvas", "Canvas GUI", "GUI Canvas", OtherObject },
                { Icons::GlyphSlider2D, "#X obj 0 0 slider2d", "Two-dimensional slider", "Slider 2D", OtherObject },
                { Icons::GlyphMousePad, "#X obj 0 0 pad", "Mouse pad", "Pad", OtherObject },
                { Icons::GlyphMultiSlider, "#X obj 0 0 multi.vsl", "Multi vertical slider", "Multi Sldr", OtherObject },
                { Icons::GlyphRangeSlider, "#X obj 0 0 range.hsl", "Range horizontal slider", "Range Sldr", OtherObject },
                { Icons::GlyphMatrixCtl, "#X obj 0 0 mtx.ctl", "Matrix control GUI", "Matrix", OtherObject },
                { Icons::GlyphDrumSeq, "#X obj 0 0 drum.seq", "Drum sequence pattern GUI", "Drum Seq", OtherObject },
                { Icons::GlyphPopmenu, "#X obj 0 0 popmenu", "Popup menu", "Popmenu", OtherObject },
                { Icons::GlyphDisplay, "#X obj 0 0 display", "Display messages", "Display", OtherObject },
                { Icons::GlyphTextNote, "#X obj 0 0 note", "Text note", "Note", OtherObject },
                { Icons::GlyphPic, "#X obj 0 0 pic", "Load pictures", "Pic", OtherObject },
                { Icons::GlyphColors, "#X obj 0 0 colors", "Pick and convert colours", "Colors", OtherObject },
                { Icons::GlyphOpenFile, "#X obj 0 0 openfile", "Open folders, files and weblinks", "Openfile", OtherObject },
                { Icons::GlyphBiplot, "#X obj 0 0 biplot", "Biquad plot", "Biplot", OtherObject },
                { Icons::GlyphZBiplot, "#X obj 0 0 zbiplot", "Z-plane biquad plot", "Z-Biplot", OtherObject },
                { Icons::GlyphSignalNumbox, "#X obj 0 0 numbox~", "Signal number box", "Numbox~", OtherObject },
                { Icons::GlyphGainFader, "#X obj 0 0 gain~", "Mono gain", "Gain", OtherObject },
                { Icons::GlyphGainFader2, "#X obj 0 0 gain2~", "Stereo gain", "Gain 2", OtherObject },
                { Icons::GlyphLevel, "#X obj 0 0 level~", "Level adjustment in dB", "Level", OtherObject },
                { Icons::GlyphMeterBar, "#X obj 0 0 meter~", "Mono VU meter", "Meter", OtherObject },
                { Icons::GlyphMeterBar2, "#X obj 0 0 meter2~", "Stereo VU meter", "Meter 2", OtherObject },
                { Icons::GlyphSpectrum, "#X obj 0 0 spectrograph~", "Spectral graph", "Spectrum", OtherObject },
                { Icons::GlyphScope3D, "#X obj 0 0 scope3d~", "3D oscilloscope", "Scope 3D", OtherObject },
                { Icons::GlyphPlaylist, "#X obj 0 0 playlist~", "Sound file playlist", "Playlist", OtherObject }
            } },
        { "General",
            {
                { Icons::GlyphTrigger, "#X obj 0 0 trigger", "Trigger", "Trigger", OtherObject },
                { Icons::GlyphMoses, "#X obj 0 0 moses", "Moses", "Moses", OtherObject },
                { Icons::GlyphSpigot, "#X obj 0 0 spigot", "Spigot", "Spigot", OtherObject },
                { Icons::GlyphBondo, "#X obj 0 0 bondo", "Bondo", "Bondo", OtherObject },
                { Icons::GlyphSelect, "#X obj 0 0 select", "Select", "Select", OtherObject },
                { Icons::GlyphRoute, "#X obj 0 0 route", "Route", "Route", OtherObject },
                { Icons::GlyphExpr, "#X obj 0 0 expr", "Expr", "Expr", OtherObject },
                { Icons::GlyphLoadbang, "#X obj 0 0 loadbang", "Loadbang", "Loadbang", OtherObject },
                { Icons::GlyphPrint, "#X obj 0 0 print", "Print", "Print", OtherObject },
                { Icons::GlyphBangObject, "#X obj 0 0 bang", "Convert any message to a bang", "Bang", OtherObject },
                { Icons::GlyphFloatObject, "#X obj 0 0 float", "Store and recall a float", "Float", OtherObject },
                { Icons::GlyphIntObject, "#X obj 0 0 int", "Store and recall an integer", "Int", OtherObject },
                { Icons::GlyphSymbolObject, "#X obj 0 0 symbol", "Store and recall a symbol", "Symbol", OtherObject },
                { Icons::GlyphValue, "#X obj 0 0 value", "Share a value between objects", "Value", OtherObject },
                { Icons::GlyphChange, "#X obj 0 0 change", "Remove repeats from a stream", "Change", OtherObject },
                { Icons::GlyphSwap, "#X obj 0 0 swap", "Swap two values", "Swap", OtherObject },
                { Icons::GlyphUntil, "#X obj 0 0 until", "Looping mechanism", "Until", OtherObject },
                { Icons::GlyphChance, "#X obj 0 0 chance", "Weighted random branching", "Chance", OtherObject },
                { Icons::GlyphKeyInput, "#X obj 0 0 key", "Grab keyboard input", "Key", OtherObject },
                { Icons::GlyphMetro, "#X obj 0 0 metro 1 120 permin", "Metro", "Metro", OtherObject },
                { Icons::GlyphCounter, "#X obj 0 0 count 5", "Count", "Count", OtherObject },
                { Icons::GlyphTimer, "#X obj 0 0 timer", "Timer", "Timer", OtherObject },
                { Icons::GlyphDelay, "#X obj 0 0 delay 1 60 permin", "Delay", "Delay", OtherObject },
                { Icons::GlyphPipe, "#X obj 0 0 pipe 100", "Delay line for messages", "Pipe", OtherObject },
                { Icons::GlyphScore, "#X obj 0 0 score", "Score sequencer", "Score", OtherObject },
                { Icons::GlyphSequencer, "#X obj 0 0 sequencer 1 2 3 4", "Data sequencer", "Sequencer", OtherObject },
                { Icons::GlyphEuclid, "#X obj 0 0 euclid 16 5", "Euclidean rhythm algorithm", "Euclid", OtherObject },
                { Icons::GlyphRandom, "#X obj 0 0 random 100", "Pseudo random integers", "Random", OtherObject },
                { Icons::GlyphRandFloat, "#X obj 0 0 rand.f", "Random float generator", "Rand Float", OtherObject },
                { Icons::GlyphRandInt, "#X obj 0 0 rand.i 0 127", "Random integer generator", "Rand Int", OtherObject },
                { Icons::GlyphDrunkard, "#X obj 0 0 drunkard", "Drunkard's walk algorithm", "Drunkard", OtherObject },
                { Icons::GlyphMarkov, "#X obj 0 0 markov", "Create and play Markov chains", "Markov", OtherObject }
            } },
        { "Data",
            {
                { Icons::GlyphPack, "#X obj 0 0 pack", "Pack", "Pack", OtherObject },
                { Icons::GlyphUnpack, "#X obj 0 0 unpack", "Unpack", "Unpack", OtherObject },
                { Icons::GlyphListAppend, "#X obj 0 0 list append", "Append lists", "Append", OtherObject },
                { Icons::GlyphListPrepend, "#X obj 0 0 list prepend", "Prepend lists", "Prepend", OtherObject },
                { Icons::GlyphListStore, "#X obj 0 0 list store", "Store and edit a list", "Store", OtherObject },
                { Icons::GlyphListSplit, "#X obj 0 0 list split 1", "Split a list", "Split", OtherObject },
                { Icons::GlyphListLength, "#X obj 0 0 list length", "Length of a list", "Length", OtherObject },
                { Icons::GlyphTextDefine, "#X obj 0 0 text define", "Store a list of messages", "Text Def", OtherObject },
                { Icons::GlyphTextGet, "#X obj 0 0 text get", "Read a line from a text", "Text Get", OtherObject },
                { Icons::GlyphTextSet, "#X obj 0 0 text set", "Write a line to a text", "Text Set", OtherObject },
                { Icons::GlyphTextSeq, "#X obj 0 0 text sequence", "Sequence a text", "Text Seq", OtherObject },
                { Icons::GlyphQlist, "#X obj 0 0 qlist", "Text-based sequencer", "Qlist", OtherObject },
                { Icons::GlyphTextfile, "#X obj 0 0 textfile", "Read and write text files", "Textfile", OtherObject },
                { Icons::GlyphFormat, "#X obj 0 0 format", "Format messages", "Format", OtherObject },
                { Icons::GlyphMakeFilename, "#X obj 0 0 makefilename file%d", "Format a symbol with a variable field", "Filename", OtherObject },
                { Icons::GlyphTabread, "#X obj 0 0 tabread", "Read a number from a table", "Tabread", OtherObject },
                { Icons::GlyphTabread4, "#X obj 0 0 tabread4", "4-point interpolating table read", "Tabread4", OtherObject },
                { Icons::GlyphTabwrite, "#X obj 0 0 tabwrite", "Write a number to a table", "Tabwrite", OtherObject },
                { Icons::GlyphSoundfiler, "#X obj 0 0 soundfiler", "Read and write tables as soundfiles", "Soundfiler", OtherObject },
                { Icons::GlyphBuffer, "#X obj 0 0 buffer", "Get and set an array buffer", "Buffer", OtherObject },
                { Icons::GlyphSfload, "#X obj 0 0 sfload", "Load a sound file into an array", "Sfload", OtherObject },
                { Icons::GlyphTabosc, "#X obj 0 0 tabosc4~", "4-point interpolating oscillator", "Tabosc4", OtherObject },
                { Icons::GlyphTabplay, "#X obj 0 0 tabplay~", "Play a table as a sample", "Tabplay", OtherObject },
                { Icons::GlyphSignalTabread, "#X obj 0 0 tabread~", "Non-interpolating table read", "Tabread~", OtherObject },
                { Icons::GlyphSignalTabread4, "#X obj 0 0 tabread4~", "4-point interpolating table read", "Tabread4~", OtherObject },
                { Icons::GlyphSignalTabwrite, "#X obj 0 0 tabwrite~", "Write a signal into an array", "Tabwrite~", OtherObject },
                { Icons::GlyphReadsf, "#X obj 0 0 readsf~ 2", "Read a soundfile from disk", "Readsf", OtherObject },
                { Icons::GlyphWritesf, "#X obj 0 0 writesf~ 2", "Write a soundfile to disk", "Writesf", OtherObject },
                { Icons::GlyphSamplePlayer, "#X obj 0 0 player~", "Multichannel sample player", "Player", OtherObject },
                { Icons::GlyphSfz, "#X obj 0 0 sfz~", "Sfz sample player using sfizz", "Sfz", OtherObject }
            } },
        { "MIDI",
            {
                { Icons::GlyphMidiIn, "#X obj 0 0 midiin", "MIDI in", "MIDI in", OtherObject },
                { Icons::GlyphMidiOut, "#X obj 0 0 midiout", "MIDI out", "MIDI out", OtherObject },
                { Icons::GlyphNoteIn, "#X obj 0 0 notein", "Note in", "Note in", OtherObject },
                { Icons::GlyphNoteOut, "#X obj 0 0 noteout", "Note out", "Note out", OtherObject },
                { Icons::GlyphCtlIn, "#X obj 0 0 ctlin", "Control in", "Ctl in", OtherObject },
                { Icons::GlyphCtlOut, "#X obj 0 0 ctlout", "Control out", "Ctl out", OtherObject },
                { Icons::GlyphPgmIn, "#X obj 0 0 pgmin", "Program in", "Pgm in", OtherObject },
                { Icons::GlyphPgmOut, "#X obj 0 0 pgmout", "Program out", "Pgm out", OtherObject },
                { Icons::GlyphSysexIn, "#X obj 0 0 sysexin", "Sysex in", "Sysex in", OtherObject },
                { Icons::GlyphMtof, "#X obj 0 0 mtof", "MIDI to frequency", "mtof", OtherObject },
                { Icons::GlyphFtom, "#X obj 0 0 ftom", "Frequency to MIDI", "ftom", OtherObject },
                { Icons::GlyphAutotune, "#X obj 0 0 autotune", "Pitch quantizer", "Autotune", OtherObject },
                { Icons::GlyphBendIn, "#X obj 0 0 bendin", "Pitch bend in", "Bend in", OtherObject },
                { Icons::GlyphBendOut, "#X obj 0 0 bendout", "Pitch bend out", "Bend out", OtherObject },
                { Icons::GlyphTouchIn, "#X obj 0 0 touchin", "Aftertouch in", "Touch in", OtherObject },
                { Icons::GlyphTouchOut, "#X obj 0 0 touchout", "Aftertouch out", "Touch out", OtherObject },
                { Icons::GlyphPolyTouchIn, "#X obj 0 0 polytouchin", "Poly aftertouch in", "Ptouch in", OtherObject },
                { Icons::GlyphPolyTouchOut, "#X obj 0 0 polytouchout", "Poly aftertouch out", "Ptouch out", OtherObject },
                { Icons::GlyphMidiRealtime, "#X obj 0 0 midirealtimein", "MIDI realtime messages in", "Realtime", OtherObject },
                { Icons::GlyphMakenote, "#X obj 0 0 makenote 64 250", "Send note-on and schedule note-off", "Makenote", OtherObject },
                { Icons::GlyphStripnote, "#X obj 0 0 stripnote", "Strip note-off messages", "Stripnote", OtherObject },
                { Icons::GlyphPolyVoices, "#X obj 0 0 poly 4 1", "Polyphonic voice allocator", "Poly", OtherObject },
                { Icons::GlyphMidiLearn, "#X obj 0 0 midi.learn", "MIDI learn", "MIDI learn", OtherObject },
                { Icons::GlyphPanic, "#X obj 0 0 panic", "Flush hanging MIDI notes", "Panic", OtherObject }
            } },
        { "Input & Output",
            {
                { Icons::GlyphAdc, "#X obj 0 0 adc~", "Adc", "Adc", OtherObject },
                { Icons::GlyphDac, "#X obj 0 0 dac~", "Dac", "Dac", OtherObject },
                { Icons::GlyphOut, "#X obj 0 0 out~", "Out", "Out", OtherObject },
                { Icons::GlyphBlocksize, "#X obj 0 0 blocksize~", "Blocksize", "Blocksize", OtherObject },
                { Icons::GlyphSamplerate, "#X obj 0 0 samplerate~", "Samplerate", "Samplerate", OtherObject },
                { Icons::GlyphSetDsp, "#X obj 0 0 setdsp~", "Setdsp", "Setdsp", OtherObject },
                { Icons::GlyphSend, "#X obj 0 0 s", "Send", "Send", OtherObject },
                { Icons::GlyphReceive, "#X obj 0 0 r", "Receive", "Receive", OtherObject },
                { Icons::GlyphSignalSend, "#X obj 0 0 s~", "Send~", "Send~", OtherObject },
                { Icons::GlyphSignalReceive, "#X obj 0 0 r~", "Receive~", "Receive~", OtherObject },
                { Icons::GlyphNetsend, "#X obj 0 0 netsend", "Netsend", "Netsend", OtherObject },
                { Icons::GlyphNetreceive, "#X obj 0 0 netreceive", "Netreceive", "Netreceive", OtherObject },
                { Icons::GlyphOSCsend, "#X obj 0 0 osc.send", "OSC send", "OSC send", OtherObject },
                { Icons::GlyphOSCreceive, "#X obj 0 0 osc.receive", "OSC receive", "OSC receive", OtherObject },
                { Icons::GlyphOscParse, "#X obj 0 0 oscparse", "OSC packets to Pd lists", "OSC parse", OtherObject },
                { Icons::GlyphOscFormat, "#X obj 0 0 oscformat", "Pd lists to OSC packets", "OSC format", OtherObject },
                { Icons::GlyphPdlink, "#X obj 0 0 pdlink", "Send messages across a network by name", "Pdlink", OtherObject }
            } },
        { "Signal Utility",
            {
                { Icons::GlyphMultiSnake, "#X obj 0 0 snake~ 2", "Multichannel snake", "Snake", OtherObject },
                { Icons::GlyphMultiGet, "#X obj 0 0 get~", "Multichannel get", "Get", OtherObject },
                { Icons::GlyphMultiPick, "#X obj 0 0 pick~", "Multichannel pick", "Pick", OtherObject },
                { Icons::GlyphMultiMerge, "#X obj 0 0 merge~", "Multichannel merge", "Merge", OtherObject },
                { Icons::GlyphMultiUnmerge, "#X obj 0 0 unmerge~", "Multichannel unmerge", "Unmerge", OtherObject },
                { Icons::GlyphNumChans, "#X obj 0 0 nchs~", "Number of channels in a connection", "Num Chans", OtherObject },
                { Icons::GlyphMixChans, "#X obj 0 0 mix~", "Mix multichannel signals", "Mix", OtherObject },
                { Icons::GlyphSumChans, "#X obj 0 0 sum~", "Sum channels into one", "Sum", OtherObject },
                { Icons::GlyphSliceChans, "#X obj 0 0 slice~ 1", "Split a multichannel signal", "Slice", OtherObject },
                { Icons::GlyphRepeatChans, "#X obj 0 0 repeat~ 4", "Copy a signal to multiple channels", "Repeat", OtherObject },
                { Icons::GlyphGroupChans, "#X obj 0 0 group~ 2", "Group channels", "Group", OtherObject },
                { Icons::GlyphSelectChans, "#X obj 0 0 select~", "Select inputs", "Select~", OtherObject },
                { Icons::GlyphLaceChans, "#X obj 0 0 lace~", "Interleave multichannel signals", "Lace", OtherObject },
                { Icons::GlyphDelaceChans, "#X obj 0 0 delace~", "Deinterleave a multichannel signal", "Delace", OtherObject },
                { Icons::GlyphOutMc, "#X obj 0 0 out.mc~", "Multichannel output", "Out MC", OtherObject },
                { Icons::GlyphSigConv, "#X obj 0 0 sig~", "Convert numbers to a signal", "Sig", OtherObject },
                { Icons::GlyphSnapshot, "#X obj 0 0 snapshot~", "Convert a signal to a number", "Snapshot", OtherObject },
                { Icons::GlyphThrow, "#X obj 0 0 throw~", "Throw a signal to a catch~", "Throw", OtherObject },
                { Icons::GlyphCatch, "#X obj 0 0 catch~", "Catch signals from throw~", "Catch", OtherObject },
                { Icons::GlyphSignalPrint, "#X obj 0 0 print~", "Print raw signal values", "Print~", OtherObject },
                { Icons::GlyphEnvFollow, "#X obj 0 0 env~", "Envelope follower", "Env", OtherObject },
                { Icons::GlyphRms, "#X obj 0 0 rms~", "Detect RMS amplitude", "RMS", OtherObject },
                { Icons::GlyphPeakDetect, "#X obj 0 0 peak~", "Detect peak amplitude", "Peak", OtherObject },
                { Icons::GlyphZerocross, "#X obj 0 0 zerocross~", "Impulses at zero crossings", "Zerocross", OtherObject },
                { Icons::GlyphSigmund, "#X obj 0 0 sigmund~", "Sinusoidal analysis and pitch tracking", "Sigmund", OtherObject },
                { Icons::GlyphBonk, "#X obj 0 0 bonk~", "Attack detection", "Bonk", OtherObject }
            } },
        { "Sources",
            {
                { Icons::GlyphPhasor, "#X obj 0 0 phasor~", "Phasor", "Phasor", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 osc~ 440", "Osc", "Osc", OtherObject },
                { Icons::GlyphOscBL, "#X obj 0 0 bl.osc~ 440", "Osc band limited", "Bl. Osc", OtherObject },
                { Icons::GlyphTriangle, "#X obj 0 0 tri~ 440", "Triangle", "Triangle", OtherObject },
                { Icons::GlyphTriBL, "#X obj 0 0 bl.tri~ 100", "Triangle band limited", "Bl. Tri", OtherObject },
                { Icons::GlyphSquare, "#X obj 0 0 square~", "Square", "Square", OtherObject },
                { Icons::GlyphSquareBL, "#X obj 0 0 bl.square~ 440", "Square band limited", "Bl. Square", OtherObject },
                { Icons::GlyphSaw, "#X obj 0 0 saw~ 440", "Saw", "Saw", OtherObject },
                { Icons::GlyphSawBL, "#X obj 0 0 bl.saw~ 440", "Saw band limited", "Bl. Saw", OtherObject },
                { Icons::GlyphImp, "#X obj 0 0 imp~ 100", "Impulse", "Impulse", OtherObject },
                { Icons::GlyphImpBL, "#X obj 0 0 bl.imp~ 100", "Impulse band limited", "Bl. Imp", OtherObject },
                { Icons::GlyphWavetable, "#X obj 0 0 wavetable~", "Wavetable", "Wavetab", OtherObject },
                { Icons::GlyphWavetableBL, "#X obj 0 0 bl.wavetable~", "Wavetable band limited", "Bl. Wavetab", OtherObject },
                { Icons::GlyphPlaits, "#X obj 0 0 plaits~", "Plaits", "Plaits", OtherObject },
                { Icons::GlyphCosine, "#X obj 0 0 cos~", "Cosine oscillator and waveshaper", "Cos", OtherObject },
                { Icons::GlyphSine, "#X obj 0 0 sine~ 440", "Sine oscillator", "Sine", OtherObject },
                { Icons::GlyphPulseOsc, "#X obj 0 0 pulse~ 440 0.5", "Pulse train oscillator", "Pulse", OtherObject },
                { Icons::GlyphVSaw, "#X obj 0 0 vsaw~ 440 0.5", "Variable sawtooth-triangle oscillator", "V. Saw", OtherObject },
                { Icons::GlyphBlip, "#X obj 0 0 blip~ 440 10", "Band-limited cosine oscillator", "Blip", OtherObject },
                { Icons::GlyphFm, "#X obj 0 0 fm~ 440 2 1", "Frequency modulation unit", "FM", OtherObject },
                { Icons::GlyphPm, "#X obj 0 0 pm~ 440 1 1", "Phase modulation unit", "PM", OtherObject },
                { Icons::GlyphWavetable2D, "#X obj 0 0 wt2d~", "Two-dimensional wavetable oscillator", "Wavetab 2D", OtherObject },
                { Icons::GlyphOscBank, "#X obj 0 0 oscbank~", "Bank of oscillators", "Osc Bank", OtherObject },
                { Icons::GlyphLfo, "#X obj 0 0 lfo 1", "Control rate LFO", "LFO", OtherObject },
                { Icons::GlyphNoise, "#X obj 0 0 noise~", "White noise", "Noise", OtherObject },
                { Icons::GlyphPinkNoise, "#X obj 0 0 pink~", "Pink noise", "Pink", OtherObject },
                { Icons::GlyphBrownNoise, "#X obj 0 0 brown~", "Brown noise", "Brown", OtherObject },
                { Icons::GlyphCrackle, "#X obj 0 0 crackle~", "Crackle noise", "Crackle", OtherObject },
                { Icons::GlyphDust, "#X obj 0 0 dust~ 10", "Random impulses", "Dust", OtherObject },
                { Icons::GlyphLfNoise, "#X obj 0 0 lfnoise~ 10", "Low frequency noise", "LF Noise", OtherObject },
                { Icons::GlyphStepNoise, "#X obj 0 0 stepnoise~ 10", "Step noise", "Step Noise", OtherObject },
                { Icons::GlyphRampNoise, "#X obj 0 0 rampnoise~ 10", "Ramp noise", "Ramp Noise", OtherObject }
            } },
        { "Envelopes",
            {
                { Icons::GlyphLineSignal, "#X obj 0 0 line~", "Audio ramp generator", "Line~", OtherObject },
                { Icons::GlyphVline, "#X obj 0 0 vline~", "High-precision audio ramp generator", "VLine", OtherObject },
                { Icons::GlyphLineCtl, "#X obj 0 0 line", "Series of linearly stepped numbers", "Line", OtherObject },
                { Icons::GlyphAdsr, "#X obj 0 0 adsr~ 10 100 0.5 500", "Attack, decay, sustain, release envelope", "ADSR", OtherObject },
                { Icons::GlyphAsr, "#X obj 0 0 asr~ 10 500", "Attack, sustain, release envelope", "ASR", OtherObject },
                { Icons::GlyphDecayEnv, "#X obj 0 0 decay~", "Exponential decay", "Decay", OtherObject },
                { Icons::GlyphEnvGen, "#X obj 0 0 envgen~ 0 100 1 500 0", "Envelope generator", "Env Gen", OtherObject },
                { Icons::GlyphFuncGen, "#X obj 0 0 function~ 0 0.5 1 0.5 0", "Function generator", "Func Gen", OtherObject },
                { Icons::GlyphEnvelopeShape, "#X obj 0 0 envelope~", "Envelope waveforms", "Envelope", OtherObject },
                { Icons::GlyphSusLoop, "#X obj 0 0 susloop~", "Sustain looper for samplers", "Sus Loop", OtherObject },
                { Icons::GlyphRampEnv, "#X obj 0 0 ramp~", "Resettable ramp", "Ramp", OtherObject },
                { Icons::GlyphGlide, "#X obj 0 0 glide~ 100", "Signal glide and portamento", "Glide", OtherObject },
                { Icons::GlyphLag, "#X obj 0 0 lag~ 100", "Non-linear lag", "Lag", OtherObject },
                { Icons::GlyphSlew, "#X obj 0 0 slew~ 100", "Slew limiter", "Slew", OtherObject },
                { Icons::GlyphSmooth, "#X obj 0 0 smooth~ 100", "Signal smoother", "Smooth", OtherObject }
            } },
        { "Filters",
            {
                { Icons::GlyphSVFilter, "#X obj 0 0 svfilter~ 1729 0.42", "State variable filter", "SVFilter", OtherObject },
                { Icons::GlyphLop, "#X obj 0 0 lop~ 1000", "One-pole lowpass filter", "Lop", OtherObject },
                { Icons::GlyphHip, "#X obj 0 0 hip~ 500", "One-pole highpass filter", "Hip", OtherObject },
                { Icons::GlyphBpFilter, "#X obj 0 0 bp~ 1000 5", "2-pole bandpass filter", "BP", OtherObject },
                { Icons::GlyphVcf, "#X obj 0 0 vcf~ 5", "Voltage-controlled bandpass filter", "VCF", OtherObject },
                { Icons::GlyphBiquad, "#X obj 0 0 biquad~", "2-pole, 2-zero filter", "Biquad", OtherObject },
                { Icons::GlyphSlop, "#X obj 0 0 slop~ 1000", "Slew-limiting lowpass filter", "Slop", OtherObject },
                { Icons::GlyphLowpassRes, "#X obj 0 0 lowpass~ 1000 1", "Resonant lowpass filter", "Lowpass", OtherObject },
                { Icons::GlyphHighpassRes, "#X obj 0 0 highpass~ 500 1", "Resonant highpass filter", "Highpass", OtherObject },
                { Icons::GlyphBandpassRes, "#X obj 0 0 bandpass~ 1000 2", "Resonant bandpass filter", "Bandpass", OtherObject },
                { Icons::GlyphBandstopFilt, "#X obj 0 0 bandstop~ 1000 2", "Bandstop filter", "Bandstop", OtherObject },
                { Icons::GlyphLowshelfFilt, "#X obj 0 0 lowshelf~ 200 0.5 -6", "Lowshelf filter", "Lowshelf", OtherObject },
                { Icons::GlyphHighshelfFilt, "#X obj 0 0 highshelf~ 4000 0.5 -6", "Highshelf filter", "Highshelf", OtherObject },
                { Icons::GlyphParametricEq, "#X obj 0 0 eq~ 1000 2 6", "Parametric equalizer", "EQ", OtherObject },
                { Icons::GlyphAllpassFilt, "#X obj 0 0 allpass.filt~ 2 500 1", "Allpass filter", "Allpass", OtherObject },
                { Icons::GlyphCombFilt, "#X obj 0 0 comb.filt~ 10 0.9", "Comb filter", "Comb Filt", OtherObject },
                { Icons::GlyphResonantFilt, "#X obj 0 0 resonant~ 1000 10", "Constant-skirt resonant filter", "Resonant", OtherObject },
                { Icons::GlyphCrossover, "#X obj 0 0 crossover~", "Crossover filter", "Crossover", OtherObject },
                { Icons::GlyphMoog, "#X obj 0 0 moog~ 1000 0.75 0.1", "Moog ladder filter", "Moog", OtherObject }
            } },
        { "Effects",
            {
                { Icons::GlyphCrusher, "#X obj 0 0 crusher~ 0.1 0.1", "Crusher", "Crusher", OtherObject },
                { Icons::GlyphDelayEffect, "#X obj 0 0 delay~ 22050 14700", "Delay", "Delay", OtherObject },
                { Icons::GlyphDrive, "#X obj 0 0 drive~", "Drive", "Drive", OtherObject },
                { Icons::GlyphFlanger, "#X obj 0 0 flanger~ 0.1 20 -0.6", "Flanger", "Flanger", OtherObject },
                { Icons::GlyphCombRev, "#X obj 0 0 comb.rev~ 500 1 0.99 0.99", "Comb reverberator", "Comb. Rev", OtherObject },
                { Icons::GlyphComp, "#X obj 0 0 duck~", "Sidechain compressor", "Duck", OtherObject },
                { Icons::GlyphBallance, "#X obj 0 0 balance~", "Balance", "Balance", OtherObject },
                { Icons::GlyphPan, "#X obj 0 0 pan2~", "Pan", "Pan", OtherObject },
                { Icons::GlyphReverb, "#X obj 0 0 free.rev~ 0.7 0.6 0.5 0.7", "Reverb", "Reverb", OtherObject },
                { Icons::GlyphFreeze, "#X obj 0 0 freeze~", "Freeze", "Freeze", OtherObject },
                { Icons::GlyphRingmod, "#X obj 0 0 rm~ 150", "Ringmod", "Ringmod", OtherObject },
                { Icons::GlyphClip, "#X obj 0 0 clip~ -0.5 0.5", "Clip", "Clip", OtherObject },
                { Icons::GlyphFold, "#X obj 0 0 fold~ -0.5 0.5", "Fold", "Fold", OtherObject },
                { Icons::GlyphWrap, "#X obj 0 0 wrap2~ -0.5 0.5", "Wrap", "Wrap", OtherObject },
                { Icons::GlyphChorus, "#X obj 0 0 chorus~ 0.5 0.5 0.5", "Chorus effect", "Chorus", OtherObject },
                { Icons::GlyphPhaser, "#X obj 0 0 phaser~ 4 1 0.5", "Phaser effect", "Phaser", OtherObject },
                { Icons::GlyphTremolo, "#X obj 0 0 tremolo~ 4 1", "Amplitude modulation", "Tremolo", OtherObject },
                { Icons::GlyphVibrato, "#X obj 0 0 vibrato~ 5 50", "Vibrato", "Vibrato", OtherObject },
                { Icons::GlyphVocoder, "#X obj 0 0 vocoder~ 24 75", "Channel vocoder", "Vocoder", OtherObject },
                { Icons::GlyphWaveshaper, "#X obj 0 0 shaper~", "Waveshaper", "Shaper", OtherObject },
                { Icons::GlyphDownsample, "#X obj 0 0 downsample~ 8000", "Downsample a signal", "Downsamp", OtherObject },
                { Icons::GlyphPitchShift, "#X obj 0 0 pitch.shift~ 1200 75", "Pitch shifter", "Pitch Shift", OtherObject },
                { Icons::GlyphFreqShift, "#X obj 0 0 freq.shift~ 100", "Frequency shifter", "Freq Shift", OtherObject },
                { Icons::GlyphCompress, "#X obj 0 0 compress~ -20 4 10 100", "Compressor", "Compress", OtherObject },
                { Icons::GlyphExpand, "#X obj 0 0 expand~ -40 2 10 100", "Expander", "Expand", OtherObject },
                { Icons::GlyphNoiseGate, "#X obj 0 0 noisegate~ -60 10 100", "Noise gate", "Noise Gate", OtherObject },
                { Icons::GlyphNormalize, "#X obj 0 0 norm~ -3", "Normalizer", "Normalize", OtherObject },
                { Icons::GlyphPlateReverb, "#X obj 0 0 plate.rev~", "Plate reverb", "Plate Rev", OtherObject },
                { Icons::GlyphEchoReverb, "#X obj 0 0 echo.rev~ 8 0.5", "Echo reverb", "Echo Rev", OtherObject },
                { Icons::GlyphDelwrite, "#X obj 0 0 delwrite~", "Write into a delay line", "Delwrite", OtherObject },
                { Icons::GlyphDelread, "#X obj 0 0 delread4~", "Interpolating delay line read", "Delread4", OtherObject }
            } },
        { "Math",
            {
                { Icons::GlyphGeneric, "", "Add", "+", OtherObject },
                { Icons::GlyphGeneric, "", "Subtract", "-", OtherObject },
                { Icons::GlyphGeneric, "", "Multiply", "*", OtherObject },
                { Icons::GlyphGeneric, "", "Divide", "/", OtherObject },
                { Icons::GlyphGeneric, "", "Remainder", "%", OtherObject },
                { Icons::GlyphGeneric, "", "Reversed inlet subtraction", "!-", OtherObject },
                { Icons::GlyphGeneric, "", "Reversed inlet division", "!/", OtherObject },
                { Icons::GlyphGeneric, "", "Greater than", ">", OtherObject },
                { Icons::GlyphGeneric, "", "Less than", "<", OtherObject },
                { Icons::GlyphGeneric, "", "Greater or equal", ">=", OtherObject },
                { Icons::GlyphGeneric, "", "Less or equal", "<=", OtherObject },
                { Icons::GlyphGeneric, "", "Equality", "==", OtherObject },
                { Icons::GlyphGeneric, "", "Not equal", "!=", OtherObject },
                { Icons::GlyphGeneric, "", "Minimum", "min", OtherObject },
                { Icons::GlyphGeneric, "", "Maximum", "max", OtherObject },
                { Icons::GlyphGeneric, "", "Integer division", "div", OtherObject },
                { Icons::GlyphGeneric, "", "Integer remainder", "mod", OtherObject },
                { Icons::GlyphGeneric, "", "Absolute value", "abs", OtherObject },
                { Icons::GlyphGeneric, "", "Square root", "sqrt", OtherObject },
                { Icons::GlyphGeneric, "", "Power", "pow", OtherObject },
                { Icons::GlyphGeneric, "", "Logarithm", "log", OtherObject },
                { Icons::GlyphGeneric, "", "Exponential", "exp", OtherObject },
                { Icons::GlyphGeneric, "", "Sine", "sin", OtherObject },
                { Icons::GlyphGeneric, "", "Cosine", "cos", OtherObject },
                { Icons::GlyphGeneric, "", "Tangent", "tan", OtherObject },
                { Icons::GlyphGeneric, "", "Arctangent", "atan", OtherObject },
                { Icons::GlyphGeneric, "", "2-argument arctangent", "atan2", OtherObject },
                { Icons::GlyphGeneric, "", "Wrap to the range 0 to 1", "wrap", OtherObject },
                { Icons::GlyphGeneric, "", "Force a number into a range", "clip", OtherObject },
                { Icons::GlyphGeneric, "", "Logical and", "&&", OtherObject },
                { Icons::GlyphGeneric, "", "Logical or", "||", OtherObject },
                { Icons::GlyphGeneric, "", "Bitwise and", "&", OtherObject },
                { Icons::GlyphGeneric, "", "Bitwise or", "|", OtherObject },
                { Icons::GlyphGeneric, "", "Left bit shift", "<<", OtherObject },
                { Icons::GlyphGeneric, "", "Right bit shift", ">>", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Add", "+~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Subtract", "-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Multiply", "*~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Divide", "/~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Remainder", "%~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Reversed inlet subtraction", "!-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Reversed inlet division", "!/~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Greater than", ">~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Less than", "<~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Greater or equal", ">=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Less or equal", "<=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Equality", "==~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Not equal", "!=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Minimum", "min~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Maximum", "max~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Absolute value", "abs~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Square root", "sqrt~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Reciprocal square root", "rsqrt~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Power", "pow~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Logarithm", "log~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Exponential", "exp~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Sine", "sin~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Hyperbolic tangent", "tanh~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Remainder modulo 1", "wrap~", OtherObject }
            } },
    };

    static inline HeapArray<std::pair<String, HeapArray<std::tuple<String, String, String, String, ObjectIDs>>>> const heavyObjectList = {
        { "Essentials",
            {
                { Icons::GlyphEmptyObject, "#X obj 0 0", "(@keypress) Empty object", "Object", NewObject },
                { Icons::GlyphMessage, "#X msg 0 0", "(@keypress) Message", "Message", NewMessage },
                { Icons::GlyphFloatBox, "#X floatatom 0 0 5 0 0 0 - - - 0", "(@keypress) Float box", "Float", NewFloatAtom },
                { Icons::GlyphSymbolBox, "#X symbolatom 0 0 10 0 0 0 - - - 0", "Symbol box", "Symbol", NewSymbolAtom },
                { Icons::GlyphComment, "#X text 0 0 comment", "(@keypress) Comment", "Comment", NewComment },
                { Icons::GlyphArray, "#N canvas 0 0 450 250 (subpatch) 0;\n#X array @arrName 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(@keypress) Array", "Array", NewArray },
                { Icons::GlyphGOP, "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 0 0 graph;", "(@keypress) Graph on parent", "Graph", NewGraphOnParent },
            } },
        { "User Interface",
            {
                // GUI object default settings are in OjbectManager.h
                { Icons::GlyphBang, "bng", "(@keypress) Bang", "Bang", NewBang },
                { Icons::GlyphToggle, "tgl", "(@keypress) Toggle", "Toggle", NewToggle },
                { Icons::GlyphKnob, "knob", "Knob", "Knob", OtherObject },
                { Icons::GlyphVSlider, "vsl", "(@keypress) Vertical slider", "V. Slider", NewVerticalSlider },
                { Icons::GlyphHSlider, "hsl", "(@keypress) Horizontal slider", "H. Slider", NewHorizontalSlider },
                { Icons::GlyphVRadio, "vradio", "(@keypress) Vertical radio box", "V. Radio", NewVerticalRadio },
                { Icons::GlyphHRadio, "hradio", "(@keypress) Horizontal radio box", "H. Radio", NewHorizontalRadio },
                { Icons::GlyphNumber, "nbx", "(@keypress) Number box", "Number", NewNumbox },
                { Icons::GlyphCanvas, "cnv", "(@keypress) Canvas", "Canvas", NewCanvas },
            } },
        { "General",
            {
                { Icons::GlyphMetro, "#X obj 0 0 metro 120", "Metro", "Metro", OtherObject },
                { Icons::GlyphTrigger, "#X obj 0 0 trigger", "Trigger", "Trigger", OtherObject },
                { Icons::GlyphMoses, "#X obj 0 0 moses", "Moses", "Moses", OtherObject },
                { Icons::GlyphSpigot, "#X obj 0 0 spigot", "Spigot", "Spigot", OtherObject },
                { Icons::GlyphSelect, "#X obj 0 0 select", "Select", "Select", OtherObject },
                { Icons::GlyphRoute, "#X obj 0 0 route", "Route", "Route", OtherObject },
                { Icons::GlyphLoadbang, "#X obj 0 0 loadbang", "Loadbang", "Loadbang", OtherObject },
                { Icons::GlyphPack, "#X obj 0 0 pack", "Pack", "Pack", OtherObject },
                { Icons::GlyphUnpack, "#X obj 0 0 unpack", "Unpack", "Unpack", OtherObject },
                { Icons::GlyphPrint, "#X obj 0 0 print", "Print", "Print", OtherObject },
                { Icons::GlyphTimer, "#X obj 0 0 timer", "Timer", "Timer", OtherObject },
                { Icons::GlyphDelay, "#X obj 0 0 delay 60", "Delay", "Delay", OtherObject },
            } },
        { "MIDI",
            {
                { Icons::GlyphMidiIn, "#X obj 0 0 midiin", "MIDI in", "MIDI in", OtherObject },
                { Icons::GlyphMidiOut, "#X obj 0 0 midiout", "MIDI out", "MIDI out", OtherObject },
                { Icons::GlyphNoteIn, "#X obj 0 0 notein", "Note in", "Note in", OtherObject },
                { Icons::GlyphNoteOut, "#X obj 0 0 noteout", "Note out", "Note out", OtherObject },
                { Icons::GlyphCtlIn, "#X obj 0 0 ctlin", "Control in", "Ctl in", OtherObject },
                { Icons::GlyphCtlOut, "#X obj 0 0 ctlout", "Control out", "Ctl out", OtherObject },
                { Icons::GlyphPgmIn, "#X obj 0 0 pgmin", "Program in", "Pgm in", OtherObject },
                { Icons::GlyphPgmOut, "#X obj 0 0 pgmout", "Program out", "Pgm out", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 touchin", "Touch in", "Tch in", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 touchout", "Touch out", "Tch out", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 polytouchin", "Poly Touch in", "Ptch in", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 polytouchout", "Poly Touch in", "Ptch out", OtherObject },
                { Icons::GlyphMtof, "#X obj 0 0 mtof", "MIDI to frequency", "mtof", OtherObject },
                { Icons::GlyphFtom, "#X obj 0 0 ftom", "Frequency to MIDI", "ftom", OtherObject },
            } },
        { "Input & Output",
            {
                { Icons::GlyphAdc, "#X obj 0 0 adc~", "Adc", "Adc", OtherObject },
                { Icons::GlyphDac, "#X obj 0 0 dac~", "Dac", "Dac", OtherObject },
                { Icons::GlyphSend, "#X obj 0 0 s", "Send", "Send", OtherObject },
                { Icons::GlyphReceive, "#X obj 0 0 r", "Receive", "Receive", OtherObject },
                { Icons::GlyphSignalSend, "#X obj 0 0 s~", "Send~", "Send~", OtherObject },
                { Icons::GlyphSignalReceive, "#X obj 0 0 r~", "Receive~", "Receive~", OtherObject },
            } },
        { "Oscillators",
            {
                { Icons::GlyphPhasor, "#X obj 0 0 phasor~", "Phasor", "Phasor", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 osc~ 440", "Osc", "Osc", OtherObject },
                { Icons::GlyphOscBL, "#X obj 0 0 hv.osc~ sine", "Sine band limited", "Hv Sine", OtherObject },
                { Icons::GlyphSquareBL, "#X obj 0 0 hv.osc~ square", "Square band limited", "Hv Square", OtherObject },
                { Icons::GlyphSawBL, "#X obj 0 0 hv.osc~ saw", "Saw band limited", "Hv Saw", OtherObject },
                { Icons::GlyphPinknoise, "#X obj 0 0 hv.pinknoise~", "Pink Noise", "Pink Noise", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 hv.lfo sine", "Sine LFO", "Sine LFO", OtherObject },
                { Icons::GlyphLFORamp, "#X obj 0 0 hv.lfo ramp", "Ramp LFO", "Ramp LFO", OtherObject },
                { Icons::GlyphLFOSaw, "#X obj 0 0 hv.lfo saw", "Saw LFO", "Saw LFO", OtherObject },
                { Icons::GlyphTriangle, "#X obj 0 0 hv.lfo triangle", "Triangle LFO", "Tri LFO", OtherObject },
                { Icons::GlyphLFOSquare, "#X obj 0 0 hv.lfo square", "Square LFO", "Sq LFO", OtherObject },
                { Icons::GlyphPulse, "#X obj 0 0 hv.lfo pulse", "Pulse LFO", "Pulse LFO", OtherObject },
            } },
        { "Effects",
            {
                { Icons::GlyphComp, "#X obj 0 0 hv.compressor~", "Compressor", "Compress", OtherObject },
                { Icons::GlyphFlanger, "#X obj 0 0 hv.flanger~", "Flanger", "Flanger", OtherObject },
                { Icons::GlyphCombRev, "#X obj 0 0 hv.comb~", "Comb filter", "Comb. Filt", OtherObject },
                { Icons::GlyphReverb, "#X obj 0 0 hv.reverb~", "Reverb", "Reverb", OtherObject },
                { Icons::GlyphRezLowpass, "#X obj 0 0 hv.filter~ lowpass", "Resonant Lowpass Filter", "Res Lp Filt", OtherObject },
                { Icons::GlyphBandpass, "#X obj 0 0 hv.filter~ bandpass1", "Resonant Bandpass Filter", "Res Bp Filt", OtherObject },
                { Icons::GlyphRezHighpass, "#X obj 0 0 hv.filter~ highpass", "Resonant Highpass Filter", "Res Hp Filt", OtherObject },
                { Icons::GlyphAllPass, "#X obj 0 0 hv.filter~ allpass", "Resonant Allpass Filter", "Res Ap Filt", OtherObject },
                { Icons::GlyphLowpass, "#X obj 0 0 hv.lop~", "One-pole Lowpass", "1p Lp Filt", OtherObject },
                { Icons::GlyphHighpass, "#X obj 0 0 hv.hip~", "One-pole Highpass", "1p Hp Filt", OtherObject },
                { Icons::GlyphFreqShift, "#X obj 0 0 hv.freqshift~", "Frequency Shifter", "Freq Shift", OtherObject },
            } },
        { "Math",
            {
                { Icons::GlyphGeneric, "", "Add", "+", OtherObject },
                { Icons::GlyphGeneric, "", "Subtract", "-", OtherObject },
                { Icons::GlyphGeneric, "", "Multiply", "*", OtherObject },
                { Icons::GlyphGeneric, "", "Divide", "/", OtherObject },
                { Icons::GlyphGeneric, "", "Remainder", "%", OtherObject },
                { Icons::GlyphGeneric, "", "Greater than", ">", OtherObject },
                { Icons::GlyphGeneric, "", "Less than", "<", OtherObject },
                { Icons::GlyphGeneric, "", "Greater or equal", ">=", OtherObject },
                { Icons::GlyphGeneric, "", "Less or equal", "<=", OtherObject },
                { Icons::GlyphGeneric, "", "Equality", "==", OtherObject },
                { Icons::GlyphGeneric, "", "Not equal", "!=", OtherObject },
                { Icons::GlyphGeneric, "", "Minimum", "min", OtherObject },
                { Icons::GlyphGeneric, "", "Maximum", "max", OtherObject },
            } },
        { "Signal Math",
            {
                { Icons::GlyphGenericSignal, "", "(signal) Add", "+~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Subtract", "-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Multiply", "*~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Divide", "/~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.gt~", "(signal) Greater than", ">~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.lt~", "(signal) Less than", "<~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.gte~", "(signal) Greater or equal", ">=~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.lte~", "(signal) Less or equal", "<=~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.eq~", "(signal) Equality", "==~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.neq~", "(signal) Not equal", "!=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Minimum", "min~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Maximum", "max~", OtherObject },
            } },
    };

    static String getObjectPatch(String const& patch, String const& name)
    {
        if (patch.isEmpty())
            return "#X obj 0 0 " + name;
        if (!patch.startsWith("#"))
            return ObjectThemeManager::get()->getCompleteFormat(patch);

        return patch;
    }

private:
    void addObjectItem(Section* section, String const& icon, String const& patch, String const& tooltip, String const& name, ObjectIDs const objectID)
    {
        addItem(section, new ObjectItem(editor, name, icon, tooltip, getObjectPatch(patch, name), objectID, dismissMenu));
    }

    void addItem(Section* section, Component* item)
    {
        section->items.add(item);
        addAndMakeVisible(item);
    }

    static String getObjectNameFromPatch(String const& patch)
    {
        return patch.fromFirstOccurrenceOf("#X obj 0 0 ", false, false).upToFirstOccurrenceOf(" ", false, false).trim();
    }

    OwnedArray<Section> sections;
    PluginEditor* editor;
    std::function<void(bool)> dismissMenu;

    static constexpr int headerHeight = 26;
    static constexpr int rowHeight = 28;
    static constexpr int margin = 8;
    static constexpr int maxSearchResults = 50;
};

class ObjectCategoryList final : public Component {

public:
    std::function<void(String const&)> onCategoryClicked = [](String const&) { };

    void setCategories(StringArray const& newCategories)
    {
        categories = newCategories;
        repaint();
    }

    void setSelectedCategory(String const& category)
    {
        auto const index = categories.indexOf(category);
        if (index == selectedIndex)
            return;

        selectedIndex = index;
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        for (int i = 0; i < categories.size(); i++) {
            auto const bounds = getRowBounds(i);

            if (i == selectedIndex || i == hoveredIndex) {
                g.setColour(i == selectedIndex ? colours.popupMenuActiveBackgroundColour : colours.popupMenuActiveBackgroundColour.withAlpha(0.5f));
                g.fillRoundedRectangle(bounds.reduced(4, 1).toFloat(), Corners::defaultCornerRadius);
            }

            Fonts::drawText(g, categories[i], bounds.withTrimmedLeft(14), colours.popupMenuTextColour, 14);
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        setHoveredRow(getRowAt(e.y));
    }

    void mouseExit(MouseEvent const& e) override
    {
        setHoveredRow(-1);
    }

    void mouseUp(MouseEvent const& e) override
    {
        auto const row = getRowAt(e.y);
        if (isPositiveAndBelow(row, categories.size()))
            onCategoryClicked(categories[row]);
    }

private:
    Rectangle<int> getRowBounds(int const index) const
    {
        return { 0, index * rowHeight, getWidth(), rowHeight };
    }

    int getRowAt(int const y) const
    {
        return y < 0 ? -1 : y / rowHeight;
    }

    void setHoveredRow(int const row)
    {
        if (row != hoveredIndex) {
            hoveredIndex = row;
            repaint();
        }
    }

    StringArray categories;
    int selectedIndex = -1;
    int hoveredIndex = -1;

    static constexpr int rowHeight = 26;
};

class ObjectBrowserButton final : public Component {
public:
    std::function<void()> onClick = [] { };

    explicit ObjectBrowserButton()
    {
        setInterceptsMouseClicks(true, false);
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        auto b = getLocalBounds().reduced(4, 2);

        if (isMouseOver()) {
            g.setColour(colours.popupMenuActiveBackgroundColour);
            g.fillRoundedRectangle(b.toFloat(), Corners::defaultCornerRadius);
        }

        auto const colour = colours.popupMenuTextColour;
        auto const iconArea = b.removeFromLeft(24).withSizeKeepingCentre(24, 24);

        Fonts::drawIcon(g, Icons::Object, iconArea, colour, 14.0f, true);
        b.removeFromLeft(4);
        b.removeFromRight(3);

        Fonts::drawFittedText(g, "Object Browser", b, colour, 1, 0.9f, 14.0f);
    }

    void mouseUp(MouseEvent const& e) override
    {
        onClick();
        repaint();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }
};

class ObjectListViewport final : public BouncingViewport {
public:
    std::function<void()> onScroll = [] { };

private:
    void visibleAreaChanged(Rectangle<int> const& newVisibleArea) override
    {
        onScroll();
    }
};

class AddObjectMenu final : public Component {

public:
    explicit AddObjectMenu(PluginEditor* e)
        : editor(e)
        , objectList(e, [this](bool const shouldFade) { dismiss(shouldFade); })
    {
        auto const& colours = getThemeColours(*this);

        searchInput.setBackgroundColour(PlugDataColour::popupMenuActiveBackgroundColourId);
        searchInput.setTextToShowWhenEmpty("Search objects...", colours.popupMenuTextColour.withAlpha(0.5f));
        searchInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::textColourId, colours.popupMenuTextColour);
        searchInput.setBorder({ 1, 23, 5, 1 });
        searchInput.onTextChange = [this] { updateObjects(); };

        viewport.setViewedComponent(&objectList, false);
        viewport.setScrollBarsShown(true, false);
        viewport.onScroll = [this] { updateSelectedCategory(); };

        StringArray categoryNames;
        for (auto const& [categoryName, objectCategory] : ObjectList::getObjectsToShow())
            categoryNames.add(categoryName);

        categoryList.setCategories(categoryNames);
        categoryList.onCategoryClicked = [this](String const& category) {
            if (searchInput.getText().isNotEmpty()) {
                searchInput.clear();
                updateObjects();
            }
            viewport.setViewPosition(0, objectList.getCategoryY(category));
            updateSelectedCategory();
        };

        objectBrowserButton.onClick = [this] {
            dismiss(false);
            MessageManager::callAsync([e = SafePointer(editor)] {
                if (e)
                    Dialogs::showObjectBrowserDialog(&e->openedDialog, e);
            });
        };

        addAndMakeVisible(searchInput);
        addAndMakeVisible(viewport);
        addAndMakeVisible(categoryList);
        addAndMakeVisible(objectBrowserButton);

        setSize(panelWidth, panelHeight);
        updateObjects();

        updater.addAnimator(alphaAnimator);
        alphaAnimator.complete();
    }

    void parentHierarchyChanged() override
    {
        // Don't pop up the on-screen keyboard as soon as the panel appears
        if (SettingsFile::getInstance()->isUsingTouchMode())
            return;

        MessageManager::callAsync([_this = SafePointer(this)] {
            if (_this && _this->isShowing())
                _this->searchInput.grabKeyboardFocus();
        });
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // The dialog puts its close button in the titlebar area
        if (SettingsFile::getInstance()->isUsingTouchMode())
            bounds.removeFromTop(titlebarHeight);

        searchInput.setBounds(bounds.removeFromTop(36).reduced(6, 4));

        auto sidebarBounds = bounds.removeFromLeft(sidebarWidth);
        objectBrowserButton.setBounds(sidebarBounds.removeFromBottom(30).reduced(2, 0));
        categoryList.setBounds(sidebarBounds.withTrimmedTop(4).withTrimmedBottom(6));

        viewport.setBounds(bounds);
        objectList.setSize(viewport.getMaximumVisibleWidth(), objectList.getHeight());
    }

    void paint(Graphics& g) override
    {
        if (!SettingsFile::getInstance()->isUsingTouchMode())
            return;

        // The dialog already painted our background, we only need the title itself
        Fonts::drawStyledText(g, "Add New Object", Rectangle<int>(0, 4, getWidth(), titlebarHeight - 8), getThemeColours(*this).panelTextColour, Semibold, 15, Justification::centred);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        g.setColour(colours.outlineColour);
        g.drawVerticalLine(sidebarWidth, searchInput.getBottom() + 4.0f, getHeight() - 4.0f);
        g.drawHorizontalLine(objectBrowserButton.getY() - 3, 10.0f, sidebarWidth - 10.0f);

        Fonts::drawIcon(g, Icons::Search, searchInput.getX(), searchInput.getY(), searchInput.getHeight(), colours.popupMenuTextColour, 12);
    }

    void dismiss(bool const shouldHide)
    {
        auto* panel = getPanelComponent();
        if (!panel)
            return;

        // Fade the panel on drag start: calling dismiss or setVisible will lead to the drag event getting lost, so we just set alpha instead
        // Ditto for calling animator.fadeOut because that will also call setVisible(false)
        if (shouldHide) {
            startAlpha = panel->getAlpha();
            targetAlpha = 0.0f;
            if (alphaAnimator.isComplete())
                alphaAnimator.start();
        }
        // and destroy the panel on mouse-up
        else if (auto* dialog = dynamic_cast<Dialog*>(panel)) {
            MessageManager::callAsync([d = SafePointer(dialog)] {
                if (d)
                    d->closeDialog();
            });
        } else {
            currentCalloutBox->dismiss();
        }
    }

    static void show(PluginEditor* editor, Rectangle<int> const bounds)
    {
        if (SettingsFile::getInstance()->isUsingTouchMode()) {
            auto* dialog = new Dialog(&editor->openedDialog, editor, panelWidth, panelHeight + titlebarHeight, true);
            dialog->setViewedComponent(new AddObjectMenu(editor));
            editor->openedDialog.reset(dialog);
        } else {
            currentCalloutBox = &editor->showCalloutBox(std::make_unique<AddObjectMenu>(editor), bounds);
        }
    }

private:

    Component* getPanelComponent() const
    {
        if (auto* dialog = findParentComponentOfClass<Dialog>())
            return dialog;

        return currentCalloutBox.getComponent();
    }

    void updateObjects()
    {
        auto const query = searchInput.getText().trim();
        if (query.isEmpty())
            objectList.showAllCategories();
        else
            objectList.showSearchResults(query);

        objectList.setSize(viewport.getMaximumVisibleWidth(), objectList.getHeight());
        viewport.setViewPosition(0, 0);
        updateSelectedCategory();
    }

    void updateSelectedCategory()
    {
        auto const visibleArea = viewport.getViewArea();

        // When we're scrolled all the way down, the last category can never reach the top of the viewport
        auto const isAtBottom = viewport.canScrollVertically() && visibleArea.getBottom() >= objectList.getHeight();
        categoryList.setSelectedCategory(isAtBottom ? objectList.getLastCategory() : objectList.getCategoryAt(visibleArea.getY()));
    }

    ObjectBrowserButton objectBrowserButton;
    static inline SafePointer<CallOutBox> currentCalloutBox = nullptr;
    PluginEditor* editor;
    ObjectList objectList;
    ObjectListViewport viewport;
    ObjectCategoryList categoryList;
    SearchEditor searchInput;

    static constexpr int panelWidth = 670;
    static constexpr int panelHeight = 400;
    static constexpr int titlebarHeight = 32;
    static constexpr int sidebarWidth = 125;

    float startAlpha, targetAlpha;
    VBlankAnimatorUpdater updater { this };
    Animator alphaAnimator = ValueAnimatorBuilder { }
                                 .withDurationMs(220)
                                 .withEasing(Easings::createEaseOut())
                                 .withValueChangedCallback([this](float v) {
                                     if (auto* panel = getPanelComponent())
                                         panel->setAlpha(makeAnimationLimits(startAlpha, targetAlpha).lerp(v));
                                 })
                                 .build();
};
