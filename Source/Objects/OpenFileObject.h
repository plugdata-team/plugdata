/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include "Utility/MidiDeviceManager.h"

class OpenFileObject final : public TextBase {
public:
    bool mouseWasOver = false;

    TextLayout textLayout;
    hash32 layoutTextHash = 0;
    int lastTextWidth = 0;
    int32 lastColourARGB = 0;

    OpenFileObject(pd::WeakReference ptr, Object* object)
        : TextBase(ptr, object)
    {
    }
    
    bool hideInlet() override
    {
        return true;
    }

    void showEditor() override
    {
        if (editor == nullptr) {
            editor.reset(TextObjectHelper::createTextEditor(object, 15));

            auto const font = editor->getFont();
            auto const textWidth = Fonts::getStringWidth(objectText, font) + 20;
            editor->setLookAndFeel(&object->getLookAndFeel());
            editor->setBorder(border);
            editor->setBounds(getLocalBounds().withWidth(textWidth));
            editor->getProperties().set("NoBackground", true);
            editor->getProperties().set("NoOutline", true);
            object->setSize(textWidth + Object::doubleMargin, getHeight() + Object::doubleMargin);
            setSize(textWidth, getHeight());

            editor->setText(objectText, false);
            editor->addListener(this);
            editor->selectAll();

            addAndMakeVisible(editor.get());
            editor->grabKeyboardFocus();

            editor->onFocusLost = [this] {
                object->updateBounds();
                hideEditor();
            };

            resized();
            repaint();
        }
    }

    int getTextObjectWidth() const
    {
        auto objText = getLinkText();
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        }

        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<t_text>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getRawPointer());
        }

        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int const idealWidth = CachedStringWidth<15>::calculateStringWidth(objText) + 14;

        // We want to adjust the width so ideal text with aligns with fontWidth
        int const offset = idealWidth % fontWidth;

        int textWidth;
        if (objText.isEmpty()) { // If text is empty, set to minimum width
            textWidth = std::max(charWidth, 1) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            textWidth = std::clamp(idealWidth, fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            textWidth = std::max(charWidth, 1) * fontWidth + offset;
        }

        auto const maxIolets = std::max(object->numInputs, object->numOutputs);
        textWidth = std::max(textWidth, maxIolets * 18);

        return textWidth;
    }

    void updateTextLayout() override
    {
        auto const objText = getLinkText();
        auto const mouseIsOver = isMouseOver();
        bool const locked = getValue<bool>(object->locked) || getValue<bool>(object->commandLocked);
        auto colour = PlugDataColours::objectSelectedOutlineColour;
        if(locked && mouseIsOver)
            colour = colour.withRotatedHue(0.5f);
        
        int const textWidth = getTextSize().getWidth() - 11;
        if (cachedTextRender.prepareLayout(objText, Fonts::getCurrentFont().withHeight(15), colour, textWidth, getValue<int>(sizeProperty), PlugDataLook::getUseSyntaxHighlighting() && isValid)) {
            repaint();
        }
    }

    String getLinkText() const
    {
        auto tokens = StringArray::fromTokens(editor ? editor->getText() : objectText, true);
        tokens.removeRange(0, tokens.indexOf("-h") + 1);
        if(tokens.size() > 1)
            tokens.removeRange(0, 1);
        
        return tokens.joinIntoString(" ");
    }
    
    void render(NVGcontext* nvg) override
    {
        updateTextLayout();
        
        auto const b = getLocalBounds();
        
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgColour(PlugDataColours::textObjectBackgroundColour), nvgRGBA(0, 0, 0, 0), Corners::objectCornerRadius);

        if (editor && editor->isVisible()) {
            imageRenderer.renderJUCEComponent(nvg, *editor, getImageScale());
        } else {
            cachedTextRender.renderText(nvg, border.subtractedFrom(b).toFloat(), getImageScale());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        updateTextLayout(); // make sure layout height is updated

        int x = 0, y = 0, w, h;
        if (auto obj = ptr.get<t_gobj>()) {
            auto* cnvPtr = cnv->patch.getRawPointer();
            if (!cnvPtr)
                return { x, y, getTextObjectWidth(), std::max<int>(textLayout.getHeight() + 6, 21) };

            pd::Interface::getObjectBounds(cnvPtr, obj.get(), &x, &y, &w, &h);
        }

        return { x, y, getTextObjectWidth(), std::max<int>(textLayout.getHeight() + 6, 21) };
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

    void mouseEnter(MouseEvent const& e) override
    {
        updateTextLayout();
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        updateTextLayout();
        repaint();
    }

    bool hideInGraph() override
    {
        return false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!getValue<bool>(object->locked) && !getValue<bool>(object->commandLocked))
            return;

        if (auto openfile = ptr.get<void>()) {
            pd->sendDirectMessage(openfile.get(), "bang", SmallArray<pd::Atom> {});
        }
    }
};
