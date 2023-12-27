/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/MidiDeviceManager.h"

class OpenFileObject final : public TextBase {
public:
    OpenFileObject(pd::WeakReference ptr, Object* object)
        : TextBase(ptr, object)
    {
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));

            auto font = editor->getFont();
            auto textWidth = font.getStringWidth(objectText) + 20;
            editor->setBorder(border);
            editor->setBounds(getLocalBounds().withWidth(textWidth));
            object->setSize(textWidth + Object::doubleMargin, getHeight() + Object::doubleMargin);
            setSize(textWidth, getHeight());

            editor->setText(objectText, false);
            editor->addListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onFocusLost = [this]() {
                object->updateBounds();
                hideEditor();
            };

            resized();
            repaint();
        }
    }

    Rectangle<int> getPdBounds() override
    {
        auto tokens = StringArray::fromTokens(objectText, true);
        tokens.removeRange(0, tokens.indexOf("-h") + 2);
        auto linkText = tokens.joinIntoString(" ");

        if (auto obj = ptr.get<void>()) {
            auto* cnvPtr = cnv->patch.getPointer().get();
            if (!cnvPtr)
                return {};

            auto newNumLines = 0;
            auto newBounds = TextObjectHelper::recalculateTextObjectBounds(cnvPtr, obj.cast<t_gobj>(), linkText, newNumLines);

            return newBounds;
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class LinkObjectBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            Object* object;

            explicit LinkObjectBoundsConstrainer(Object* parent)
                : object(parent)
            {
            }

            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool isStretchingTop,
                bool isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                bounds = old;
            }
        };

        return std::make_unique<LinkObjectBoundsConstrainer>(object);
    }

    void paint(Graphics& g) override
    {
        auto backgroundColour = object->findColour(PlugDataColour::textObjectBackgroundColourId);

        auto tokens = StringArray::fromTokens(objectText, true);
        tokens.removeRange(0, tokens.indexOf("-h") + 2);
        auto linkText = tokens.joinIntoString(" ");

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
            bool locked = getValue<bool>(object->locked) || getValue<bool>(object->commandLocked);

            auto colour = object->findColour((locked && isMouseOver()) ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::canvasTextColourId);

            Fonts::drawFittedText(g, linkText, textArea, colour, 1, 1.0f);
        }
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!getValue<bool>(object->locked) && !getValue<bool>(object->commandLocked))
            return;

        if (auto openfile = ptr.get<void>()) {
            pd->sendDirectMessage(openfile.get(), "bang", std::vector<pd::Atom> {});
        }
    }

    void paintOverChildren(Graphics& g) override
    {
    }
};
