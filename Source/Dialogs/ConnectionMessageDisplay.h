#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
#include "LookAndFeel.h"
#include "../Connection.h"
#include "../PluginEditor.h"

class ConnectionMessageDisplay 
    : public Component
    , public MultiTimer {
public:
    ConnectionMessageDisplay(PluginEditor* editor)
        : editor(editor)
    {
        setSize(200,36);
        startTimer(RepaintTimer, 1000/60);
        setVisible(false);
        // needed to stop the component from gaining mouse focus
        setInterceptsMouseClicks(false, false);
    }

    ~ConnectionMessageDisplay()
    {
    }

    /** Set the current connection show message display overlay, to clear give it a nullptr
    */
    void setConnection(Connection* connection)
    {
        activeConnection = SafePointer<Connection>(connection);
        if (activeConnection.getComponent()) {
            auto canvasConnectionPath = activeConnection->toDraw;
            setCentrePosition(getParentComponent()->getLocalPoint(nullptr, connection->mouseHoverPos).translated(0, -(getHeight() * 0.5)));
            startTimer(MouseHoverDelay, mouseDelay);
            stopTimer(MouseHoverExitDelay);
            updateTextString(true);
        }
        else {
            setVisible(false);
            stopTimer(MouseHoverDelay);
            // to copy tooltip behaviour, any successful interaction will cause the next interaction to have no delay
            mouseDelay = 0;
            startTimer(MouseHoverExitDelay, 500);
        }
    }

private:
    void updateTextString(bool isHoverEntered = false)
    {
        textString = activeConnection->getMessageFormated();

        if (textString[0].isEmpty()) {
            haveMessage = false;
            textString = StringArray("no message yet");
        } else
            haveMessage = true;

        //Font textFont(Fonts::getCurrentFont());
        //auto stringWidth = textFont.withHeight(14).getStringWidth(textString);
        // add margin and padding
        //stringWidth += (8 * 2) + (4 + 4);

        // only make the size wider, to fit changing values
        //if (stringWidth > getWidth() || isHoverEntered)
        //    setSize(stringWidth, 36);

        repaint();
    }

    void timerCallback(int timerID) override
    {
        switch (timerID) {
        case RepaintTimer: {
            if (activeConnection.getComponent())
                updateTextString();
            break;
            }
        case MouseHoverDelay: {
            if (activeConnection.getComponent())
                setVisible(activeConnection.getComponent());
            break;
        }
        case MouseHoverExitDelay: {
            mouseDelay = 500;
            stopTimer(MouseHoverExitDelay);
            break;
        }
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
        //if(activeConnection.getComponent()) {
        //    Path indicatorPath;
        //    indicatorPath.addPieSegment(circlePosition.x - circleRadius,
        //                          circlePosition.y - circleRadius,
        //                          circleRadius * 2.0f,
        //                          circleRadius * 2.0f, 0, (activeConnection->messageActivity * (1.0f / 12.0f)) * MathConstants<float>::twoPi, 0.5f);
        //    g.setColour(findColour(PlugDataColour::panelTextColourId));
        //    g.fillPath(indicatorPath);
        //}
        bool firstStringProcessed = false;
        int startPostionX = 8 + 4;
        for (auto stringItem : textString) {
            if (!firstStringProcessed) {
                auto fontStyle = haveMessage ? FontStyle::Bold : FontStyle::Regular;
                Font textFont(Fonts::getCurrentFont().getTypefaceName(), 14, fontStyle);
                auto stringWidth = textFont.getStringWidth(stringItem);
                Fonts::drawStyledText(g, stringItem, startPostionX, 0, stringWidth, getHeight(), findColour(PlugDataColour::panelTextColourId), fontStyle, 14, Justification::centredLeft);

                startPostionX += stringWidth + 4;
                firstStringProcessed = true;
            }
            else {
                auto stringWidth = stringItem.length() * 8;
                Fonts::drawStyledText(g, stringItem, startPostionX, 0, stringWidth, getHeight(), findColour(PlugDataColour::panelTextColourId), FontStyle::Monospace, 14, Justification::centredLeft);
                startPostionX += stringWidth + 4;
            }
        }

        previousBounds = getBounds();
    }

    PluginEditor* editor;
    static inline bool isShowing = false;

    StringArray textString;
    bool haveMessage = false;

    Component::SafePointer<Connection> activeConnection;
    int mouseDelay = 500;
    enum TimerID {RepaintTimer, MouseHoverDelay, MouseHoverExitDelay};

    Point<float> circlePosition = { 8 + 4, 36 / 2 };
    float circleRadius = 3.0f;

    Image cachedImage;
    Rectangle<int> previousBounds;
};
