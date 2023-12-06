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
        
        // Clear out the buffer from the previous connection
        float sample;
        while(sampleQueue.try_dequeue(sample))
        std::fill(lastSamples, lastSamples + 512, 0.0f);
        
        activeConnection = SafePointer<Connection>(connection);
        if (activeConnection.getComponent()) {
            mousePosition = screenPosition;
            isSignalDisplay = activeConnection->outlet->isSignal;
            startTimer(MouseHoverDelay, mouseDelay);
            stopTimer(MouseHoverExitDelay);
            if(isSignalDisplay)
            {
                auto* pd = activeConnection->outobj->cnv->pd;
                pd->connectionListener = this;
                
                startTimer(RepaintTimer, 1000 / 10);
            }
            else
            {
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
        if(!activeConnection) return;
        
        t_float output[DEFDACBLKSIZE];
        if(!activeConnection->getSignalData(output)) return;
        
        for(auto& sample : output)
        {
            sampleQueue.try_enqueue(sample);
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

        Rectangle<int> proposedPosition;
        // only make the size wider, to fit changing size of values
        if (totalStringWidth > getWidth() || isHoverEntered) {
            proposedPosition.setSize(totalStringWidth, 36);
            // make sure the proposed position is inside the editor area
            proposedPosition.setCentre(getParentComponent()->getLocalPoint(nullptr, mousePosition).translated(0, -(getHeight() * 0.5)));
            constrainedBounds = proposedPosition.constrainedWithin(getParentComponent()->getLocalBounds());
        }

        if (getBounds() != constrainedBounds)
            setBounds(constrainedBounds);

        repaint();
    }
        
    void updateSignalGraph()
    {
        if(activeConnection)
        {
            for(int i = 0; i < 512; i++)
            {
                float sample;
                if(sampleQueue.try_dequeue(sample))
                {
                    lastSamples[i] = sample;
                }
                else {
                    break;
                }
            }
            
            Rectangle<int> proposedPosition(130, 65);
            // make sure the proposed position is inside the editor area
            proposedPosition.setCentre(getParentComponent()->getLocalPoint(nullptr, mousePosition).translated(0, -(getHeight() * 0.5)));
            constrainedBounds = proposedPosition.constrainedWithin(getParentComponent()->getLocalBounds());
            
            if (getBounds() != constrainedBounds)
                setBounds(constrainedBounds);
            
            repaint();
        }
    }

    void hideDisplay()
    {
        if(activeConnection)  {
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
                if(isSignalDisplay) {
                    updateSignalGraph();
                }
                else {
                    updateTextString();
                }
            } else {
                hideDisplay();
            }
            break;
        }
        case MouseHoverDelay: {
            if (activeConnection.getComponent()) {
                if(!isSignalDisplay) {
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

        if(isSignalDisplay)
        {
            Point<float> lastPoint = {10.0f, jmap<float>(lastSamples[0], -1.0f, 1.0f, internalBounds.getY(), internalBounds.getBottom())};
            
            Path oscopePath;
            for (int x = internalBounds.getX(); x < internalBounds.getRight(); x++) {
                auto index = jmap<int>(x, internalBounds.getX(), internalBounds.getRight(), 0, 512);
                auto y = jmap<float>(lastSamples[index], -1.0f, 1.0f, internalBounds.getY(), internalBounds.getBottom());
                auto newPoint = Point<float>(x, y);
                auto segment = Line(lastPoint, newPoint);
                oscopePath.addLineSegment(segment, 0.5f);
                lastPoint = newPoint;
            }
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.fillPath(oscopePath);
            
            auto textBounds = internalBounds.reduced(4).removeFromBottom(16).removeFromRight(32);
            
            g.setColour(findColour(PlugDataColour::dialogBackgroundColourId).withAlpha(0.5f));
            g.fillRoundedRectangle(textBounds, Corners::defaultCornerRadius);
            
            g.setColour(findColour(PlugDataColour::canvasTextColourId));
            g.setFont(Fonts::getTabularNumbersFont().withHeight(11.5f));
            g.drawText(String(lastSamples[rand() % 512], 3), textBounds.toNearestInt(), Justification::centred);
        }
        else {
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

    float lastSamples[512];
    moodycamel::ReaderWriterQueue<float> sampleQueue = moodycamel::ReaderWriterQueue<float>(512);
    
    bool isSignalDisplay;
    Image cachedImage;
    Rectangle<int> previousBounds;
};
