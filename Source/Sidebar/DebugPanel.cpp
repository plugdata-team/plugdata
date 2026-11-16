/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "DebugPanel.h"

#include "Utility/Fonts.h"
#include "Utility/SettingsFile.h"
#include "Components/BouncingViewport.h"
#include "LookAndFeel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Object.h"
#include "Objects/ObjectBase.h"
#include "Connection.h"
#include "DetectiveMode.h"
#include "Pd/DSPProfiler.h"

// Further from the sidebar background than its active background, for things drawn on top of that
static Colour getRaisedSidebarColour(PlugDataColours const& colours, float const amount)
{
    auto const active = colours.sidebarActiveBackgroundColour;
    auto const background = colours.sidebarBackgroundColour;
    auto const step = [amount](uint8 const a, uint8 const b) {
        return static_cast<uint8>(jlimit(0, 255, roundToInt(a + (static_cast<int>(a) - static_cast<int>(b)) * amount)));
    };
    return Colour(step(active.getRed(), background.getRed()), step(active.getGreen(), background.getGreen()), step(active.getBlue(), background.getBlue()));
}

// Sidebar button with a label, for the Debugger panel
class DebuggerButton final : public Button {
public:
    // A raised button stands out from the sidebar's active background, for one that's drawn on it
    explicit DebuggerButton(String const& text, bool const isRaised = false)
        : Button(text)
        , raise(isRaised ? 1.0f : 0.0f)
    {
        setButtonText(text);
    }

    void paintButton(Graphics& g, bool const isHighlighted, bool const isDown) override
    {
        auto const& colours = getThemeColours(*this);

        auto amount = raise;
        if (isEnabled() && isDown)
            amount += 1.5f;
        else if (isEnabled() && isHighlighted)
            amount += 0.75f;

        g.setColour(getRaisedSidebarColour(colours, amount));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);

        auto const colour = colours.sidebarTextColour.withMultipliedAlpha(isEnabled() ? 1.0f : 0.4f);
        Fonts::drawStyledText(g, getButtonText(), getLocalBounds(), colour, Semibold, 13, Justification::centred);
    }

private:
    float raise;
};

// Options next to each other, of which one is selected. Drawn like the on/off switches of the properties panels
class SegmentedControl final : public Component
    , public TooltipClient {
public:
    explicit SegmentedControl(StringArray optionNames)
        : options(std::move(optionNames))
    {
        tooltips.resize(options.size());
        enabledOptions.resize(options.size(), true);
    }

    std::function<void(int)> onChange;

    int getSelectedIndex() const { return selectedIndex; }

    void setSelectedIndex(int const index, NotificationType const notification = sendNotification)
    {
        if (index == selectedIndex || !isPositiveAndBelow(index, options.size()) || !enabledOptions[index])
            return;

        selectedIndex = index;
        repaint();
        if (notification != dontSendNotification && onChange)
            onChange(selectedIndex);
    }

    // A disabled option is shown, but can't be selected
    void setOptionEnabled(int const index, bool const shouldBeEnabled)
    {
        enabledOptions[index] = shouldBeEnabled;
        repaint();
    }

    void setOptionTooltip(int const index, String const& tooltip)
    {
        tooltips[index] = tooltip;
    }

    String getTooltip() override
    {
        return isPositiveAndBelow(hoveredIndex, options.size()) ? tooltips[hoveredIndex] : String();
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);
        auto const textColour = colours.sidebarTextColour;
        bool const isLightTheme = textColour.getPerceivedBrightness() < 0.5f;

        g.setColour(textColour.withAlpha(0.07f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);

        for (int i = 0; i < options.size(); i++) {
            auto const segment = getSegmentBounds(i);
            auto const isSelected = i == selectedIndex;
            if (isSelected) {
                g.setColour(isLightTheme ? Colours::white : textColour.withAlpha(0.2f));
                g.fillRoundedRectangle(segment, Corners::defaultCornerRadius - 2.0f);
                g.setColour(textColour.withAlpha(0.12f));
                g.drawRoundedRectangle(segment, Corners::defaultCornerRadius - 2.0f, 0.5f);
            } else if (i == hoveredIndex && enabledOptions[i]) {
                g.setColour(textColour.withAlpha(0.05f));
                g.fillRoundedRectangle(segment, Corners::defaultCornerRadius - 2.0f);
            }

            auto const alpha = isSelected ? 1.0f : enabledOptions[i] ? 0.55f : 0.25f;
            Fonts::drawFittedText(g, options[i], segment.toNearestInt().reduced(2, 0), textColour.withAlpha(alpha), 1, 0.8f, 13.0f, Justification::centred);
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        setHoveredIndex(getSegmentAt(e.position));
    }

    void mouseExit(MouseEvent const&) override
    {
        setHoveredIndex(-1);
    }

    void mouseDown(MouseEvent const& e) override
    {
        setSelectedIndex(getSegmentAt(e.position));
    }

private:
    Rectangle<float> getSegmentBounds(int const index) const
    {
        auto const area = getLocalBounds().toFloat().reduced(2.0f);
        auto const width = area.getWidth() / static_cast<float>(options.size());
        return area.withX(area.getX() + width * index).withWidth(width);
    }

    int getSegmentAt(Point<float> const position) const
    {
        for (int i = 0; i < options.size(); i++) {
            if (getSegmentBounds(i).contains(position))
                return i;
        }
        return -1;
    }

    void setHoveredIndex(int const index)
    {
        if (index != hoveredIndex) {
            hoveredIndex = index;
            repaint();
        }
    }

    StringArray options;
    std::vector<String> tooltips;
    std::vector<bool> enabledOptions;
    int selectedIndex = 0;
    int hoveredIndex = -1;
};

// Debugger panel view that shows how much of the CPU each object takes, by object or by type
// It measures while it's showing
class ProfilerView final : public Component
    , private Timer {

    struct Row {
        String name;
        String detail;
        String type;
        void* object = nullptr;
        void* target = nullptr; // the object to show when it's clicked
        double load = 0.0;
        bool isClickable = true;
    };

    class ProfileList final : public Component
        , public SettableTooltipClient {
    public:
        std::function<void(Row const&)> onRowClicked;

        void setRows(SmallArray<Row>&& newRows)
        {
            rows = std::move(newRows);
            maxLoad = 0.0;
            for (auto const& row : rows)
                maxLoad = jmax(maxLoad, row.load);

            setSize(getWidth(), jmax(listPadding * 2 + static_cast<int>(rows.size()) * rowHeight, getParentHeight()));
            repaint();
        }

        void setSelectedObject(void* object)
        {
            selectedObject = object;
            repaint();
        }

        void paint(Graphics& g) override
        {
            auto const& colours = getThemeColours(*this);
            auto const clip = g.getClipBounds();

            for (int i = 0; i < rows.size(); i++) {
                auto const& row = rows[i];
                auto const rowBounds = getRowBounds(i);
                if (!rowBounds.intersects(clip))
                    continue;

                auto const isSelected = row.object && row.object == selectedObject;
                if (isSelected || (i == hoveredIndex && row.isClickable)) {
                    g.setColour(getRaisedSidebarColour(colours, isSelected ? 1.0f : 0.5f));
                    g.fillRoundedRectangle(rowBounds.toFloat(), Corners::defaultCornerRadius);
                }

                auto content = rowBounds.reduced(8, 5);
                auto firstLine = content.removeFromTop(18);
                g.setFont(Fonts::getTabularNumbersFont().withHeight(13.0f));
                g.setColour(colours.sidebarTextColour);
                g.drawText(formatLoad(row.load), firstLine.removeFromRight(52), Justification::centredRight);

                Fonts::drawFittedText(g, row.name, firstLine.withTrimmedRight(4), colours.sidebarTextColour, 1, 0.9f, 14.0f);
                Fonts::drawFittedText(g, row.detail, content.removeFromTop(15), colours.sidebarTextColour.withAlpha(0.55f), 1, 0.9f, 12.0f);

                // Relative to the highest load, so they're easy to compare
                if (maxLoad > 0.0 && row.load > 0.0) {
                    auto const bar = content.withTrimmedTop(4).removeFromTop(4).toFloat();
                    g.setColour(colours.objectSelectedOutlineColour);
                    g.fillRoundedRectangle(bar.withWidth(jmax(4.0f, static_cast<float>(bar.getWidth() * row.load / maxLoad))), 2.0f);
                }
            }
        }

        void mouseMove(MouseEvent const& e) override
        {
            setHoveredIndex(getRowAt(e.y));
        }

        void mouseExit(MouseEvent const&) override
        {
            setHoveredIndex(-1);
        }

        void mouseDown(MouseEvent const& e) override
        {
            auto const index = getRowAt(e.y);
            if (isPositiveAndBelow(index, rows.size()) && rows[index].isClickable && onRowClicked)
                onRowClicked(rows[index]);
        }

        MouseCursor getMouseCursor() override
        {
            return isPositiveAndBelow(hoveredIndex, rows.size()) && rows[hoveredIndex].isClickable ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor;
        }

        static String formatLoad(double const load)
        {
            auto const percent = load * 100.0;
            if (percent > 0.0 && percent < 0.1)
                return "<0.1%";
            return String(percent, 1) + "%";
        }

    private:
        Rectangle<int> getRowBounds(int const index) const
        {
            return { rowInset, listPadding + index * rowHeight, getWidth() - 2 * rowInset, rowHeight - 2 };
        }

        int getRowAt(int const y) const
        {
            auto const index = (y - listPadding) / rowHeight;
            return y >= listPadding && isPositiveAndBelow(index, rows.size()) ? index : -1;
        }

        void setHoveredIndex(int const index)
        {
            if (index != hoveredIndex) {
                hoveredIndex = index;
                repaint();
            }
        }

        static constexpr int listPadding = 4;
        static constexpr int rowInset = 4;
        static constexpr int rowHeight = 54;

        SmallArray<Row> rows;
        double maxLoad = 0.0;
        int hoveredIndex = -1;
        void* selectedObject = nullptr;
    };

public:
    explicit ProfilerView(PluginEditor* e)
        : editor(e)
    {
        // Messages can't be profiled yet, so "All" only has the signal objects
        kindSelector.setOptionEnabled(messageKind, false);
        kindSelector.setOptionTooltip(allKinds, "Signal and message objects");
        kindSelector.setOptionTooltip(signalKind, "Signal objects, while DSP is on");
        kindSelector.setOptionTooltip(messageKind, "Message objects can't be profiled yet");
        kindSelector.onChange = [this](int) {
            showType({});
        };
        addAndMakeVisible(kindSelector);

        viewSelector.onChange = [this](int) {
            showType({});
        };
        addAndMakeVisible(viewSelector);

        showAllButton.setTooltip("Show the objects of all types");
        showAllButton.onClick = [this] {
            showType({});
        };
        addChildComponent(showAllButton);

        list.onRowClicked = [this](Row const& row) {
            if (viewSelector.getSelectedIndex() == typesView) {
                viewSelector.setSelectedIndex(objectsView, dontSendNotification);
                showType(row.type);
            } else if (row.target) {
                list.setSelectedObject(row.object);
                editor->highlightSearchTarget(row.target, true);
            }
        };
        viewport.setViewedComponent(&list, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport);

        updateTooltip();
    }

    ~ProfilerView() override
    {
        setActive(false);
    }

    // Measures, and updates itself, while it's active
    void setActive(bool const shouldBeActive)
    {
        if (shouldBeActive == active)
            return;

        active = shouldBeActive;
        auto* profiler = editor->pd->dspProfiler.get();
        if (active) {
            profiler->addUser();
            update();
            startTimerHz(10);
        } else {
            profiler->removeUser();
            stopTimer();
        }
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);

        auto summary = summaryArea;
        g.setFont(Fonts::getTabularNumbersFont().withHeight(13.0f));
        g.setColour(colours.sidebarTextColour);
        if (totalText.isNotEmpty())
            g.drawText(totalText, summary.removeFromRight(56), Justification::centredRight);
        Fonts::drawFittedText(g, status, summary, colours.sidebarTextColour.withAlpha(0.6f), 1, 0.9f, 13.0f);

        if (typeFilter.isNotEmpty())
            Fonts::drawFittedText(g, typeFilter, filterArea, colours.sidebarTextColour, 1, 0.9f, 13.5f, Justification::centredLeft, Semibold);

        g.setColour(colours.sidebarActiveBackgroundColour);
        g.fillRoundedRectangle(listFrame.toFloat(), Corners::largeCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        if (placeholder.isEmpty())
            return;

        auto const& colours = getThemeColours(*this);
        Fonts::drawFittedText(g, placeholder, listFrame.reduced(16, 0).withTrimmedTop(18).withHeight(60), colours.sidebarTextColour.withAlpha(0.5f), 3, 1.0f, 13.5f, Justification::centredTop);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(8, 0).withTrimmedBottom(8);

        kindSelector.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(6);
        viewSelector.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(4);

        summaryArea = bounds.removeFromTop(30).reduced(4, 0);

        showAllButton.setVisible(typeFilter.isNotEmpty());
        if (typeFilter.isNotEmpty()) {
            auto filterRow = bounds.removeFromTop(24);
            showAllButton.setBounds(filterRow.removeFromRight(72).reduced(0, 1));
            filterArea = filterRow.withTrimmedLeft(4).withTrimmedRight(6);
            bounds.removeFromTop(6);
        }

        listFrame = bounds;
        viewport.setBounds(bounds.reduced(0, 2));
        list.setSize(viewport.getMaximumVisibleWidth(), jmax(list.getHeight(), viewport.getHeight()));
    }

private:
    static constexpr int allKinds = 0;
    static constexpr int signalKind = 1;
    static constexpr int messageKind = 2;
    static constexpr int objectsView = 0;
    static constexpr int typesView = 1;

    void timerCallback() override
    {
        update();
    }

    void update()
    {
        auto const* profiler = editor->pd->dspProfiler.get();
        if (profiler->getRevision() != lastRevision) {
            lastRevision = profiler->getRevision();
            updateRows();
        }

        String newStatus;
        if (profiler->getProfiledSeconds() > 0.0)
            newStatus = (profiler->isWaitingForDSP() ? "Waiting for DSP, measured " : "Measured for ") + formatDuration(profiler->getProfiledSeconds());
        else if (profiler->isWaitingForDSP())
            newStatus = "Waiting for DSP";
        else
            newStatus = "Measuring";

        if (newStatus != status) {
            status = newStatus;
            repaint();
        }
    }

    void showType(String const& type)
    {
        typeFilter = type;
        viewport.setViewPosition(0, 0);
        updateRows();
        updateTooltip();
        resized();
        repaint();
    }

    void updateRows()
    {
        auto const* profiler = editor->pd->dspProfiler.get();
        auto const& entries = profiler->getEntries();

        SmallArray<Row> rows;
        if (viewSelector.getSelectedIndex() == objectsView) {
            for (auto const& entry : entries) {
                if (typeFilter.isEmpty() || entry.type == typeFilter)
                    rows.add({ entry.text, entry.location, entry.type, entry.object, entry.highlightTarget, entry.getLoad(), entry.object != nullptr });
            }
        } else {
            // The instances of each type added up
            HashMap<String, int> typeRows;
            SmallArray<int> counts;
            for (auto const& entry : entries) {
                if (!typeRows.contains(entry.type)) {
                    typeRows.set(entry.type, rows.size());
                    rows.add({ entry.type, entry.object ? String() : entry.location, entry.type, nullptr, nullptr, 0.0, entry.object != nullptr });
                    counts.add(0);
                }
                auto const index = typeRows[entry.type];
                rows[index].load += entry.getLoad();
                counts[index]++;
            }
            for (int i = 0; i < rows.size(); i++) {
                if (rows[i].isClickable)
                    rows[i].detail = counts[i] == 1 ? "1 object" : String(counts[i]) + " objects";
            }
            std::ranges::stable_sort(rows, [](Row const& a, Row const& b) { return a.load > b.load; });
        }

        // Of the rows that are shown, so it's the total of one type when only that is shown
        double total = 0.0;
        for (auto const& row : rows)
            total += row.load;
        auto const newTotalText = rows.empty() ? String() : ProfileList::formatLoad(total);
        if (newTotalText != totalText) {
            totalText = newTotalText;
            repaint();
        }

        String newPlaceholder;
        if (rows.empty())
            newPlaceholder = profiler->isWaitingForDSP() ? "Turn on DSP to see how much CPU each signal object takes" : "Measuring...";

        if (newPlaceholder != placeholder) {
            placeholder = newPlaceholder;
            repaint();
        }

        list.setRows(std::move(rows));
    }

    void updateTooltip()
    {
        if (viewSelector.getSelectedIndex() == typesView)
            list.setTooltip("Click a type to see its objects");
        else
            list.setTooltip("Share of the time available to compute the audio, since this view was opened. Click an object to show it in its patch");
    }

    static String formatDuration(double const seconds)
    {
        auto const wholeSeconds = static_cast<int>(seconds);
        if (wholeSeconds < 60)
            return String(wholeSeconds) + " s";
        return String(wholeSeconds / 60) + " min " + String(wholeSeconds % 60) + " s";
    }

    PluginEditor* editor;

    SegmentedControl kindSelector { { "All", "Signal", "Message" } };
    SegmentedControl viewSelector { { "Objects", "Types" } };
    DebuggerButton showAllButton { "Show all" };
    ProfileList list;
    BouncingViewport viewport;

    bool active = false;
    String status;
    String totalText;
    String placeholder;
    String typeFilter;
    uint32 lastRevision = 0;

    Rectangle<int> summaryArea, filterArea, listFrame;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfilerView)
};

// The messages through the connection, oldest first, drawn like the console
class MessageList final : public Component
    , public SettableTooltipClient {
public:
    struct Row {
        uint64 number = 0;
        String text;

        TextLayout layout;
        int layoutWidth = -1;
        int y = 0;
        int height = 0;
    };

    explicit MessageList(Viewport& v)
        : viewport(v)
    {
        setWantsKeyboardFocus(true);
        setTooltip("Messages through the connection, oldest first, numbered from when the Debugger started tracing");
    }

    void setRows(SmallArray<Row>&& newRows)
    {
        auto const wasAtBottom = viewport.getViewPositionY() + viewport.getViewHeight() >= getHeight() - 4;

        rows = std::move(newRows);
        selected.erase(std::ranges::remove_if(selected, [this](uint64 const number) { return !findRow(number); }).begin(), selected.end());
        layoutWidth = -1;
        updateLayout();

        if (wasAtBottom)
            viewport.setViewPosition(0, jmax(0, getHeight() - viewport.getViewHeight()));
        repaint();
    }


    void clearSelection()
    {
        selected.clear();
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto const& colours = getThemeColours(*this);
        auto const clip = g.getClipBounds();
        auto const numberFont = Fonts::getTabularNumbersFont().withHeight(12.0f);

        for (auto const& row : rows) {
            auto const rowBounds = Rectangle<int>(0, row.y, getWidth(), row.height).reduced(rowInset, 1);
            if (!rowBounds.intersects(clip))
                continue;

            if (selected.contains(row.number)) {
                g.setColour(getRaisedSidebarColour(colours, 1.0f));
                g.fillRoundedRectangle(rowBounds.toFloat(), Corners::defaultCornerRadius);
            }

            auto content = rowBounds.reduced(rowPaddingX, rowPaddingY).toFloat();
            g.setFont(numberFont);
            g.setColour(colours.sidebarTextColour.withAlpha(0.45f));
            g.drawText(String(row.number), content.removeFromLeft(numberColumnWidth).withHeight(lineHeight), Justification::centredRight);

            row.layout.draw(g, content.withTrimmedLeft(columnGap));
        }
    }

    void resized() override
    {
        if (getWidth() != layoutWidth)
            updateLayout();
    }

    void lookAndFeelChanged() override
    {
        layoutWidth = -1;
        for (auto& row : rows)
            row.layoutWidth = -1;
        updateLayout();
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!isRealClickEvent(e))
            return;

        grabKeyboardFocus();

        auto const* row = getRowAt(e.y);
        if (!row) {
            clearSelection();
            return;
        }

        auto const number = row->number;
        if (e.mods.isShiftDown() && findRow(lastClicked)) {
            auto const from = getRowIndex(lastClicked);
            auto const to = getRowIndex(number);
            for (int i = jmin(from, to); i <= jmax(from, to); i++)
                selected.add_unique(rows[i].number);
        } else if (e.mods.isCommandDown()) {
            if (!selected.remove_all(number))
                selected.add(number);
        } else if (!(e.mods.isPopupMenu() && selected.contains(number))) {
            selected.clear();
            selected.add(number);
        }

        lastClicked = number;
        repaint();

        if (e.mods.isPopupMenu()) {
            PopupMenu menu;
            menu.addItem("Copy", [this] { copySelection(); });
            menu.showMenuAsync(PopupMenu::Options().withMousePosition());
        }
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key == KeyPress('c', ModifierKeys::commandModifier, 0)) {
            copySelection();
            return true;
        }
        if (key == KeyPress('a', ModifierKeys::commandModifier, 0)) {
            for (auto const& row : rows)
                selected.add_unique(row.number);
            repaint();
            return true;
        }
        return false;
    }

    void focusLost(FocusChangeType) override
    {
        clearSelection();
    }

private:
    void updateLayout()
    {
        layoutWidth = getWidth();

        auto const numberFont = Fonts::getTabularNumbersFont().withHeight(12.0f);
        numberColumnWidth = 0.0f;
        for (auto const& row : rows)
            numberColumnWidth = jmax(numberColumnWidth, std::ceil(Fonts::getStringWidth(String(row.number), numberFont)));

        auto const& colours = getThemeColours(*this);
        auto const wrapWidth = jmax(20, layoutWidth - 2 * (rowInset + rowPaddingX) - static_cast<int>(numberColumnWidth + columnGap));

        int y = listPadding;
        for (auto& row : rows) {
            if (row.layoutWidth != wrapWidth) {
                row.layout = createLayout(row.text, colours.sidebarTextColour, wrapWidth);
                row.layoutWidth = wrapWidth;
            }

            row.y = y;
            row.height = static_cast<int>(std::ceil(jmax(lineHeight, row.layout.getHeight()))) + 2 * rowPaddingY + 2;
            y += row.height;
        }

        setSize(getWidth(), jmax(y + listPadding, viewport.getMaximumVisibleHeight()));
    }

    static TextLayout createLayout(String const& text, Colour const colour, int const wrapWidth)
    {
        auto create = [&](AttributedString::WordWrap const wordWrap) {
            AttributedString attributedText;
            attributedText.setWordWrap(wordWrap);
            attributedText.append(text, Fonts::getDefaultFont().withHeight(14.0f), colour);

            TextLayout layout;
            layout.createLayout(attributedText, static_cast<float>(wrapWidth));
            return layout;
        };

        // Break long words too, so a long symbol doesn't run out of the frame
        auto layout = create(AttributedString::byWord);
        if (layout.getWidth() > static_cast<float>(wrapWidth))
            layout = create(AttributedString::byChar);
        return layout;
    }

    Row const* findRow(uint64 const number) const
    {
        auto const index = getRowIndex(number);
        return index >= 0 ? &rows[index] : nullptr;
    }

    int getRowIndex(uint64 const number) const
    {
        for (int i = 0; i < rows.size(); i++) {
            if (rows[i].number == number)
                return i;
        }
        return -1;
    }

    Row const* getRowAt(int const y) const
    {
        for (auto const& row : rows) {
            if (y >= row.y && y < row.y + row.height)
                return &row;
        }
        return nullptr;
    }

    void copySelection() const
    {
        StringArray lines;
        for (auto const& row : rows) {
            if (selected.contains(row.number))
                lines.add(row.text);
        }
        if (!lines.isEmpty())
            SystemClipboard::copyTextToClipboard(lines.joinIntoString("\n"));
    }

    static constexpr int listPadding = 4;
    static constexpr int rowInset = 4;
    static constexpr int rowPaddingX = 8;
    static constexpr int rowPaddingY = 3;
    static constexpr float lineHeight = 17.0f;
    static constexpr float columnGap = 10.0f;

    Viewport& viewport;
    SmallArray<Row> rows;
    SmallArray<uint64> selected;
    uint64 lastClicked = 0;
    float numberColumnWidth = 0.0f;
    int layoutWidth = -1;
};


// Debugger panel view that traces the messages through a connection: the selected one, or the pinned one.
// While it's showing, it records the messages through every connection in this window, so a connection
// shows what went through it before it was selected. Pd only reports them while debug visualization is on
class TraceView final : public Component
    , private pd::RawMessageListener
    , private Timer {
public:
    explicit TraceView(PluginEditor* editor);
    ~TraceView() override;

    void setSelectedConnection(Connection* connection);
    Connection* getSelectedConnection() const { return selectedConnection.getComponent(); }

    // Records and updates itself while it's active
    void setActive(bool shouldBeActive);

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;
    void resized() override;

private:
    struct Entry {
        uint64 number;
        DetectiveMode::Message message;
    };

    struct History {
        Component::SafePointer<Connection> connection;
        bool isInThisWindow = false;
        std::deque<Entry> messages;
        uint64 count = 0;
    };

    void receiveRawMessage(void* target, t_symbol* symbol, SmallArray<pd::Atom> const& atoms) override;
    void timerCallback() override;
    void update();
    void updateMessages();

    // The pinned connection, or else the selected one
    Connection* getTracedConnection() const;

    PluginEditor* editor;
    Component::SafePointer<Connection> selectedConnection;
    Component::SafePointer<Connection> pinnedConnection;

    UnorderedMap<Connection*, History> histories;
    bool active = false;
    double lastCleanupMs = 0.0;

    SmallIconButton pinButton { Icons::Pin };
    DebuggerButton turnOnButton { "Turn on", true };
    std::unique_ptr<MessageList> messageList;
    BouncingViewport viewport;

    String label;
    String connectionName;
    String placeholder;
    Connection* shownConnection = nullptr;
    uint64 shownCount = 0;
    size_t shownSize = 0;

    Rectangle<int> labelArea, connectionArea, listFrame;

    static constexpr size_t maxHistory = 100;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TraceView)
};

TraceView::TraceView(PluginEditor* e)
    : editor(e)
{
    pinButton.onClick = [this] {
        pinnedConnection = pinnedConnection ? nullptr : getTracedConnection();
        update();
    };
    addAndMakeVisible(pinButton);

    turnOnButton.setTooltip("Turn on debug visualization");
    turnOnButton.onClick = [] {
        SettingsFile::getInstance()->setProperty("debug_visualization", true);
    };
    addChildComponent(turnOnButton);

    messageList = std::make_unique<MessageList>(viewport);
    viewport.setViewedComponent(messageList.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    update();
}

TraceView::~TraceView()
{
    setActive(false);
}

void TraceView::setSelectedConnection(Connection* connection)
{
    selectedConnection = connection;
    update();
}

Connection* TraceView::getTracedConnection() const
{
    return pinnedConnection ? pinnedConnection.getComponent() : selectedConnection.getComponent();
}

void TraceView::setActive(bool const shouldBeActive)
{
    if (shouldBeActive == active)
        return;

    active = shouldBeActive;
    if (active) {
        editor->pd->messageDispatcher->addRawMessageListener(this);
        update();
        startTimerHz(15);
    } else {
        editor->pd->messageDispatcher->removeRawMessageListener(this);
        stopTimer();
    }
}

void TraceView::receiveRawMessage(void* target, t_symbol* symbol, SmallArray<pd::Atom> const& atoms)
{
    // Every Connection listens to its Pd connection, so the dispatcher knows which ones are open
    auto const* listeners = editor->pd->messageDispatcher->getMessageListeners(target);
    if (!listeners)
        return;

    std::optional<DetectiveMode::Message> message;
    for (auto const& listener : *listeners) {
        auto* connection = dynamic_cast<Connection*>(listener.get());
        if (!connection)
            continue;

        // A new entry, or one left behind by a deleted connection at the same address
        auto& history = histories[connection];
        if (history.connection != connection) {
            history = {};
            history.connection = connection;
            history.isInThisWindow = editor->isParentOf(connection);
        }
        if (!history.isInThisWindow)
            continue;

        if (!message)
            message.emplace(symbol, atoms);

        history.messages.push_back({ ++history.count, *message });
        if (history.messages.size() > maxHistory)
            history.messages.pop_front();
    }
}

void TraceView::timerCallback()
{
    auto const now = Time::getMillisecondCounterHiRes();
    if (now - lastCleanupMs > 1000.0) {
        lastCleanupMs = now;
        for (auto it = histories.begin(); it != histories.end();) {
            if (it->second.connection)
                ++it;
            else
                it = histories.erase(it);
        }
    }

    update();
}

void TraceView::update()
{
    auto const* connection = getTracedConnection();
    auto const isPinned = pinnedConnection != nullptr;
    pinButton.setEnabled(connection != nullptr);
    pinButton.setToggleState(isPinned, dontSendNotification);
    pinButton.setButtonText(isPinned ? Icons::Pinned : Icons::Pin);
    pinButton.setTooltip(isPinned ? "Unpin, to trace the selected connection" : "Pin this connection, to keep tracing it when the selection changes");

    String newConnectionName;
    if (connection) {
        // Object and message boxes by their text, others like GUIs by their type
        auto const describe = [](Object const* object) {
            if (!object || !object->gui)
                return String("?");

            bool isText = false;
            if (auto gobj = object->gui->ptr.get<t_gobj>())
                isText = pd::Interface::isTextObject(gobj.get()) && gobj.cast<t_text>()->te_type != T_ATOM;
            return isText ? object->gui->getText().toString() : object->gui->getType().toString();
        };
        newConnectionName = describe(connection->outobj.get()) + String(CharPointer_UTF8(" \xe2\x86\x92 ")) + describe(connection->inobj.get());
    }

    String const newLabel = isPinned ? "Pinned connection" : "Selected connection";
    if (newConnectionName != connectionName || newLabel != label) {
        connectionName = newConnectionName;
        label = newLabel;
        repaint();
    }

    updateMessages();
}

void TraceView::updateMessages()
{
    auto* connection = getTracedConnection();
    auto const it = connection ? histories.find(connection) : histories.end();
    auto const* history = it != histories.end() && it->second.connection == connection ? &it->second : nullptr;
    auto const count = history ? history->count : 0;
    auto const size = history ? history->messages.size() : 0;

    auto const isDebugging = DetectiveMode::isDebuggingEnabled();
    turnOnButton.setVisible(!isDebugging);

    String newPlaceholder;
    if (!isDebugging)
        newPlaceholder = "Turn on debug visualization to trace messages";
    else if (!connection)
        newPlaceholder = "Select a connection to see the messages that go through it";
    else if (size == 0)
        newPlaceholder = "No messages yet";

    if (newPlaceholder != placeholder) {
        placeholder = newPlaceholder;
        repaint();
    }

    if (connection == shownConnection && count == shownCount && size == shownSize)
        return;

    if (connection != shownConnection)
        messageList->clearSelection();

    shownConnection = connection;
    shownCount = count;
    shownSize = size;

    SmallArray<MessageList::Row> rows;
    if (history) {
        rows.reserve(static_cast<int>(size));
        for (auto const& entry : history->messages) {
            MessageList::Row row;
            row.number = entry.number;
            row.text = entry.message.toString();
            rows.add(std::move(row));
        }
    }
    messageList->setRows(std::move(rows));
}

void TraceView::paint(Graphics& g)
{
    auto const& colours = getThemeColours(*this);
    Fonts::drawText(g, label, labelArea, colours.sidebarTextColour.withAlpha(0.6f), 13);

    g.setColour(colours.sidebarActiveBackgroundColour);
    g.fillRoundedRectangle(connectionArea.toFloat(), Corners::largeCornerRadius);
    g.fillRoundedRectangle(listFrame.toFloat(), Corners::largeCornerRadius);

    auto const nameArea = connectionArea.withTrimmedLeft(12).withTrimmedRight(pinButton.getWidth() + 4);
    if (connectionName.isNotEmpty())
        Fonts::drawFittedText(g, connectionName, nameArea, colours.sidebarTextColour, 1, 0.9f, 14.0f, Justification::centredLeft, Semibold);
    else
        Fonts::drawText(g, "None", nameArea, colours.sidebarTextColour.withAlpha(0.45f), 14);
}

void TraceView::paintOverChildren(Graphics& g)
{
    if (placeholder.isEmpty())
        return;

    auto const& colours = getThemeColours(*this);
    Fonts::drawFittedText(g, placeholder, listFrame.reduced(16, 0).withTrimmedTop(18).withHeight(40), colours.sidebarTextColour.withAlpha(0.5f), 2, 1.0f, 13.5f, Justification::centredTop);
}

void TraceView::resized()
{
    auto bounds = getLocalBounds().reduced(8, 0).withTrimmedBottom(8);

    labelArea = bounds.removeFromTop(26).withTrimmedLeft(4);
    connectionArea = bounds.removeFromTop(36);
    pinButton.setBounds(connectionArea.withTrimmedRight(4).removeFromRight(28).withSizeKeepingCentre(28, 28));
    bounds.removeFromTop(8);

    listFrame = bounds;
    viewport.setBounds(listFrame.reduced(0, 2));
    messageList->setSize(viewport.getMaximumVisibleWidth(), messageList->getHeight());
    turnOnButton.setBounds(listFrame.withTrimmedTop(66).withHeight(28).withSizeKeepingCentre(100, 28));
}

DebugPanel::DebugPanel(PluginEditor* editor)
    : viewSelector(std::make_unique<SegmentedControl>(StringArray { "Trace", "Profiler" }))
    , traceView(std::make_unique<TraceView>(editor))
    , profilerView(std::make_unique<ProfilerView>(editor))
{
    viewSelector->setOptionTooltip(0, "The messages that go through a connection");
    viewSelector->setOptionTooltip(1, "How much CPU each object takes");
    viewSelector->onChange = [this](int) {
        updateActiveView();
    };
    addAndMakeVisible(*viewSelector);
    addAndMakeVisible(*traceView);
    addChildComponent(*profilerView);
}

DebugPanel::~DebugPanel() = default;

void DebugPanel::visibilityChanged()
{
    // A view only runs while it can be seen, which also depends on the window, so check that now and then
    if (isVisible())
        startTimerHz(4);
    else
        stopTimer();

    updateActiveView();
}

void DebugPanel::timerCallback()
{
    updateActiveView();
}

void DebugPanel::updateActiveView()
{
    auto const index = viewSelector->getSelectedIndex();
    auto const isShowingPanel = isShowing();
    traceView->setVisible(index == 0);
    profilerView->setVisible(index == 1);
    traceView->setActive(isShowingPanel && index == 0);
    profilerView->setActive(isShowingPanel && index == 1);
}

void DebugPanel::setConnection(Connection* connection)
{
    traceView->setSelectedConnection(connection);
}

Connection* DebugPanel::getConnection() const
{
    return traceView->getSelectedConnection();
}

void DebugPanel::paint(Graphics& g)
{
    g.fillAll(getThemeColours(*this).sidebarBackgroundColour);
}

void DebugPanel::resized()
{
    auto bounds = getLocalBounds();
    viewSelector->setBounds(bounds.removeFromTop(44).reduced(8).withTrimmedBottom(-2));
    traceView->setBounds(bounds);
    profilerView->setBounds(bounds);
}
