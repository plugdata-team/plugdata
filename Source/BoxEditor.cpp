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
    //setEditable(false, box->locked);
    //setJustificationType(Justification::centred);
    //setLookAndFeel(&clook);
};

//==============================================================================
void ClickLabel::setText (const String& newText, NotificationType notification)
{
    hideEditor (true);

    if (lastTextValue != newText)
    {
        lastTextValue = newText;
        textValue = newText;
        repaint();

        if (notification != dontSendNotification && onTextChange != nullptr)
            onTextChange();
            
    }
}

String ClickLabel::getText (bool returnActiveEditorContents) const
{
    return (returnActiveEditorContents && isBeingEdited())
                ? editor->getText()
                : textValue.toString();
}


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

void ClickLabel::inputAttemptWhenModal()
{
    if (editor)
    {
        textEditorReturnKeyPressed (*getCurrentTextEditor());
    }
}

static void copyColourIfSpecified (ClickLabel& l, TextEditor& ed, int colourID, int targetColourID)
{
    if (l.isColourSpecified (colourID) || l.getLookAndFeel().isColourSpecified (colourID))
        ed.setColour (targetColourID, l.findColour (colourID));
}

TextEditor* ClickLabel::createEditorComponent()
{
    
    auto* editor = new TextEditor (getName());
    editor->applyFontToAllText(font);
    copyAllExplicitColoursTo (*editor);

    copyColourIfSpecified (*this, *editor, Label::textWhenEditingColourId, TextEditor::textColourId);
    copyColourIfSpecified (*this, *editor, Label::backgroundWhenEditingColourId, TextEditor::backgroundColourId);
    copyColourIfSpecified (*this, *editor, Label::outlineWhenEditingColourId, TextEditor::focusedOutlineColourId);

    editor->setAlwaysOnTop(true);
    
    bool multiLine = box->pdObject && box->pdObject->getType() == pd::Type::Comment;
    
    auto& suggestor = box->cnv->suggestor;
    // Allow multiline for comment objects
    editor->setMultiLine(multiLine, false);
    editor->setReturnKeyStartsNewLine(multiLine);
    
    editor->setInputFilter(&suggestor, false);
    editor->addKeyListener(&suggestor);
    
    editor->onFocusLost = [this](){
        if(!box->cnv->suggestor.hasKeyboardFocus(true)) {
            hideEditor(false);
        }
        
    };

    suggestor.createCalloutBox(box, editor);
    
    auto boundsInParent = getBounds() + box->getPosition();

    suggestor.setBounds(boundsInParent.getX(), boundsInParent.getBottom(), 200, 200);
    suggestor.resized();

    return editor;
}


void ClickLabel::editorShown (TextEditor* textEditor)
{

    if (onEditorShow != nullptr)
        onEditorShow();
}

void ClickLabel::editorAboutToBeHidden (TextEditor* textEditor)
{
    if (auto* peer = getPeer())
        peer->dismissPendingTextInput();

    if (onEditorHide != nullptr)
        onEditorHide();
    
    box->cnv->suggestor.setVisible(false);

    textEditor->setInputFilter(nullptr, false);

    // Clear overridden lambda
    textEditor->onTextChange = []() {};
    
    if(box->graphics && !box->graphics->fakeGUI()) {
        setVisible(false);
        box->resized();
    }
    
    box->cnv->suggestor.openedEditor = nullptr;
    box->cnv->suggestor.currentBox = nullptr;
}

void ClickLabel::showEditor()
{
    if (editor == nullptr)
    {
        editor.reset (createEditorComponent());
        editor->setSize (10, 10);
        addAndMakeVisible (editor.get());
        editor->setText (getText(), false);
        editor->setKeyboardType (keyboardType);
        editor->addListener (this);
        editor->grabKeyboardFocus();

        if (editor == nullptr) // may be deleted by a callback
            return;

        editor->setHighlightedRegion (Range<int> (0, textValue.toString().length()));

        resized();
        repaint();

        editorShown (editor.get());

        //enterModalState (false);
        
        editor->grabKeyboardFocus();
    }
}

bool ClickLabel::updateFromTextEditorContents (TextEditor& ed)
{
    auto newText = ed.getText();

    if (textValue.toString() != newText)
    {
        lastTextValue = newText;
        textValue = newText;
        repaint();

        return true;
    }

    return false;
}

void ClickLabel::hideEditor (bool discardCurrentEditorContents)
{
    if (editor != nullptr)
    {
        WeakReference<Component> deletionChecker (this);
        std::unique_ptr<TextEditor> outgoingEditor;
        std::swap (outgoingEditor, editor);

        editorAboutToBeHidden (outgoingEditor.get());

        const bool changed = updateFromTextEditorContents (*outgoingEditor);
        
        outgoingEditor.reset();

        repaint();

        if (onTextChange != nullptr)
            onTextChange();
    }
}

void ClickLabel::textEditorReturnKeyPressed (TextEditor& ed)
{
    if (editor != nullptr)
    {
        editor->giveAwayKeyboardFocus();
    }
}

bool ClickLabel::isBeingEdited() const noexcept
{
    return editor != nullptr;
}

static void copyColourIfSpecified (Label& l, TextEditor& ed, int colourID, int targetColourID)
{
    if (l.isColourSpecified (colourID) || l.getLookAndFeel().isColourSpecified (colourID))
        ed.setColour (targetColourID, l.findColour (colourID));
}

TextEditor* ClickLabel::getCurrentTextEditor() const noexcept
{
    return editor.get();
}

//==============================================================================
void ClickLabel::paint (Graphics& g)
{
    g.fillAll (findColour (Label::backgroundColourId));

    if (!isBeingEdited())
    {
        auto alpha = isEnabled() ? 1.0f : 0.5f;

        g.setColour (findColour (Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);
        
        auto textArea = border.subtractedFrom(getLocalBounds());

        g.drawFittedText (getText(), textArea, justification,
                          jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                          minimumHorizontalScale);

        g.setColour (findColour (Label::outlineColourId).withMultipliedAlpha (alpha));
    }
    else if (isEnabled())
    {
        g.setColour (findColour (Label::outlineColourId));
    }

    g.drawRect (getLocalBounds());
}

void ClickLabel::mouseDoubleClick (const MouseEvent& e)
{
    if (editDoubleClick
         && isEnabled()
         && ! e.mods.isPopupMenu())
    {
        showEditor();
    }
}

void ClickLabel::resized()
{
    if (editor != nullptr)
        editor->setBounds (getLocalBounds());
}


void ClickLabel::setEditable(bool editable)
{
    editDoubleClick = editable;

    setWantsKeyboardFocus (editDoubleClick);
    setFocusContainerType (editDoubleClick ? FocusContainerType::keyboardFocusContainer
                                              : FocusContainerType::none);
    invalidateAccessibilityHandler();
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
    }

    // select the first button
    buttons[0]->setToggleState(true, sendNotification);

    // Set up viewport
    port = std::make_unique<Viewport>();
    port->setScrollBarsShown(true, false);
    port->setViewedComponent(buttonholder.get(), false);
    port->setInterceptsMouseClicks(true, true);
    addAndMakeVisible(port.get());

    //setLookAndFeel(&buttonlook);
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
        auto* but = buttons[i];
        but->setAlwaysOnTop(true);
        but->setWantsKeyboardFocus(false);
        but->addMouseListener(this, false);
        but->onClick = [this, i, box, editor]() mutable {
            move(0, i);
            editor->grabKeyboardFocus();
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
    g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), 3.0f, 2.5f);
}

void SuggestionBox::resized()
{
    port->setBounds(0, 0, getWidth(), std::min(5, numOptions) * 20);
    buttonholder->setBounds(0, 0, getWidth(), std::min(numOptions, 20) * 20);

    for (int i = 0; i < buttons.size(); i++)
        buttons[i]->setBounds(2, (i * 20) + 2, getWidth() - 2, 23);
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
