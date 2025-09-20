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
#include "Utility/CachedStringWidth.h"

class ConnectionMessageDisplay final
    : public Component
    , public MultiTimer {

    PluginEditor* editor;

public:
    explicit ConnectionMessageDisplay(PluginEditor* parentEditor)
        : editor(parentEditor), pd(parentEditor->pd), connectionPtr(nullptr)
    {
        setSize(36, 36);
        setVisible(false);
        setBufferedToImage(true);
    }

    ~ConnectionMessageDisplay() override
    {
        if(connectionPtr) {
            pd->unregisterWeakReference(connectionPtr, &weakRef);
        }
        editor->pd->connectionListener = nullptr;
    }

    bool hitTest(int x, int y) override
    {
        return false;
    }

    // Activate the current connection info display overlay, to hide give it a nullptr
    void setConnection(Connection* connection, Point<int> screenPosition = { 0, 0 })
    {
        // multiple events can hide the display, so we don't need to do anything
        // if this object has already been set to null
        if (connectionPtr == nullptr && connection == nullptr)
            return;

        if (editor->pluginMode || (connection && connection->inobj && getValue<bool>(connection->inobj->cnv->presentationMode))) {
            hideDisplay();
            return;
        }

        auto clearSignalDisplayBuffer = [this] {
            SignalBlock sample;
            while (sampleQueue.try_dequeue(sample)) { }
            for (int ch = 0; ch < 8; ch++) {
                std::fill_n(lastSamples[ch], signalBlockSize, 0.0f);
                cycleLength[ch] = 0.0f;
            }
        };

        // So we can safely assign activeConnection
        activeConnection = connection;

        if (connection) {
            connectionPtr = connection->getPointer();
            weakRef = true;
            pd->registerWeakReference(connectionPtr, &weakRef);
            mousePosition = screenPosition;
            isSignalDisplay = connection->outlet->isSignal;
            lastNumChannels = std::min(connection->numSignalChannels, 7);
            startTimer(MouseHoverDelay, mouseDelay);
            stopTimer(MouseHoverExitDelay);
            if (isSignalDisplay) {
                clearSignalDisplayBuffer();
                editor->pd->connectionListener = this;
                startTimer(RepaintTimer, 1000 / 5);
                updateSignalGraph();
            } else {
                startTimer(RepaintTimer, 1000 / 60);
                updateTextString(true);
            }
        } else {
            if(connectionPtr) {
                pd->unregisterWeakReference(connectionPtr, &weakRef);
            }
            hideDisplay();
            connectionPtr = nullptr;
            // to copy tooltip behaviour, any successful interaction will cause the next interaction to have no delay
            mouseDelay = 0;
            stopTimer(MouseHoverDelay);
            startTimer(MouseHoverExitDelay, 500);
            editor->pd->connectionListener = nullptr;
        }
    }

    void updateSignalData()
    {
        const ScopedTryLock sl (pd->audioLock);
        
        if (sl.isLocked() && connectionPtr != nullptr && weakRef) {
            if (auto const* signal = outconnect_get_signal(connectionPtr.load())) {
                auto const numChannels = std::min(signal->s_nchans, 8);
                auto const numSamples = signal->s_n;
                auto const blockSize = std::min(pdBlockSize, numSamples);
                auto const blocks = numSamples / blockSize;
                
                if(numChannels > 0) {
                    auto* samples = signal->s_vec;
                    if (!samples)
                        return;
                    
                    for(int block = 0; block < blocks; block++)
                    {
                        StackArray<float, 512> output;
                        for(int ch = 0; ch < numChannels; ch++)
                        {
                            auto* start = samples + (ch * numSamples + block * blockSize);
                            auto* destination = output.data() + (ch * blockSize);
                            std::copy_n(start, blockSize, destination);
                        }
                        sampleQueue.try_enqueue(SignalBlock(std::move(output), numChannels, blockSize));
                    }
                }
            }
        }
    }

private:
    void updateTextString(bool const isHoverEntered = false)
    {
        messageItemsWithFormat.clear();

        if (connectionPtr == nullptr || !activeConnection)
            return;
        
        auto haveMessage = true;
        auto textString = activeConnection->getMessageFormated();

        if (textString[0].isEmpty()) {
            haveMessage = false;
            textString = StringArray("no message yet");
        }

        auto const halfEditorWidth = editor->getWidth() / 2;
        auto fontStyle = haveMessage ? FontStyle::Semibold : FontStyle::Regular;
        auto textFont = Font(haveMessage ? Fonts::getSemiBoldFont() : Fonts::getDefaultFont());
        textFont.setSizeAndStyle(14, FontStyle::Regular, 1.0f, 0.0f);

        int totalStringWidth = 8 * 2 + 4;
        for (int i = 0; i < textString.size(); i++) {
            auto const firstOrLast = i == 0 || i == textString.size() - 1;
            String stringItem = textString[i];
            stringItem += firstOrLast ? "" : ",";

            // first item uses system font
            // use cached width calculation for performance
            int const stringWidth = CachedFontStringWidth::get()->calculateSingleLineWidth(textFont, stringItem);

            if (totalStringWidth + stringWidth > halfEditorWidth) {
                auto const elideText = String("(" + String(textString.size() - i) + String(")..."));
                auto const elideFont = Font(Fonts::getSemiBoldFont());

                auto const elideWidth = CachedFontStringWidth::get()->calculateSingleLineWidth(elideFont, elideText);
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
        proposedPosition.setPosition(mousePosition.translated(0, -getHeight()));
        constrainedBounds = proposedPosition.constrainedWithin(editor->getScreenBounds());
        if (getBounds() != constrainedBounds)
            setBounds(constrainedBounds);
    }

    void updateSignalGraph()
    {
        if (connectionPtr != nullptr && activeConnection) {
            int i = 0;
            SignalBlock block;
            while (sampleQueue.try_dequeue(block)) {
                if (i < signalBlockSize) {
                    lastNumChannels = block.numChannels;
                    for (int ch = 0; ch < lastNumChannels; ch++) {
                        std::copy_n(block.samples.begin() + ch * block.numSamples, block.numSamples, lastSamples[ch] + i);
                    }
                    i += block.numSamples;
                }
            }

            auto const newBounds = Rectangle<int>(130, jmap<int>(lastNumChannels, 1, 8, 50, 150));
            updateBoundsFromProposed(newBounds);
            repaint();
        }
    }

    void hideDisplay()
    {
        stopTimer(RepaintTimer);
        setVisible(false);
        if (connectionPtr != nullptr && activeConnection) {
            auto* pd = activeConnection->outobj->cnv->pd;
            pd->connectionListener = nullptr;
            connectionPtr = nullptr;
            activeConnection = nullptr;
        }
    }

    void timerCallback(int const timerID) override
    {
        switch (timerID) {
        case RepaintTimer: {
            if (connectionPtr != nullptr && activeConnection) {
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
            if (connectionPtr != nullptr && activeConnection) {
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

        StackShadow::renderDropShadow(hash("connection_message_display"), g, messageDisplay, Colour(0, 0, 0).withAlpha(0.3f), 7);

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
                
                
                if(!std::isfinite(peakAmplitude) || !std::isfinite(valleyAmplitude))
                {
                    peakAmplitude = 0;
                    valleyAmplitude = 1;
                }
                
                while (peakAmplitude < valleyAmplitude || approximatelyEqual(peakAmplitude, valleyAmplitude)) {
                    peakAmplitude += 0.001f;
                    valleyAmplitude -= 0.001f;
                }

                // Apply FFT to get the peak frequency, we use this to decide the amount of samples we display
                float fftBlock[complexFFTSize];
                std::copy_n(lastSamples[ch], signalBlockSize, fftBlock);
                signalDisplayFFT.performRealOnlyForwardTransform(fftBlock);

                float maxMagnitude = 0.0f;
                int peakFreqIndex = 1;
                for (int i = 0; i < signalBlockSize; i++) {
                    auto binMagnitude = std::hypot(fftBlock[i * 2], fftBlock[i * 2 + 1]);
                    if (binMagnitude > maxMagnitude) {
                        maxMagnitude = binMagnitude;
                        peakFreqIndex = i;
                    }
                }
                peakFreqIndex = std::max(peakFreqIndex, 1);

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

                    auto y = jmap<float>(interpolatedSample, valleyAmplitude, peakAmplitude, channelBounds.getBottom(), channelBounds.getY());
                    auto newPoint = Point<float>(x, y);
                    if (newPoint.isFinite()) {
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
        TextStringWithMetrics(String text, FontStyle const fontStyle, int const width)
            : text(std::move(text))
            , fontStyle(fontStyle)
            , width(width)
        {
        }
        String text;
        FontStyle fontStyle;
        int width;
    };

    SmallArray<TextStringWithMetrics, 8> messageItemsWithFormat;
    pd::Instance* pd;
    
    pd_weak_reference weakRef = false;
    AtomicValue<t_outconnect*, Sequential> connectionPtr;
    SafePointer<Connection> activeConnection;
    
    int mouseDelay = 500;
    Point<int> mousePosition;
    StringArray lastTextString;
    enum TimerID { RepaintTimer,
        MouseHoverDelay,
        MouseHoverExitDelay };
    Rectangle<int> constrainedBounds = { 0, 0, 0, 0 };

    struct SignalBlock {
        SignalBlock()
            : numChannels(0), numSamples(0)
        {
        }

        SignalBlock(StackArray<float, 512>&& input, int const channels, int const samples)
            : samples(input), numChannels(channels), numSamples(samples)
        {
        }

        SignalBlock(SignalBlock&& toMove) noexcept
        {
            numChannels = toMove.numChannels;
            numSamples = toMove.numSamples;
            samples = toMove.samples;
        }

        SignalBlock& operator=(SignalBlock&& toMove) noexcept
        {
            if (&toMove != this) {
                numChannels = toMove.numChannels;
                numSamples = toMove.numSamples;
                samples = toMove.samples;
            }
            return *this;
        }

        StackArray<float, 512> samples;
        int numChannels;
        int numSamples;
    };

    Image oscilloscopeImage;
    static constexpr int pdBlockSize = DEFDACBLKSIZE;
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
