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
// In screen pixels: the bubble keeps its size at any zoom
constexpr float bubblePaddingX = 8.0f;
constexpr float bubblePaddingY = 5.0f;
constexpr float bubbleFontHeight = 13.5f;
constexpr float bubbleMaxWidth = 420.0f;
constexpr float bubbleArrowSize = 5.0f;
constexpr float bubbleGap = 3.0f;
constexpr float dotRadius = 4.0f;
constexpr float dotHaloRadius = 8.0f;

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

// It plays where it can be seen: on a canvas that shows it in its current mode, not in a tab in the background
bool canAnimate(Connection const* connection)
{
    if (!connection->isShowing())
        return false;
    auto const* cnv = connection->findParentComponentOfClass<Canvas>();
    return cnv && cnv->shouldShowMessageAnimation();
}
}

DetectiveMode::DetectiveMode(PluginProcessor* processor)
    : pd(processor)
    , chainSymbol(processor->generateSymbol("_chain"))
{
    speed.referTo(SettingsFile::getInstance()->getPropertyAsValue("detective_speed"));
    setEnabled(isDebuggingEnabled() && isAnimationRequested());
    updatePdDebugging();
}

DetectiveMode::~DetectiveMode()
{
    pd->messageDispatcher->removeRawMessageListener(this);
}

bool DetectiveMode::isDebuggingEnabled()
{
    return SettingsFile::getInstance()->getProperty<bool>("debug_visualization");
}

bool DetectiveMode::isVisualizationEnabled(String const& option)
{
    return isDebuggingEnabled() && SettingsFile::getInstance()->getProperty<bool>(option);
}

bool DetectiveMode::isAnimationRequested()
{
    auto const overlays = SettingsFile::getInstance()->getProperty<DynamicObject>("overlays");
    for (auto const* mode : { "edit", "lock", "alt" }) {
        if (static_cast<int>(overlays->getProperty(mode)) & MessageAnimation)
            return true;
    }
    return false;
}

void DetectiveMode::updatePdDebugging()
{
    set_plugdata_debugging_enabled(isDebuggingEnabled());
}

void DetectiveMode::settingsChanged(String const& name, var const&)
{
    if (name == "debug_visualization")
        updatePdDebugging();
    if (name == "debug_visualization" || name == "overlays")
        setEnabled(isDebuggingEnabled() && isAnimationRequested());
}

void DetectiveMode::setEnabled(bool const shouldBeEnabled)
{
    if (shouldBeEnabled == enabled)
        return;

    enabled = shouldBeEnabled;
    if (enabled) {
        pd->messageDispatcher->addRawMessageListener(this);
    } else {
        pd->messageDispatcher->removeRawMessageListener(this);
        stopTimer();
        stopMessage();
        clearPending();
        chainSource = nullptr;
        paused = false;
        stepping = false;
    }
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

void DetectiveMode::clearPending()
{
    pending.clear();
    numPending = 0;
    isPlayingChain = false;
}

DetectiveMode::Message::Message(t_symbol* symbol, SmallArray<pd::Atom> const& messageAtoms)
    : selector(symbol)
    , truncated(messageAtoms.size() > maxAtoms)
{
    atoms.append(messageAtoms.begin(), messageAtoms.begin() + jmin<int>(messageAtoms.size(), maxAtoms));
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

SmallArray<Connection*, 2> DetectiveMode::findConnections(void* connection) const
{
    // Every Connection listens to its Pd connection, so the dispatcher knows which ones are open
    SmallArray<Connection*, 2> connections;
    if (auto const* listeners = pd->messageDispatcher->getMessageListeners(connection)) {
        for (auto const& listener : *listeners) {
            if (auto* openConnection = dynamic_cast<Connection*>(listener.get()))
                connections.add(openConnection);
        }
    }
    return connections;
}

void DetectiveMode::receiveRawMessage(void* target, t_symbol* symbol, SmallArray<pd::Atom> const& atoms)
{
    if (symbol == chainSymbol) {
        chainSource = target;
        chainCounter++;
        return;
    }

    // Connections of patches that aren't open can't play it
    auto const connections = findConnections(target);
    if (!std::ranges::any_of(connections, canAnimate) || numPending >= maxPendingMessages)
        return;

    addPendingMessage(target, Message(symbol, atoms));
    if (!isTimerRunning()) {
        lastTickMs = Time::getMillisecondCounterHiRes();
        startTimerHz(60);
    }
}

void DetectiveMode::addPendingMessage(void* connection, Message message)
{
    if (pending.empty() || pending.back().id != chainCounter) {
        // A chain replaces the waiting one from the same source, so a [metro] or a dragged slider can't build up a backlog
        for (auto it = pending.begin() + (isPlayingChain ? 1 : 0); it != pending.end();) {
            if (chainSource && it->source == chainSource) {
                numPending -= it->messages.size();
                it = pending.erase(it);
            } else {
                ++it;
            }
        }
        pending.push_back({ chainCounter, chainSource, {} });
    }

    pending.back().messages.push_back({ connection, std::move(message) });
    numPending++;
}

void DetectiveMode::startNextMessage()
{
    while (!pending.empty()) {
        auto& chain = pending.front();
        auto next = std::move(chain.messages.front());
        chain.messages.pop_front();
        numPending--;

        // The rest of a chain that started playing can't be replaced
        isPlayingChain = !chain.messages.empty();
        if (!isPlayingChain)
            pending.pop_front();

        // Its connection can have been hidden or deleted since it was sent
        SmallArray<Bubble, 2> newBubbles;
        for (auto* connection : findConnections(next.connection)) {
            if (canAnimate(connection)) {
                Bubble bubble;
                bubble.canvas = connection->findParentComponentOfClass<Canvas>();
                bubble.connection = connection;
                newBubbles.add(std::move(bubble));
            }
        }

        if (newBubbles.empty())
            continue;

        // Long cables take longer, so the message doesn't race along them
        auto const length = newBubbles[0].connection->getPath().getLength();
        durationMs = jlimit(minDurationMs, maxDurationMs, length * durationPerPixelMs);

        current = std::move(next.message);
        currentText = current->toString(maxBubbleTextLength);
        bubbles = std::move(newBubbles);
        progress = 0.0f;
        return;
    }
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
    // After a stall, carry on from where the message was instead of jumping ahead
    auto const now = Time::getMillisecondCounterHiRes();
    auto const elapsedMs = jmin(static_cast<float>(now - lastTickMs), 50.0f);
    lastTickMs = now;

    if (current) {
        auto const exists = std::any_of(bubbles.begin(), bubbles.end(), [](Bubble const& bubble) {
            return bubble.connection != nullptr;
        });
        auto const isShowing = std::any_of(bubbles.begin(), bubbles.end(), [](Bubble const& bubble) {
            return bubble.connection && canAnimate(bubble.connection);
        });

        if (isShowing && (!paused || stepping))
            progress += elapsedMs * jlimit(0.1f, 5.0f, getValue<float>(speed)) / durationMs;

        // Skip the message once its connection is deleted, or can't be seen while playing (another tab, a mode without the animation).
        // While paused, it waits to be seen again
        if (!exists || (!isShowing && !paused) || progress >= 1.0f) {
            stopMessage();
            stepping = false;
        }
    }

    if (!current)
        startNextMessage();

    updateBubbles();

    // Nothing left to show: the next message starts the timer again
    if (!current)
        stopTimer();
}

bool DetectiveMode::updateBubble(Bubble& bubble)
{
    auto* cnv = bubble.canvas.getComponent();
    auto* connection = bubble.connection.getComponent();
    if (!cnv || !connection || !cnv->viewport || !canAnimate(connection))
        return false;

    auto const& path = connection->getPath();
    auto const visibleArea = cnv->viewport->getViewArea();
    auto const zoom = getValue<float>(cnv->zoomScale);
    if (path.isEmpty() || visibleArea.isEmpty() || zoom <= 0.0f)
        return false;

    // Scrolled out of view: a bubble at the edge of the view would point at nothing
    bubble.dot = path.getPointAlongPath(jlimit(0.0f, 1.0f, progress) * path.getLength());
    if (!visibleArea.contains(bubble.dot))
        return false;

    // Wrap the text so the widest bubble still fits in the view
    auto const maxBubbleWidth = jlimit(60.0f, bubbleMaxWidth, visibleArea.getWidth() * zoom - 16.0f);
    auto const wrapWidth = jmax(1, static_cast<int>(maxBubbleWidth - 2.0f * bubblePaddingX));
    auto const textColour = getThemeColours(*cnv).popupMenuTextColour;
    if (bubble.layoutWidth != wrapWidth || bubble.layoutColour != textColour) {
        bubble.layout = createBubbleLayout(currentText, textColour, wrapWidth);
        bubble.layoutWidth = wrapWidth;
        bubble.layoutColour = textColour;
    }

    bubble.scale = 1.0f / zoom;

    // Centred above the dot, or below it if only that has room, and inside the view
    auto const size = (Point<float>(std::ceil(bubble.layout.getWidth()), std::ceil(bubble.layout.getHeight())) + Point<float>(bubblePaddingX, bubblePaddingY) * 2.0f) * bubble.scale;
    auto const offset = (dotRadius + bubbleGap + bubbleArrowSize) * bubble.scale;
    bubble.isBelowDot = bubble.dot.y - offset - size.y < visibleArea.getY() && bubble.dot.y + offset + size.y <= visibleArea.getBottom();

    auto const y = bubble.isBelowDot ? bubble.dot.y + offset : bubble.dot.y - offset - size.y;
    bubble.box = Rectangle<float>(bubble.dot.x - size.x * 0.5f, y, size.x, size.y).constrainedWithin(visibleArea.reduced(4.0f * bubble.scale));
    return true;
}

void DetectiveMode::updateBubbles()
{
    for (auto& bubble : bubbles) {
        Rectangle<int> area;
        if (updateBubble(bubble)) {
            auto const haloSize = dotHaloRadius * 2.0f * bubble.scale;
            auto const dotArea = Rectangle<float>(haloSize, haloSize).withCentre(bubble.dot);
            area = bubble.box.getUnion(dotArea).expanded((bubbleArrowSize + 3.0f) * bubble.scale).getSmallestIntegerContainer();
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

        // A dot on the cable: a halo, and a ring that sets it apart from a cable of the same colour
        auto const drawDot = [nvg, &bubble](float const radius, Colour const colour) {
            nanovg::nvgBeginPath(nvg);
            nanovg::nvgFillColor(nvg, nvgColour(colour));
            nanovg::nvgCircle(nvg, bubble.dot.x, bubble.dot.y, radius * bubble.scale);
            nanovg::nvgFill(nvg);
        };
        drawDot(dotHaloRadius, colours.dataColour.withAlpha(0.25f));
        drawDot(dotRadius + 1.5f, colours.canvasBackgroundColour);
        drawDot(dotRadius, colours.dataColour);

        // The bubble is drawn in screen pixels, from its top left corner
        nanovg::nvgTranslate(nvg, bubble.box.getX(), bubble.box.getY());
        nanovg::nvgScale(nvg, bubble.scale, bubble.scale);
        auto const width = bubble.box.getWidth() / bubble.scale;
        auto const height = bubble.box.getHeight() / bubble.scale;

        // Looks like a tooltip, with an arrow that points at the dot
        constexpr auto cornerRadius = Corners::defaultCornerRadius;
        auto const arrowX = jlimit(cornerRadius + bubbleArrowSize, jmax(cornerRadius + bubbleArrowSize, width - cornerRadius - bubbleArrowSize), (bubble.dot.x - bubble.box.getX()) / bubble.scale);
        auto const arrowBase = bubble.isBelowDot ? 0.0f : height;
        auto const arrowDirection = bubble.isBelowDot ? -1.0f : 1.0f;
        auto const drawShape = [&](float const outline, Colour const colour) {
            nanovg::nvgBeginPath(nvg);
            nanovg::nvgRoundedRect(nvg, -outline, -outline, width + outline * 2.0f, height + outline * 2.0f, cornerRadius + outline);
            nanovg::nvgMoveTo(nvg, arrowX - bubbleArrowSize - outline, arrowBase - arrowDirection);
            nanovg::nvgLineTo(nvg, arrowX, arrowBase + arrowDirection * (bubbleArrowSize + outline * 1.4f));
            nanovg::nvgLineTo(nvg, arrowX + bubbleArrowSize + outline, arrowBase - arrowDirection);
            nanovg::nvgClosePath(nvg);
            nanovg::nvgFillColor(nvg, nvgColour(colour));
            nanovg::nvgFill(nvg);
        };
        drawShape(1.0f, colours.outlineColour);
        drawShape(0.0f, colours.popupMenuBackgroundColour);

        auto const textBounds = Rectangle<float>(width, height).reduced(bubblePaddingX, bubblePaddingY);
        if (auto* llgc = cnv->editor->getNanoLLGC(); llgc && !textBounds.isEmpty()) {
            Graphics g(*llgc);
            NVGGraphicsContext::ScopedAnchoredDraw anchor(*llgc, textBounds);
            bubble.layout.draw(g, textBounds);
        }
    }
}
