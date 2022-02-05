/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "BoxEditor.h"

#include <JuceHeader.h>

#include "Box.h"
#include "Canvas.h"

ClickLabel::ClickLabel(Box* parent, MultiComponentDragger<Box>& multiDragger) : box(parent), dragger(multiDragger) {}

//==============================================================================
void ClickLabel::setText(const String& newText, NotificationType notification) {
  hideEditor();

  if (lastTextValue != newText) {
    lastTextValue = newText;
    textValue = newText;
    repaint();

    if (notification != dontSendNotification && onTextChange != nullptr) onTextChange();
  }
}

String ClickLabel::getText(bool returnActiveEditorContents) const { return (returnActiveEditorContents && isBeingEdited()) ? editor->getText() : textValue.toString(); }

void ClickLabel::mouseDown(const MouseEvent& e) {
  auto* canvas = findParentComponentOfClass<Canvas>();
  if (canvas->isGraph || canvas->pd->locked) return;

  isDown = true;
  canvas->handleMouseDown(box, e);
}

void ClickLabel::mouseUp(const MouseEvent& e) {
  auto* canvas = findParentComponentOfClass<Canvas>();
  if (canvas->isGraph || canvas->pd->locked) return;

  isDown = false;
  dragger.handleMouseUp(box, e);

  if (e.getDistanceFromDragStart() > 10 || e.getLengthOfMousePress() > 600) {
    canvas->connectingEdge = nullptr;
  }
}

void ClickLabel::mouseDrag(const MouseEvent& e) {
  auto* canvas = findParentComponentOfClass<Canvas>();
  if (canvas->isGraph || canvas->pd->locked) return;

  canvas->handleMouseDrag(e);
}

void ClickLabel::inputAttemptWhenModal() {
  if (editor) {
    textEditorReturnKeyPressed(*getCurrentTextEditor());
  }
}

static void copyColourIfSpecified(ClickLabel& l, TextEditor& ed, int colourId, int targetColourId) {
  if (l.isColourSpecified(colourId) || l.getLookAndFeel().isColourSpecified(colourId)) ed.setColour(targetColourId, l.findColour(colourId));
}

TextEditor* ClickLabel::createEditorComponent() {
  auto* newEditor = new TextEditor(getName());
  newEditor->applyFontToAllText(font);
  copyAllExplicitColoursTo(*newEditor);

  copyColourIfSpecified(*this, *newEditor, Label::textWhenEditingColourId, TextEditor::textColourId);
  copyColourIfSpecified(*this, *newEditor, Label::backgroundWhenEditingColourId, TextEditor::backgroundColourId);
  copyColourIfSpecified(*this, *newEditor, Label::outlineWhenEditingColourId, TextEditor::focusedOutlineColourId);

  newEditor->setAlwaysOnTop(true);

  bool multiLine = box->pdObject && box->pdObject->getType() == pd::Type::Comment;

  auto& suggestor = box->cnv->suggestor;
  // Allow multiline for comment objects
  newEditor->setMultiLine(multiLine, false);
  newEditor->setReturnKeyStartsNewLine(multiLine);

  newEditor->setInputFilter(&suggestor, false);
  newEditor->addKeyListener(&suggestor);

  newEditor->onFocusLost = [this]() {
    if (!box->cnv->suggestor.hasKeyboardFocus(true)) {
      hideEditor();
    }
  };

  suggestor.createCalloutBox(box, newEditor);

  auto boundsInParent = getBounds() + box->getPosition();

  suggestor.setBounds(boundsInParent.getX(), boundsInParent.getBottom(), 200, 115);
  suggestor.resized();

  return newEditor;
}

void ClickLabel::editorShown(TextEditor* textEditor) {
  if (onEditorShow != nullptr) onEditorShow();
}

void ClickLabel::editorAboutToBeHidden(TextEditor* textEditor) {
  if (auto* peer = getPeer()) peer->dismissPendingTextInput();

  if (onEditorHide != nullptr) onEditorHide();

  box->cnv->suggestor.setVisible(false);

  textEditor->setInputFilter(nullptr, false);

  // Clear overridden lambda
  textEditor->onTextChange = []() {};

  if (box->graphics && !box->graphics->fakeGui()) {
    setVisible(false);
    box->resized();
  }

  box->cnv->suggestor.openedEditor = nullptr;
  box->cnv->suggestor.currentBox = nullptr;
}

void ClickLabel::showEditor() {
  if (editor == nullptr) {
    editor.reset(createEditorComponent());
    editor->setSize(10, 10);
    addAndMakeVisible(editor.get());
    editor->setText(getText(), false);
    editor->setKeyboardType(keyboardType);
    editor->addListener(this);

    if (editor == nullptr)  // may be deleted by a callback
      return;

    editor->setHighlightedRegion(Range<int>(0, textValue.toString().length()));

    resized();
    repaint();

    editorShown(editor.get());

    // enterModalState (false);

    editor->grabKeyboardFocus();
  }
}

bool ClickLabel::updateFromTextEditorContents(TextEditor& ed) {
  auto newText = ed.getText();

  if (textValue.toString() != newText) {
    lastTextValue = newText;
    textValue = newText;
    repaint();

    return true;
  }

  return false;
}

void ClickLabel::hideEditor() {
  if (editor != nullptr) {
    WeakReference<Component> deletionChecker(this);
    std::unique_ptr<TextEditor> outgoingEditor;
    std::swap(outgoingEditor, editor);

    editorAboutToBeHidden(outgoingEditor.get());

    updateFromTextEditorContents(*outgoingEditor);

    outgoingEditor.reset();

    repaint();

    if (onTextChange != nullptr) onTextChange();
  }
}

void ClickLabel::textEditorReturnKeyPressed(TextEditor& ed) {
  if (editor != nullptr) {
    editor->giveAwayKeyboardFocus();
  }
}

bool ClickLabel::isBeingEdited() const noexcept { return editor != nullptr; }

TextEditor* ClickLabel::getCurrentTextEditor() const noexcept { return editor.get(); }

//==============================================================================
void ClickLabel::paint(Graphics& g) {
  g.fillAll(findColour(Label::backgroundColourId));

  if (!isBeingEdited()) {
    auto alpha = isEnabled() ? 1.0f : 0.5f;

    g.setColour(findColour(Label::textColourId).withMultipliedAlpha(alpha));
    g.setFont(font);

    auto textArea = border.subtractedFrom(getLocalBounds());

    g.drawFittedText(getText(), textArea, justification, jmax(1, static_cast<int>((static_cast<float>(textArea.getHeight()) / font.getHeight()))), minimumHorizontalScale);

    g.setColour(findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
  } else if (isEnabled()) {
    g.setColour(findColour(Label::outlineColourId));
  }

  g.drawRect(getLocalBounds());
}

void ClickLabel::mouseDoubleClick(const MouseEvent& e) {
  if (editDoubleClick && isEnabled() && !e.mods.isPopupMenu()) {
    showEditor();
  }
}

void ClickLabel::resized() {
  if (editor != nullptr) editor->setBounds(getLocalBounds());
}

void ClickLabel::setEditable(bool editable) {
  editDoubleClick = editable;

  setWantsKeyboardFocus(editDoubleClick);
  setFocusContainerType(editDoubleClick ? FocusContainerType::keyboardFocusContainer : FocusContainerType::none);
  invalidateAccessibilityHandler();
}

SuggestionBox::SuggestionBox(Resources& r) : editorLook(r) {
  // Set up the button list that contains our suggestions
  buttonholder = std::make_unique<Component>();

  for (int i = 0; i < 20; i++) {
    SuggestionComponent* but = buttons.add(new SuggestionComponent);
    buttonholder->addAndMakeVisible(buttons[i]);

    but->setClickingTogglesState(true);
    but->setRadioGroupId(110);

    // Colour pattern
    but->setColour(TextButton::buttonColourId, colours[i % 2]);
  }

  // select the first button
  // buttons[0]->setToggleState(true, sendNotification);

  // Set up viewport
  port = std::make_unique<Viewport>();
  port->setScrollBarsShown(true, false);
  port->setViewedComponent(buttonholder.get(), false);
  port->setInterceptsMouseClicks(true, true);
  port->setViewportIgnoreDragFlag(true);
  addAndMakeVisible(port.get());

  setLookAndFeel(&editorLook);
  setInterceptsMouseClicks(true, true);
  setAlwaysOnTop(true);
  setVisible(true);
}

SuggestionBox::~SuggestionBox() {
  buttons.clear();
  setLookAndFeel(nullptr);
}

void SuggestionBox::createCalloutBox(Box* box, TextEditor* editor) {
  currentBox = box;
  openedEditor = editor;

  // Should run after the input filter
  editor->onTextChange = [this, editor, box]() {
    if (isCompleting && !editor->getText().containsChar(' ')) {
      editor->setHighlightedRegion({highlightStart, highlightEnd});
    }
    auto width = editor->getTextWidth() + 10;

    if (width > box->getWidth()) {
      box->setSize(width, box->getHeight());
    }
  };

  for (int i = 0; i < buttons.size(); i++) {
    auto* but = buttons[i];
    but->setAlwaysOnTop(true);

    but->onClick = [this, i, editor]() mutable {
      move(0, i);
      if (!editor->isVisible()) editor->setVisible(true);
      editor->grabKeyboardFocus();
    };
  }

  // buttons[0]->setToggleState(true, sendNotification);
  setVisible(editor->getText().isNotEmpty());
  repaint();
}

void SuggestionBox::move(int offset, int setto) {
  if (!openedEditor) return;

  // Calculate new selected index
  if (setto == -1)
    currentidx += offset;
  else
    currentidx = setto;

  if (numOptions == 0) return;

  // Limit it to minimum of the number of buttons and the number of suggestions
  int numButtons = std::min(20, numOptions);
  currentidx = (currentidx + numButtons) % numButtons;

  auto* but = buttons[currentidx];

  // If we use setto, the toggle state should already be set
  if (setto == -1) but->setToggleState(true, dontSendNotification);

  if (openedEditor) {
    String newText = buttons[currentidx]->getButtonText();
    openedEditor->setText(newText, dontSendNotification);
    highlightEnd = newText.length();
    openedEditor->setHighlightedRegion({highlightStart, highlightEnd});
  }

  // Auto-scroll item into viewport bounds
  if (port->getViewPositionY() > but->getY()) {
    port->setViewPosition(0, but->getY());
  } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight()) {
    port->setViewPosition(0, but->getY() - (but->getHeight() * 4));
  }
}

void SuggestionBox::paint(Graphics& g) {
  g.setColour(MainLook::firstBackground);
  g.fillRect(port->getBounds());
}

void SuggestionBox::paintOverChildren(Graphics& g) {
  g.setColour(bordercolor);
  g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), 3.0f, 2.5f);
}

void SuggestionBox::resized() {
  port->setBounds(0, 0, getWidth(), std::min(std::min(5, numOptions) * 23, getHeight()));
  buttonholder->setBounds(0, 0, getWidth(), std::min((numOptions + 1), 20) * 22 + 2);

  for (int i = 0; i < buttons.size(); i++) buttons[i]->setBounds(2, (i * 22) + 2, getWidth() - 2, 23);

  repaint();
}

bool SuggestionBox::keyPressed(const KeyPress& key, Component* originatingComponent) {
  if (key == KeyPress::upKey || key == KeyPress::downKey) {
    move(key == KeyPress::downKey ? 1 : -1);
    return true;
  }
  return false;
}

String SuggestionBox::filterNewText(TextEditor& e, const String& newInput) {
  String mutableInput = newInput;
  // onChange(mutableInput);

  // Find start of highlighted region
  // This is the start of the last auto-completion suggestion
  // This region will automatically be removed after this function because it's selected
  int start = e.getHighlightedRegion().getLength() > 0 ? e.getHighlightedRegion().getStart() : e.getText().length();

  // Reconstruct users typing
  String typedText = e.getText().substring(0, start) + mutableInput;
  highlightStart = typedText.length();

  // Update suggestions
  auto found = currentBox->cnv->pd->objectLibrary.autocomplete(typedText.toStdString());

  for (int i = 0; i < std::min<int>(buttons.size(), found.size()); i++) buttons[i]->setText(found[i]);

  for (int i = found.size(); i < buttons.size(); i++) buttons[i]->setText("     ");

  numOptions = found.size();

  setVisible(typedText.isNotEmpty() && numOptions);

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
