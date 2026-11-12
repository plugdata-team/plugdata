/*
 // Copyright (c) 2026 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "DetectiveMode.h"

#include <nanovg_async.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Canvas.h"
#include "CanvasViewport.h"
#include "Connection.h"
#include "LookAndFeel.h"
#include "Utility/Fonts.h"
#include "Utility/NVGGraphicsContext.h"

namespace {
constexpr float bubblePaddingX = 8.0f;
constexpr float bubblePaddingY = 6.0f;
constexpr float bubbleOutlineWidth = 1.0f;
constexpr float bubbleFontHeight = 13.0f;
constexpr float bubbleGap = 10.0f;
constexpr float bubbleMaxWidth = 560.0f;
constexpr float bubbleCornerRadius = 6.0f;

// The message's text, broken between words, or inside a word that's too long for the bubble
TextLayout createBubbleLayout(String const& text, Colour const colour, int const wrapWidth)
{
    auto createLayout = [&](AttributedString::WordWrap const wordWrap) {
        AttributedString attributedText;
        attributedText.setJustification(Justification::topLeft);
        attributedText.setWordWrap(wordWrap);
        attributedText.append(text, Fonts::getDefaultFont().withHeight(bubbleFontHeight), colour);

        TextLayout layout;
        layout.createLayout(attributedText, static_cast<float>(wrapWidth));
        return layout;
    };

    auto layout = createLayout(AttributedString::byWord);
    if (layout.getWidth() > static_cast<float>(wrapWidth))
        layout = createLayout(AttributedString::byChar);

    return layout;
}

// Next to the dot: to its right unless only its left has room, and above it unless only below has room, inside the view
Rectangle<float> placeBubble(Rectangle<float> const& visibleArea, Point<float> const dot, Point<float> const size, float const offset)
{
    auto const fitsRight = dot.x + offset + size.x <= visibleArea.getRight() || dot.x - offset - size.x < visibleArea.getX();
    auto const fitsAbove = dot.y - offset - size.y >= visibleArea.getY() || dot.y + offset + size.y > visibleArea.getBottom();

    auto const x = fitsRight ? dot.x + offset : dot.x - offset - size.x;
    auto const y = fitsAbove ? dot.y - offset - size.y : dot.y + offset;
    return Rectangle<float>(x, y, size.x, size.y).constrainedWithin(visibleArea);
}
}

DetectiveMode::DetectiveMode(PluginProcessor* processor)
    : pd(processor)
{
    speed.referTo(SettingsFile::getInstance()->getPropertyAsValue("detective_speed"));
    setEnabled(SettingsFile::getInstance()->getProperty<bool>("detective_mode"));
}

DetectiveMode::~DetectiveMode()
{
    if (traceCollector) {
        pd->lockAudioThread();
        register_trace_trigger(static_cast<t_pdinstance*>(pd->instance), nullptr, nullptr);
        pd->unlockAudioThread();
    }
}

void DetectiveMode::updatePdDebugging()
{
    auto* settings = SettingsFile::getInstance();
    set_plugdata_debugging_enabled(settings->getProperty<bool>("debug_connections") || settings->getProperty<bool>("detective_mode"));
}

void DetectiveMode::settingsChanged(String const& name, var const& value)
{
    if (name == "detective_mode")
        setEnabled(static_cast<bool>(value));
}

void DetectiveMode::setEnabled(bool const shouldBeEnabled)
{
    if (shouldBeEnabled == isEnabled())
        return;

    // Pd only calls the trace trigger with its lock held, so (un)registering under the lock means it never sees a collector being created or deleted
    auto* instance = static_cast<t_pdinstance*>(pd->instance);
    if (shouldBeEnabled) {
        pd->lockAudioThread();
        traceCollector = std::make_unique<pd::ConnectionTraceCollector>(gensym("_activity"));
        register_trace_trigger(instance, traceCollector.get(), &pd::ConnectionTraceCollector::enqueue);
        pd->unlockAudioThread();

        lastTickMs = Time::getMillisecondCounterHiRes();
        startTimerHz(60);
    } else {
        stopTimer();
        pd->lockAudioThread();
        register_trace_trigger(instance, nullptr, nullptr);
        pd->unlockAudioThread();

        traceCollector.reset();
        stopMessage();
        pending.clear();
        history.clear();
        openConnections.clear();
        openConnectionsAreCurrent = false;
        paused = false;
        stepping = false;
    }

    updatePdDebugging();
}

void DetectiveMode::setPaused(bool const shouldPause)
{
    paused = shouldPause;
    stepping = false;
}

void DetectiveMode::step()
{
    paused = true;
    stepping = current.has_value();
}

std::deque<DetectiveMode::Message> const* DetectiveMode::getHistory(t_outconnect* connection) const
{
    auto const it = history.find(connection);
    return it != history.end() ? &it->second : nullptr;
}

String DetectiveMode::Message::toString(int const maxLength) const
{
    auto text = selector ? String::fromUTF8(selector->s_name) : String();
    bool cut = truncated;
    for (auto const& atom : atoms) {
        if (maxLength >= 0 && text.length() >= maxLength) {
            cut = true;
            break;
        }
        text << ' ' << atom.toString();
    }

    if (maxLength >= 0 && text.length() > maxLength) {
        text = text.substring(0, maxLength);
        cut = true;
    }
    if (cut)
        text << String(CharPointer_UTF8(" \xe2\x80\xa6"));

    return text;
}

UnorderedMap<t_outconnect*, SmallArray<std::pair<Canvas*, Connection*>, 1>> const& DetectiveMode::getOpenConnections()
{
    if (openConnectionsAreCurrent)
        return openConnections;

    openConnections.clear();
    for (auto* editor : pd->getEditors()) {
        for (auto* cnv : editor->getCanvases()) {
            for (auto* connection : cnv->connections) {
                if (auto* oc = connection->getPointer())
                    openConnections[oc].add({ cnv, connection });
            }
        }
    }

    // Forget the messages of connections that were deleted, or whose patch was closed
    for (auto it = history.begin(); it != history.end();) {
        if (openConnections.contains(it->first))
            ++it;
        else
            it = history.erase(it);
    }

    openConnectionsAreCurrent = true;
    return openConnections;
}

void DetectiveMode::collectMessages()
{
    traceCollector->dequeue([this](pd::ConnectionTraceEvent const& event) {
        // Messages in patches that aren't open can't be shown or selected
        if (!getOpenConnections().contains(event.connection))
            return;

        Message message;
        message.sequenceNumber = event.sequenceNumber;
        message.selector = event.selector;
        message.truncated = event.truncated;
        for (int i = 0; i < event.numAtoms; i++)
            message.atoms.emplace_back(event.atoms + i);

        auto& connectionHistory = history[event.connection];
        connectionHistory.push_back(message);
        if (connectionHistory.size() > maxHistory)
            connectionHistory.pop_front();

        if (pending.size() < maxPendingMessages)
            pending.push_back({ event.connection, std::move(message) });
    });
}

bool DetectiveMode::startNextMessage()
{
    while (!pending.empty()) {
        auto next = std::move(pending.front());
        pending.pop_front();

        auto const& connections = getOpenConnections();
        auto const it = connections.find(next.connection);
        if (it == connections.end())
            continue;

        // Only show it where it can be seen: not in a tab in the background, or in presentation mode
        SmallArray<Bubble, 2> newBubbles;
        for (auto const& [cnv, connection] : it->second) {
            if (connection->isShowing()) {
                Bubble bubble;
                bubble.canvas = cnv;
                bubble.connection = connection;
                newBubbles.add(std::move(bubble));
            }
        }

        if (newBubbles.empty())
            continue;

        current = std::move(next.message);
        currentText = current->toString(maxBubbleTextLength);
        bubbles = std::move(newBubbles);
        progress = 0.0f;
        return true;
    }

    return false;
}

void DetectiveMode::stopMessage()
{
    for (auto const& bubble : bubbles) {
        if (auto* cnv = bubble.canvas.getComponent())
            cnv->repaint(bubble.area);
    }

    bubbles.clear();
    current.reset();
    currentText.clear();
    progress = 0.0f;
}

void DetectiveMode::timerCallback()
{
    openConnectionsAreCurrent = false;

    // After a stall, carry on from where the message was instead of jumping ahead
    auto const now = Time::getMillisecondCounterHiRes();
    auto const elapsedMs = jmin(static_cast<float>(now - lastTickMs), 50.0f);
    lastTickMs = now;

    collectMessages();

    if (current) {
        auto const exists = std::any_of(bubbles.begin(), bubbles.end(), [](Bubble const& bubble) {
            return bubble.connection != nullptr;
        });
        auto const isShowing = std::any_of(bubbles.begin(), bubbles.end(), [](Bubble const& bubble) {
            return bubble.connection && bubble.connection->isShowing();
        });

        if (isShowing && (!paused || stepping))
            progress += elapsedMs * jlimit(0.1f, 5.0f, getValue<float>(speed)) / messageDurationMs;

        // Skip the message once its connection is deleted, or can't be seen while playing (another tab, presentation mode).
        // While paused, it waits to be seen again
        if (!exists || (!isShowing && !paused) || progress >= 1.0f) {
            stopMessage();
            stepping = false;
        }
    }

    if (!current)
        startNextMessage();

    updateBubbles();
}

bool DetectiveMode::updateBubble(Bubble& bubble)
{
    auto* cnv = bubble.canvas.getComponent();
    auto* connection = bubble.connection.getComponent();
    if (!cnv || !connection || !cnv->viewport || !connection->isShowing())
        return false;

    auto const& path = connection->getPath();
    auto const visibleArea = cnv->viewport->getViewArea();
    if (path.isEmpty() || visibleArea.isEmpty())
        return false;

    // Wrap the text so the widest bubble still fits in the view
    auto const maxBubbleWidth = jlimit(60.0f, bubbleMaxWidth, visibleArea.getWidth() - 16.0f);
    auto const wrapWidth = jmax(1, static_cast<int>(maxBubbleWidth - 2.0f * (bubblePaddingX + bubbleOutlineWidth)));
    auto const textColour = getThemeColours(*cnv).canvasTextColour;
    if (bubble.layoutWidth != wrapWidth || bubble.layoutColour != textColour) {
        bubble.layout = createBubbleLayout(currentText, textColour, wrapWidth);
        bubble.layoutWidth = wrapWidth;
        bubble.layoutColour = textColour;
    }

    auto const size = Point<float>(std::ceil(bubble.layout.getWidth()), std::ceil(bubble.layout.getHeight())) + Point<float>(bubblePaddingX + bubbleOutlineWidth, bubblePaddingY + bubbleOutlineWidth) * 2.0f;

    bubble.cableWidth = connection->getPathWidth();
    bubble.dot = path.getPointAlongPath(jlimit(0.0f, 1.0f, progress) * path.getLength());
    bubble.box = placeBubble(visibleArea, bubble.dot, size, jmax(bubbleGap, bubble.cableWidth + 4.0f));
    return true;
}

void DetectiveMode::updateBubbles()
{
    for (auto& bubble : bubbles) {
        Rectangle<int> area;
        if (updateBubble(bubble)) {
            auto const dotArea = Rectangle<float>(bubble.cableWidth * 2.0f, bubble.cableWidth * 2.0f).withCentre(bubble.dot);
            area = bubble.box.getUnion(dotArea).expanded(3.0f).getSmallestIntegerContainer();
        }

        // Redraw where it was and where it is now
        if (area != bubble.area) {
            if (auto* cnv = bubble.canvas.getComponent())
                cnv->repaint(bubble.area.getUnion(area));
            bubble.area = area;
        }
    }
}

void DetectiveMode::render(Canvas const* cnv, NVGcontext* nvg, Rectangle<int> const invalidRegion) const
{
    for (auto const& bubble : bubbles) {
        if (bubble.canvas.getComponent() != cnv || bubble.area.isEmpty() || !bubble.area.intersects(invalidRegion))
            continue;

        auto const& colours = getThemeColours(*cnv);
        NVGScopedState scopedState(nvg);

        auto const& box = bubble.box;
        nanovg::nvgDrawRoundedRect(nvg, box.getX(), box.getY(), box.getWidth(), box.getHeight(), nvgColour(colours.canvasBackgroundColour.withAlpha(0.95f)), nvgColour(colours.outlineColour), bubbleCornerRadius);

        auto const textBounds = box.reduced(bubblePaddingX + bubbleOutlineWidth, bubblePaddingY + bubbleOutlineWidth);
        if (auto* llgc = cnv->editor->getNanoLLGC(); llgc && !textBounds.isEmpty()) {
            Graphics g(*llgc);
            NVGGraphicsContext::ScopedAnchoredDraw anchor(*llgc, textBounds);
            bubble.layout.draw(g, textBounds);
        }

        // A dot on the cable, with a glow around it
        auto const drawDot = [nvg, dot = bubble.dot](float const radius, Colour const colour) {
            nanovg::nvgBeginPath(nvg);
            nanovg::nvgFillColor(nvg, nvgColour(colour));
            nanovg::nvgCircle(nvg, dot.x, dot.y, radius);
            nanovg::nvgFill(nvg);
        };
        drawDot(bubble.cableWidth, Colours::white.withAlpha(0.6f));
        drawDot(bubble.cableWidth * 0.75f, Colours::white);
        drawDot(bubble.cableWidth * 0.45f, colours.dataColour);
    }
}
