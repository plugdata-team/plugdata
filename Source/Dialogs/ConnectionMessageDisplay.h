#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Constants.h"
#include "LookAndFeel.h"
#include "../Connection.h"

class ConnectionMessageDisplay 
    : public Component
    , public MultiTimer {
public:
    ConnectionMessageDisplay()
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
        messageItemsWithFormat.clear();

        auto haveMessage = true;
        auto textString = activeConnection->getMessageFormated();

        if (textString[0].isEmpty()) {
            haveMessage = false;
            textString = StringArray("no message yet");
        }

        auto fontStyle = haveMessage ? FontStyle::Bold : FontStyle::Regular;
        auto textFont = Font(Fonts::getCurrentFont());
        textFont.setSizeAndStyle(14, fontStyle, 1.0f, 0.0f);
        int stringWidth;
        int totalStringWidth = (8 * 2) + 4;
        String stringItem;
        bool firstOrLast = false;
        for (int i = 0; i < textString.size(); i++){
            firstOrLast = i == 0 ||  i == textString.size() - 1 ? true : false;
            stringItem = textString[i];
            stringItem += firstOrLast ? "" : ",";
            // first item uses system font
            stringWidth = textFont.getStringWidth(stringItem);

            messageItemsWithFormat.add(TextStringWithMetrics(stringItem, fontStyle, stringWidth));

            // set up font for next item/s
            fontStyle = FontStyle::Monospace;
            textFont = Font(Fonts::getMonospaceFont());

            // calculate total needed width
            totalStringWidth += stringWidth + 4;
        }
        // only make the size wider, to fit changing values
        if (totalStringWidth > getWidth() || isHoverEntered)
            setSize(totalStringWidth, 36);

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
            if (activeConnection.getComponent()) {
                setCentrePosition(getParentComponent()->getLocalPoint(nullptr, activeConnection->mouseHoverPos).translated(0, -(getHeight() * 0.5)));
                setVisible(activeConnection.getComponent());
            }
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

        int startPostionX = 8 + 4;
        for (auto item : messageItemsWithFormat) {
            Fonts::drawStyledText(g, item.text, startPostionX, 0, item.width, getHeight(), findColour(PlugDataColour::panelTextColourId), item.fontStyle, 14, Justification::centredLeft);
            startPostionX += item.width + 4;
        }
        //previousBounds = getBounds();
    }

    static inline bool isShowing = false;

    struct TextStringWithMetrics {
        TextStringWithMetrics(String text, FontStyle fontStyle, int width) : text(text), fontStyle(fontStyle), width(width){};
        String text;
        FontStyle fontStyle;
        int width;
    };
    Array<TextStringWithMetrics> messageItemsWithFormat;

    Component::SafePointer<Connection> activeConnection;
    int mouseDelay = 500;
    enum TimerID {RepaintTimer, MouseHoverDelay, MouseHoverExitDelay};

    Point<float> circlePosition = { 8 + 4, 36 / 2 };
    float circleRadius = 3.0f;

    Image cachedImage;
    Rectangle<int> previousBounds;
};
