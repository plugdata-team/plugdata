/*
 // Copyright (c) 2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/MidiDeviceManager.h"

class OpenFileObject final : public TextBase {
public:
    
    bool mouseWasOver = false;
    
    OpenFileObject(pd::WeakReference ptr, Object* object)
        : TextBase(ptr, object)
    {
    }
    
    bool hideInlets() override { return true; }

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
    
    Rectangle<int> getTextSize() override
    {
        auto objText = getLinkText();
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        }
                
        int fontWidth = 7;
        int charWidth = 0;
        if (auto obj = ptr.get<void>()) {
            charWidth = TextObjectHelper::getWidthInChars(obj.get());
            fontWidth = glist_fontwidth(cnv->patch.getPointer().get());
        }
        
        auto textBounds = cachedTextRender.getTextBounds();
        
        // Calculating string width is expensive, so we cache all the strings that we already calculated the width for
        int idealWidth = textBounds.getWidth() + 14;
        
        // We want to adjust the width so ideal text with aligns with fontWidth
        int offset = idealWidth % fontWidth;
        
        int textWidth;
        if (objText.isEmpty()) { // If text is empty, set to minimum width
            textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth;
        } else if (charWidth == 0) { // If width is set to automatic, calculate based on text width
            textWidth = std::clamp(idealWidth, TextObjectHelper::minWidth * fontWidth, fontWidth * 60);
        } else { // If width was set manually, calculate what the width is
            textWidth = std::max(charWidth, TextObjectHelper::minWidth) * fontWidth + offset;
        }
        
        auto maxIolets = std::max(object->numInputs, object->numOutputs);
        textWidth = std::max(textWidth, maxIolets * 18);
        
        return {textWidth, textBounds.getHeight()};
    }
        
    void updateTextLayout() override
    {
        /*
        auto objText = getLinkText();
        if (editor && cnv->suggestor && cnv->suggestor->getText().isNotEmpty()) {
            objText = cnv->suggestor->getText();
        }
        
        auto mouseIsOver = isMouseOver();
        
        int textWidth = getTextSize().getWidth() - 14; // Reserve a bit of extra space for the text margin
        auto currentLayoutHash = hash(objText);
        auto colour = object->findColour(PlugDataColour::canvasTextColourId);
        
        if(layoutTextHash != currentLayoutHash || colour.getARGB() != lastColourARGB || textWidth != lastTextWidth || mouseIsOver != mouseWasOver)
        {
            bool locked = getValue<bool>(object->locked) || getValue<bool>(object->commandLocked);
            auto colour = object->findColour((locked && mouseIsOver) ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::canvasTextColourId);
            
            auto attributedText = AttributedString(objText);
            attributedText.setColour(colour);
            attributedText.setJustification(Justification::centredLeft);
            attributedText.setFont(Font(15));
            attributedText.setColour(colour);
            
            textLayout = TextLayout();
            textLayout.createLayout(attributedText, textWidth);
            layoutTextHash = currentLayoutHash;
            lastColourARGB = colour.getARGB();
            lastTextWidth = textWidth;
        } */
    }
    
    String getLinkText()
    {
        auto tokens = StringArray::fromTokens(editor ? editor->getText() : objectText, true);
        tokens.removeRange(0, tokens.indexOf("-h") + 2);
        return tokens.joinIntoString(" ");
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

    void render(NVGcontext* nvg) override
    {
        // TODO: IMPLEMENT THIS
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
