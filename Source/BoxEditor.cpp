/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "BoxEditor.h"
#include "Box.h"
#include "Canvas.h"
#include "Edge.h"
#include <JuceHeader.h>

ClickLabel::ClickLabel(Box* parent, MultiComponentDragger<Box>& multiDragger)
    : box(parent)
    , dragger(multiDragger)
{
    setEditable(false, box->locked);
    setJustificationType(Justification::centred);
    setLookAndFeel(&clook);
};

void ClickLabel::mouseDown(const MouseEvent& e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if (canvas->isGraph || canvas->pd->locked)
        return;

    isDown = true;
    dragger.handleMouseDown(box, e);
}

void ClickLabel::mouseUp(const MouseEvent& e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if (canvas->isGraph || canvas->pd->locked)
        return;

    isDown = false;
    dragger.handleMouseUp(box, e);

    if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600) {
        Edge::connectingEdge = nullptr;
    }
}

void ClickLabel::mouseDrag(const MouseEvent& e)
{
    Canvas* canvas = findParentComponentOfClass<Canvas>();
    if (canvas->isGraph || canvas->pd->locked)
        return;

    dragger.handleMouseDrag(e);
}

TextEditor* ClickLabel::createEditorComponent()
{

    auto* editor = Label::createEditorComponent();

    editor->setAlwaysOnTop(true);
    editor->setMultiLine(false, false);

    editor->setInputFilter(&suggestor, false);
    editor->addKeyListener(&suggestor);

    suggestor.createCalloutBox(box, editor);

    box->cnv->addChildComponent(suggestor);
    auto boundsInParent = getBounds() + box->getPosition();

    suggestor.setBounds(boundsInParent.getX(), boundsInParent.getBottom(), 200, 200);
    suggestor.resized();

    return editor;
}

void ClickLabel::editorAboutToBeHidden(TextEditor* editor)
{
    suggestor.setVisible(false);

    editor->setInputFilter(nullptr, false);

    // Clear overridden lambda
    editor->onTextChange = []() {};

    suggestor.openedEditor = nullptr;
    suggestor.currentBox = nullptr;
}

SuggestionBox::SuggestionBox()
{
    // Set up the button list that contains our suggestions
    buttonholder = std::make_unique<Component>();

    for (int i = 0; i < 20; i++) {
        SuggestionComponent* but = buttons.add(new SuggestionComponent);
        buttonholder->addAndMakeVisible(buttons[i]);

        // Colour pattern
        but->setColour(TextButton::buttonColourId, colours[i % 2]);
        but->onStateChange = [this, i, but]() mutable {
            if (but->getToggleState())
                move(0, i);
        };
    }

    // select the first button
    buttons[0]->setToggleState(true, sendNotification);

    // Set up viewport
    port = std::make_unique<Viewport>();
    port->setScrollBarsShown(true, false);
    port->setViewedComponent(buttonholder.get(), false);
    addAndMakeVisible(port.get());

    setLookAndFeel(&buttonlook);
    setInterceptsMouseClicks(true, true);
    setAlwaysOnTop(true);
    setVisible(true);
}

SuggestionBox::~SuggestionBox()
{
    buttons.clear();
    setLookAndFeel(nullptr);
}

void SuggestionBox::createCalloutBox(Box* box, TextEditor* editor)
{
    currentBox = box;
    openedEditor = editor;

    // Should run after the input filter
    editor->onTextChange = [this, editor, box]() {
        if (isCompleting && !editor->getText().containsChar(' ')) {
            editor->setHighlightedRegion({ highlightStart, highlightEnd });
        }
        auto width = editor->getTextWidth() + 10;
        
        if(width > box->getWidth()) {
            box->setSize(width, box->getHeight());
        }
    };

    for (int i = 0; i < buttons.size(); i++) {
        TextButton* but = buttons[i];
        but->onClick = [this, i]() mutable {
            currentidx = i;
        };
    }

    buttons[0]->setToggleState(true, sendNotification);
    setVisible(true);
    repaint();
}

void SuggestionBox::move(int offset, int setto)
{
    if (!openedEditor)
        return;

    // Calculate new selected index
    if (setto == -1)
        currentidx += offset;
    else
        currentidx = setto;

    if (numOptions == 0)
        return;

    // Limit it to minimum of the number of buttons and the number of suggestions
    int numButtons = std::min(20, numOptions);
    currentidx = (currentidx + numButtons) % numButtons;

    TextButton* but = buttons[currentidx];
    but->setToggleState(true, dontSendNotification);

    if (openedEditor) {
        String newText = buttons[currentidx]->getButtonText();
        openedEditor->setText(newText, dontSendNotification);
        highlightEnd = newText.length();
        openedEditor->setHighlightedRegion({ highlightStart, highlightEnd });
    }

    // Auto-scroll item into viewport bounds
    if (port->getViewPositionY() > but->getY()) {
        port->setViewPosition(0, but->getY());
    } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight()) {
        port->setViewPosition(0, but->getY() - (but->getHeight() * 4));
    }
}

void SuggestionBox::paintOverChildren(Graphics& g)
{
    g.setColour(bordercolor);
    g.drawRect(port->getBounds());
}

void SuggestionBox::resized()
{
    port->setBounds(0, 0, getWidth(), std::min(5, numOptions) * 20);
    buttonholder->setBounds(0, 0, getWidth(), std::min(numOptions, 20) * 20);

    for (int i = 0; i < buttons.size(); i++)
        buttons[i]->setBounds(0, i * 20, getWidth(), 20);
}

bool SuggestionBox::keyPressed(const KeyPress& key, Component* originatingComponent)
{
    if (key == KeyPress::upKey || key == KeyPress::downKey) {
        move(key == KeyPress::downKey  ? 1 : -1);
        return true;
    }
    return false;
}

String SuggestionBox::filterNewText(TextEditor& e, const String& newInput)
{
    String mutableInput = newInput;
    // onChange(mutableInput);

    // Find start of highlighted region
    // This is the start of the last auto-completion suggestion
    // This region will automatically be removed after this function because it's selected
    int start = e.getHighlightedRegion().getLength() > 0 ? e.getHighlightedRegion().getStart() : e.getText().length();

    // Reconstruct users typing
    String typedText = e.getText().substring(0, start) + mutableInput;
    highlightStart = typedText.length();

    if (typedText.length() > 0)
        setVisible(true);
    else
        setVisible(false);

    // Update suggestions
    auto found = currentBox->cnv->main.pd.objectLibrary.autocomplete(typedText.toStdString());

    for (int i = 0; i < std::min<int>(buttons.size(), found.size()); i++)
        buttons[i]->setText(found[i]);

    for (int i = found.size(); i < buttons.size(); i++)
        buttons[i]->setText("     ");

    numOptions = found.size();
    resized();

    // Get length of user-typed text
    int textlen = e.getText().substring(0, start).length();

    // Retrieve best suggestion
    if (currentidx >= found.size() || textlen == 0) {
        highlightEnd = 0;
        return mutableInput;
    }

    String fullName = found[currentidx];

    highlightEnd = fullName.length();

    if (!mutableInput.containsNonWhitespaceChars() || (e.getText() + mutableInput).contains(" ")) {
        isCompleting = false;
        return mutableInput;
    }

    isCompleting = true;
    mutableInput = fullName.substring(textlen);

    return mutableInput;
}
