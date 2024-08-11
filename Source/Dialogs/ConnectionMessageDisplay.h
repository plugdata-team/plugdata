/*
 // Copyright (c) 2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>
#include "Constants.h"
#include "LookAndFeel.h"
#include "Connection.h"
#include "PluginEditor.h"
#include "Object.h"

class ConnectionMessageDisplay
    : public Component
    , public MultiTimer {

    PluginEditor* editor;

public:
    ConnectionMessageDisplay(PluginEditor* parentEditor)
        : editor(parentEditor)
    {
        setSize(36, 36);
        setVisible(false);
        // needed to stop the component from gaining mouse focus
        setInterceptsMouseClicks(false, false);
        setBufferedToImage(true);
    }

    ~ConnectionMessageDisplay()
        override
        = default;
        

    // Activate the current connection info display overlay, to hide give it a nullptr
    void setConnection(Connection* connection, Point<int> screenPosition = { 0, 0 })
    {
        // multiple events can hide the display, so we don't need to do anything
        // if this object has already been set to null
        if (activeConnection == nullptr && connection == nullptr)
            return;

        auto clearSignalDisplayBuffer = [this]() {
            SignalBlock sample;
            while (sampleQueue.try_dequeue(sample)) { }
            for (int ch = 0; ch < 8; ch++) {
                std::fill(lastSamples[ch], lastSamples[ch] + signalBlockSize, 0.0f);
                cycleLength[ch] = 0.0f;
            }
        };

        activeConnection = SafePointer<Connection>(connection);

        if (activeConnection.getComponent()) {
            mousePosition = screenPosition;
            isSignalDisplay = activeConnection->outlet->isSignal;
            lastNumChannels = std::min(activeConnection->numSignalChannels, 7);
            startTimer(MouseHoverDelay, mouseDelay);
            stopTimer(MouseHoverExitDelay);
            if (isSignalDisplay) {
                clearSignalDisplayBuffer();
                auto* pd = activeConnection->outobj->cnv->pd;
                pd->connectionListener = this;
                startTimer(RepaintTimer, 1000 / 5);
                updateSignalGraph();
            } else {
                startTimer(RepaintTimer, 1000 / 60);
                updateTextString(true);
            }
        } else {
            hideDisplay();
            // to copy tooltip behaviour, any successful interaction will cause the next interaction to have no delay
            mouseDelay = 0;
            stopTimer(MouseHoverDelay);
            startTimer(MouseHoverExitDelay, 500);
        }
    }

    void updateSignalData()
    {
        if (!activeConnection)
            return;

        float output[DEFDACBLKSIZE * 8];
        if (auto numChannels = activeConnection->getSignalData(output, 8)) {
            sampleQueue.try_enqueue(SignalBlock(output, numChannels));
        }
    }

private:
    void updateTextString(bool isHoverEntered = false)
    {
        messageItemsWithFormat.clear();

        auto haveMessage = true;
        auto textString = activeConnection->getMessageFormated();

        if (textString[0].isEmpty()) {
            haveMessage = false;
            textString = StringArray("no message yet");
        }

        auto halfEditorWidth = editor->getWidth() / 2;
        auto fontStyle = haveMessage ? FontStyle::Semibold : FontStyle::Regular;
        auto textFont = Font(haveMessage ? Fonts::getSemiBoldFont() : Fonts::getDefaultFont());
        textFont.setSizeAndStyle(14, FontStyle::Regular, 1.0f, 0.0f);

        int stringWidth;
        int totalStringWidth = (8 * 2) + 4;
        String stringItem;
        for (int i = 0; i < textString.size(); i++) {
            auto firstOrLast = (i == 0 || i == textString.size() - 1);
            stringItem = textString[i];
            stringItem += firstOrLast ? "" : ",";

            // first item uses system font
            // use cached width calculation for performance
            stringWidth = CachedFontStringWidth::get()->calculateSingleLineWidth(textFont, stringItem);

            if ((totalStringWidth + stringWidth) > halfEditorWidth) {
                auto elideText = String("(" + String(textString.size() - i) + String(")..."));
                auto elideFont = Font(Fonts::getSemiBoldFont());

                auto elideWidth = CachedFontStringWidth::get()->calculateSingleLineWidth(elideFont, elideText);
                messageItemsWithFormat.add(TextStringWithMetrics(elideText, FontStyle::Semibold, elideWidth));
                totalStringWidth += elideWidth + 4;
                break;
            }

            // calculate total needed width
            totalStringWidth += stringWidth + 4;

            messageItemsWithFormat.add(TextStringWithMetrics(stringItem, fontStyle, stringWidth));

            if (fontStyle != FontStyle::Regular) {
                // set up font for next item/s -regular font to support extended character
                fontStyle = FontStyle::Regular;
                textFont = Font(Fonts::getDefaultFont());
            }
        }

        // only make the size wider, to fit changing size of values
        if (totalStringWidth > getWidth() || isHoverEntered) {
            updateBoundsFromProposed(Rectangle<int>().withSize(totalStringWidth, 36));
        }

        // Check if changed
        if (lastTextString != textString) {
            lastTextString = textString;
            repaint();
        }
    }

    void updateBoundsFromProposed(Rectangle<int> proposedPosition)
    {
        // make sure the proposed position is inside the editor area
        proposedPosition.setCentre(mousePosition.translated(0, -(getHeight() * 0.5)));
        constrainedBounds = proposedPosition.constrainedWithin(editor->getScreenBounds());
        if (getBounds() != constrainedBounds)
            setBounds(constrainedBounds);
    }

    void updateSignalGraph()
    {
        if (activeConnection) {
            int i = 0;
            SignalBlock block;
            while (sampleQueue.try_dequeue(block)) {
                if (i < numBlocks) {
                    lastNumChannels = std::min(block.numChannels, 7);
                    for (int ch = 0; ch < lastNumChannels; ch++) {
                        std::copy(block.samples + ch * DEFDACBLKSIZE, block.samples + ch * DEFDACBLKSIZE + DEFDACBLKSIZE, lastSamples[ch] + (i * DEFDACBLKSIZE));
                    }
                }
                i++;
            }

            auto newBounds = Rectangle<int>(130, jmap<int>(lastNumChannels, 1, 8, 50, 150));
            updateBoundsFromProposed(newBounds);
            repaint();
        }
    }

    void hideDisplay()
    {
        if (activeConnection) {
            auto* pd = activeConnection->outobj->cnv->pd;
            pd->connectionListener = nullptr;
        }
        stopTimer(RepaintTimer);
        setVisible(false);
        activeConnection = nullptr;
    }

    void timerCallback(int timerID) override
    {
        switch (timerID) {
        case RepaintTimer: {
            if (activeConnection.getComponent()) {
                if (isSignalDisplay) {
                    updateSignalGraph();
                } else {
                    updateTextString();
                }
            } else {
                hideDisplay();
            }
            break;
        }
        case MouseHoverDelay: {
            if (activeConnection.getComponent()) {
                if (!isSignalDisplay) {
                    updateTextString();
                }
                setVisible(true);
            } else {
                hideDisplay();
            }
            break;
        }
        case MouseHoverExitDelay: {
            mouseDelay = 500;
            stopTimer(MouseHoverExitDelay);
            break;
        }
        default:
            break;
        }
    }

    void paint(Graphics& g) override
    {
        Path messageDisplay;
        auto internalBounds = getLocalBounds().reduced(8).toFloat();
        messageDisplay.addRoundedRectangle(internalBounds, Corners::defaultCornerRadius);

        StackShadow::renderDropShadow(g, messageDisplay, Colour(0, 0, 0).withAlpha(0.3f), 7);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.fillRoundedRectangle(internalBounds.expanded(1), Corners::defaultCornerRadius);
        g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
        g.fillRoundedRectangle(internalBounds, Corners::defaultCornerRadius);

        if (isSignalDisplay) {
            auto totalHeight = internalBounds.getHeight();
            auto textColour = findColour(PlugDataColour::canvasTextColourId);

            constexpr int complexFFTSize = signalBlockSize * 2;
            for (int ch = 0; ch < lastNumChannels; ch++) {

                auto channelBounds = internalBounds.removeFromTop(totalHeight / std::max(lastNumChannels, 1)).reduced(5).toFloat();

                auto peakAmplitude = *std::max_element(lastSamples[ch], lastSamples[ch] + signalBlockSize);
                auto valleyAmplitude = *std::min_element(lastSamples[ch], lastSamples[ch] + signalBlockSize);

                // Audio was empty, draw a line and continue, no need to perform an fft
                if (approximatelyEqual(peakAmplitude, 0.0f) && approximatelyEqual(valleyAmplitude, 0.0f)) {
                    auto textBounds = channelBounds.expanded(5).removeFromBottom(18).removeFromRight(34);
                    g.setColour(textColour);
                    g.drawHorizontalLine(channelBounds.getCentreY(), channelBounds.getX(), channelBounds.getRight());

                    // Draw text background
                    g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
                    g.fillRoundedRectangle(textBounds, Corners::defaultCornerRadius);

                    // Draw text
                    g.setColour(textColour);
                    g.setFont(Fonts::getTabularNumbersFont().withHeight(11.f));
                    g.drawText("0.000", textBounds.toNearestInt(), Justification::centred);
                    continue;
                }

                while(approximatelyEqual(peakAmplitude, valleyAmplitude)) {
                    peakAmplitude += peakAmplitude * 0.001f;
                    valleyAmplitude -= valleyAmplitude * 0.001f;
                }

                // Apply FFT to get the peak frequency, we use this to decide the amount of samples we display
                float fftBlock[complexFFTSize];
                std::copy(lastSamples[ch], lastSamples[ch] + signalBlockSize, fftBlock);
                signalDisplayFFT.performRealOnlyForwardTransform(fftBlock);

                float maxMagnitude = 0.0f;
                int peakFreqIndex = 0;
                for (int i = 0; i < signalBlockSize; i++) {
                    auto binMagnitude = std::hypot(fftBlock[i * 2], fftBlock[i * 2 + 1]);
                    if (binMagnitude > maxMagnitude) {
                        maxMagnitude = binMagnitude;
                        peakFreqIndex = i;
                    }
                }

                auto samplesPerCycle = std::clamp<int>(round(static_cast<float>(signalBlockSize * 2) / peakFreqIndex), 8, signalBlockSize);
                // Keep a short average of cycle length over time to prevent sudden changes
                cycleLength[ch] = jmap<float>(0.5f, cycleLength[ch], samplesPerCycle);

                Point<float> lastPoint = { channelBounds.getX(), jmap<float>(lastSamples[ch][0], valleyAmplitude, peakAmplitude, channelBounds.getY(), channelBounds.getBottom()) };

                Path oscopePath;
                for (int x = channelBounds.getX() + 1; x < channelBounds.getRight(); x++) {
                    auto index = jmap<float>(x, channelBounds.getX(), channelBounds.getRight(), 0, samplesPerCycle);

                    // linear interpolation, especially needed for high-frequency signals
                    auto roundedIndex = static_cast<int>(index);
                    auto currentSample = lastSamples[ch][roundedIndex];
                    auto nextSample = roundedIndex == 1023 ? lastSamples[ch][roundedIndex] : lastSamples[ch][roundedIndex + 1];
                    auto interpolatedSample = jmap<float>(index - roundedIndex, currentSample, nextSample);

                    auto y = jmap<float>(interpolatedSample, valleyAmplitude, peakAmplitude, channelBounds.getY(), channelBounds.getBottom());
                    auto newPoint = Point<float>(x, y);
                    if(newPoint.isFinite()) {
                        auto segment = Line(lastPoint, newPoint);
                        oscopePath.addLineSegment(segment, 0.75f);
                        lastPoint = newPoint;
                    }
                }

                // Draw oscope path
                g.setColour(textColour);
                g.fillPath(oscopePath);

                // Calculate text length
                auto numbersFont = Fonts::getTabularNumbersFont().withHeight(11.f);
                auto text = String(lastSamples[ch][rand() % 512], 3);
                auto textWidth = CachedFontStringWidth::get()->calculateSingleLineWidth(numbersFont, text);
                auto textBounds = channelBounds.expanded(5).removeFromBottom(18).removeFromRight(textWidth + 8);

                // Draw text background
                g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
                g.fillRoundedRectangle(textBounds, Corners::defaultCornerRadius);

                // Draw text
                g.setColour(textColour);
                g.setFont(numbersFont);
                g.drawText(text, textBounds.toNearestInt(), Justification::centred);
            }
        } else {
            int startPositionX = 8 + 4;
            for (auto const& item : messageItemsWithFormat) {
                Fonts::drawStyledText(g, item.text, startPositionX, 0, item.width, getHeight(), findColour(PlugDataColour::panelTextColourId), item.fontStyle, 14, Justification::centredLeft);
                startPositionX += item.width + 4;
            }
        }
    }

    static inline bool isShowing = false;

    struct TextStringWithMetrics {
        TextStringWithMetrics(String text, FontStyle fontStyle, int width)
            : text(std::move(text))
            , fontStyle(fontStyle)
            , width(width)
        {
        }
        String text;
        FontStyle fontStyle;
        int width;
    };

    Array<TextStringWithMetrics> messageItemsWithFormat;

    Component::SafePointer<Connection> activeConnection;
    int mouseDelay = 500;
    Point<int> mousePosition;
    StringArray lastTextString;
    enum TimerID { RepaintTimer,
        MouseHoverDelay,
        MouseHoverExitDelay };
    Rectangle<int> constrainedBounds = { 0, 0, 0, 0 };

    struct SignalBlock {
        SignalBlock()
            : numChannels(0)
        {
        }

        SignalBlock(float const* input, int channels)
            : numChannels(channels)
        {
            std::copy(input, input + (numChannels * DEFDACBLKSIZE), samples);
        }

        SignalBlock(SignalBlock&& toMove) noexcept
        {
            numChannels = toMove.numChannels;
            std::copy(toMove.samples, toMove.samples + (numChannels * DEFDACBLKSIZE), samples);
        }

        SignalBlock& operator=(SignalBlock&& toMove) noexcept
        {
            if (&toMove != this) {
                numChannels = toMove.numChannels;
                std::copy(toMove.samples, toMove.samples + (numChannels * DEFDACBLKSIZE), samples);
            }
            return *this;
        }

        float samples[32 * DEFDACBLKSIZE];
        int numChannels;
    };

    Image oscilloscopeImage;
    static constexpr int signalBlockSize = 1024;
    static constexpr int numBlocks = 1024 / 64;
    // 32*64 samples gives us space for 1024 samples across 8 channels
    moodycamel::ReaderWriterQueue<SignalBlock> sampleQueue = moodycamel::ReaderWriterQueue<SignalBlock>(numBlocks);

    float cycleLength[8] = { 0.0f };
    float lastSamples[8][1024] = { { 0.0f } };
    int lastNumChannels = 1;

    dsp::FFT signalDisplayFFT = dsp::FFT(10);
    bool isSignalDisplay;
};
