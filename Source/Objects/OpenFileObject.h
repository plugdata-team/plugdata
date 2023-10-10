/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/MidiDeviceManager.h"

class OpenFileObject final : public TextBase {
public:

    OpenFileObject(t_gobj* ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void paint(Graphics& g) override
    {
        auto backgroundColour = object->findColour(PlugDataColour::textObjectBackgroundColourId);
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        auto ioletAreaColour = object->findColour(PlugDataColour::ioletAreaColourId);

        if (ioletAreaColour != backgroundColour) {
            g.setColour(ioletAreaColour);
            g.fillRect(getLocalBounds().removeFromTop(3));
            g.fillRect(getLocalBounds().removeFromBottom(3));
        }

        if (!editor) {
            auto textArea = border.subtractedFrom(getLocalBounds());

            auto scale = getWidth() < 40 ? 0.9f : 1.0f;
            
            bool locked = getValue<bool>(object->locked) || getValue<bool>(object->commandLocked);
            
            auto colour = object->findColour((locked && isMouseOver()) ? PlugDataColour::objectSelectedOutlineColourId :  PlugDataColour::canvasTextColourId);
            
            Fonts::drawFittedText(g, objectText.fromFirstOccurrenceOf("-h", false, false), textArea, colour, numLines, scale);
        }
    }
    
    void mouseEnter(const MouseEvent& e) override
    {
        repaint();
    }
    
    void mouseExit(const MouseEvent& e) override
    {
        repaint();
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        if(!getValue<bool>(object->locked) && !getValue<bool>(object->commandLocked)) return;
        
        if(auto openfile = ptr.get<void>())
        {
            pd->sendDirectMessage(openfile.get(), "bang", std::vector<pd::Atom>{});
        }
    }

    void paintOverChildren(Graphics& g) override
    {
    }
};
