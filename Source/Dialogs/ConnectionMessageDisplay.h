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
#include "CanvasViewport.h"

class ConnectionMessageDisplay
    : public Component
    , public MultiTimer {
public:
    ConnectionMessageDisplay()
    {
        setSize(36, 36);
        setVisible(false);
        // needed to stop the component from gaining mouse focus
        setInterceptsMouseClicks(false, false);
    }

    ~ConnectionMessageDisplay()
        override
        = default;

    /** Activate the current connection info display overlay, to hide give it a nullptr
     */
    void setConnection(Connection* connection, Point<int> screenPosition = { 0, 0 })
    {
        // multiple events can hide the display, so we don't need to do anything
        // if this object has already been set to null
        if (activeConnection == nullptr && connection == nullptr)
            return;

        auto clearSignalDisplayBuffer = [this](){
            SignalBlock sample;
            while (sampleQueue.try_dequeue(sample)) {};
            for(int ch = 0; ch < 8; ch++) {
                std::fill(lastSamples[ch], lastSamples[ch] + signalBlockSize, 0.0f);
                cycleLength[ch] = 0.0f;
            }
        };

        activeConnection = SafePointer<Connection>(connection);
        if (activeConnection.getComponent()) {
            mousePosition = screenPosition;
            isSignalDisplay = activeConnection->outlet->isSignal;
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

        auto halfEditorWidth = getParentComponent()->getWidth() / 2;
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
            stringWidth = textFont.getStringWidth(stringItem);

            if ((totalStringWidth + stringWidth) > halfEditorWidth) {
                auto elideText = String("(" + String(textString.size() - i) + String(")..."));
                auto elideFont = Font(Fonts::getSemiBoldFont());
                auto elideWidth = elideFont.getStringWidth(elideText);
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
        repaint();
    }

    void updateBoundsFromProposed(Rectangle<int> proposedPosition)
    {
       // make sure the proposed position is inside the editor area
        proposedPosition.setCentre(getParentComponent()->getLocalPoint(nullptr, mousePosition).translated(0, -(getHeight() * 0.5)));
        constrainedBounds = proposedPosition.constrainedWithin(getParentComponent()->getLocalBounds());
        if (getBounds() != constrainedBounds)
            setBounds(constrainedBounds);
    }

    void updateSignalGraph()
    {
        if (activeConnection) {
            int i = 0;
            SignalBlock block;
            while (sampleQueue.try_dequeue(block)) {
                if(i < numBlocks) {
                    lastNumChannels = block.numChannels;
                    for (int ch = 0; ch < block.numChannels; ch++) {
                        std::copy(block.samples + ch * DEFDACBLKSIZE, block.samples + ch * DEFDACBLKSIZE + DEFDACBLKSIZE, lastSamples[ch] + (i * DEFDACBLKSIZE));
                    }
                }
                i++;
            }
            
            auto newBounds = Rectangle<int>(130, jmap<int>(lastNumChannels, 1, 8, 50, 150));
            oscilloscopeImage = renderOscilloscope(newBounds);
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
        
    Image renderOscilloscope(Rectangle<int> bounds)
    {
        Image oscopeImage(Image::ARGB, bounds.getRight() * 2.0f, bounds.getBottom() * 2.0f, true);
        Graphics g(oscopeImage);
        g.addTransform(AffineTransform::scale(2.0f)); // So it will also look sharp on hi-dpi screens
        
        auto totalHeight = bounds.getHeight();
        auto textColour = findColour(PlugDataColour::canvasTextColourId);
        
        int complexFFTSize = signalBlockSize * 2;
        for (int ch = 0; ch < lastNumChannels; ch++) {
            
            auto channelBounds = bounds.removeFromTop(totalHeight / std::max(lastNumChannels, 1)).reduced(5).toFloat();
            
            auto peakAmplitude = *std::max_element(lastSamples[ch], lastSamples[ch] + signalBlockSize);
            auto valleyAmplitude = *std::min_element(lastSamples[ch], lastSamples[ch] + signalBlockSize);
            
            // Audio was empty, draw a line and continue, no need to perform an fft
            if(approximatelyEqual(peakAmplitude, 0.0f) && approximatelyEqual(valleyAmplitude, 0.0f))
            {
                auto textBounds = channelBounds.expanded(5).removeFromBottom(24).removeFromRight(34);
                g.setColour(textColour);
                g.drawHorizontalLine(channelBounds.getCentreY(), channelBounds.getX(), channelBounds.getRight());
                                
                // Draw text background
                g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
                g.fillRoundedRectangle(textBounds, Corners::defaultCornerRadius);
                
                // Draw text
                g.setColour(textColour);
                g.setFont(Fonts::getTabularNumbersFont().withHeight(12.f));
                g.drawText("0.000", textBounds.toNearestInt(), Justification::centred);
                continue;
            }
                        
            if(peakAmplitude == valleyAmplitude)
            {
                peakAmplitude += 0.01f;
                valleyAmplitude -= 0.01f;
            }
            
            // Apply FFT to get the peak frequency, we use this to decide the amount of samples we display
            float fftBlock[complexFFTSize];
            std::copy(lastSamples[ch], lastSamples[ch] + signalBlockSize, fftBlock);
            signalDisplayFFT.performRealOnlyForwardTransform(fftBlock);
            
            float maxMagnitude = 0.0f;
            int peakFreqIndex = 0;
            for (int i = 0; i < signalBlockSize; i++) {
                auto binMagnitude = std::hypot(fftBlock[i*2], fftBlock[i*2+1]);
                if(binMagnitude > maxMagnitude)
                {
                    maxMagnitude = binMagnitude;
                    peakFreqIndex = i;
                }
            }
            
            auto samplesPerCycle = std::clamp<int>(round(static_cast<float>(signalBlockSize * 2) / peakFreqIndex), 8, signalBlockSize);
            // Keep a short average of cycle length over time to prevent sudden changes
            cycleLength[ch] = jmap<float>(0.5f, cycleLength[ch], samplesPerCycle);
            
            auto phase = atan2(fftBlock[peakFreqIndex * 2 + 1], fftBlock[peakFreqIndex * 2]) + M_PI;
            auto phaseShiftSamples = static_cast<int>(phase / (2.0 * M_PI) * samplesPerCycle);
            if (phaseShiftSamples > 0 && phaseShiftSamples < signalBlockSize) {
                //std::rotate(lastSamples[ch], lastSamples[ch] + phaseShiftSamples, lastSamples[ch] + signalBlockSize);
            }
            
            Point<float> lastPoint = { channelBounds.getX(), jmap<float>(lastSamples[ch][0], valleyAmplitude, peakAmplitude, channelBounds.getY(), channelBounds.getBottom()) };
            
            Path oscopePath;
            for (int x = channelBounds.getX() + 1; x < channelBounds.getRight(); x++) {
                auto index = jmap<float>(x, channelBounds.getX(), channelBounds.getRight(), 0, samplesPerCycle);
                
                // linearl interpolation, especially needed for high-frequency signals
                auto roundedIndex = static_cast<int>(index);
                auto currentSample = lastSamples[ch][roundedIndex];
                auto nextSample = roundedIndex == 1023 ? lastSamples[ch][roundedIndex] : lastSamples[ch][roundedIndex + 1];
                auto interpolatedSample = jmap<float>(index - roundedIndex, currentSample, nextSample);
                
                auto y = jmap<float>(interpolatedSample, valleyAmplitude, peakAmplitude, channelBounds.getY(), channelBounds.getBottom());
                auto newPoint = Point<float>(x, y);
                auto segment = Line(lastPoint, newPoint);
                oscopePath.addLineSegment(segment, 0.75f);
                lastPoint = newPoint;
            }
            
            // Draw oscope path
            g.setColour(textColour);
            g.fillPath(oscopePath);

            // Calculate text length
            auto numbersFont = Fonts::getTabularNumbersFont().withHeight(12.f);
            auto text = String(lastSamples[ch][rand() % 512], 3);
            auto textWidth = numbersFont.getStringWidth(text);
            auto textBounds = channelBounds.expanded(5).removeFromBottom(24).removeFromRight(textWidth + 8);
            
            // Draw text background
            g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
            g.fillRoundedRectangle(textBounds, Corners::defaultCornerRadius);
            
            // Draw text
            g.setColour(textColour);
            g.setFont(numbersFont);
            g.drawText(text, textBounds.toNearestInt(), Justification::centred);
        }
        
        return oscopeImage;
    }

    void paint(Graphics& g) override
    {

        Path messageDisplay;
        auto internalBounds = getLocalBounds().reduced(8).toFloat();
        messageDisplay.addRoundedRectangle(internalBounds, Corners::defaultCornerRadius);

        if (cachedImage.isNull() || previousBounds != getBounds()) {
            cachedImage = { Image::ARGB, getWidth(), getHeight(), true };
            Graphics g2(cachedImage);

            StackShadow::renderDropShadow(g2, messageDisplay, Colour(0, 0, 0).withAlpha(0.3f), 6);
        }

        g.setColour(Colours::black);
        g.drawImageAt(cachedImage, 0, 0);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.fillRoundedRectangle(internalBounds.expanded(1), Corners::defaultCornerRadius);
        g.setColour(findColour(PlugDataColour::dialogBackgroundColourId));
        g.fillRoundedRectangle(internalBounds, Corners::defaultCornerRadius);

        // indicator - TODO
        // if(activeConnection.getComponent()) {
        //    Path indicatorPath;
        //    indicatorPath.addPieSegment(circlePosition.x - circleRadius,
        //                          circlePosition.y - circleRadius,
        //                          circleRadius * 2.0f,
        //                          circleRadius * 2.0f, 0, (activeConnection->messageActivity * (1.0f / 12.0f)) * MathConstants<float>::twoPi, 0.5f);
        //    g.setColour(findColour(PlugDataColour::panelTextColourId));
        //    g.fillPath(indicatorPath);
        //}

        if (isSignalDisplay) {
            g.drawImage(oscilloscopeImage, internalBounds);
        } else {
            int startPostionX = 8 + 4;
            for (auto const& item : messageItemsWithFormat) {
                Fonts::drawStyledText(g, item.text, startPostionX, 0, item.width, getHeight(), findColour(PlugDataColour::panelTextColourId), item.fontStyle, 14, Justification::centredLeft);
                startPostionX += item.width + 4;
            }
        }

        // used for cached background shadow
        previousBounds = getBounds();
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
    enum TimerID { RepaintTimer,
        MouseHoverDelay,
        MouseHoverExitDelay };
    Rectangle<int> constrainedBounds = { 0, 0, 0, 0 };

    Point<float> circlePosition = { 8.0f + 4.0f, 36.0f / 2.0f };
    
    Image cachedImage;
    Rectangle<int> previousBounds;
    
    struct SignalBlock
    {
        SignalBlock() : numChannels(0)
        {
        }
        
        SignalBlock(float* input, int channels) : numChannels(channels)
        {
            std::copy(input, input + (numChannels * DEFDACBLKSIZE), samples);
        }
        
        SignalBlock(SignalBlock&& toMove)
        {
            numChannels = toMove.numChannels;
            std::copy(toMove.samples, toMove.samples + (numChannels * DEFDACBLKSIZE), samples);
        }
        
        SignalBlock& operator=(SignalBlock&& toMove)
        {
            if(&toMove != this) {
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
