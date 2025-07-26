/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class NVGcontext;
class NVGGraphicsContext;
class DraggableNumber : public Component, public TextEditor::Listener
{
public:
    enum DragMode {
        Regular,
        Integer,
        Logarithmic
    };

protected:
    int16 decimalDrag = 0;
    int16 hoveredDecimal = -1;
    double dragValue = 0.0;
    Rectangle<float> hoveredDecimalPosition;

    String currentValue;
    double lastValue = 0.0;
    double logarithmicHeight = 256.0;
    int16 lastLogarithmicDragPosition = 0;
    double min = 0.0, max = 0.0;
    bool editableOnSingleClick = false, editableOnDoubleClick = false;
    bool handleFocusLossManually = false;
    float minimumHorizontalScale = 1.0f;

    BorderSize<int> border { 1, 5, 1, 5 };

    DragMode dragMode : 2 = Regular;
    bool isMinLimited : 1 = false;
    bool isMaxLimited : 1 = false;
    bool resetOnCommandClick : 1 = false;
    bool wasReset : 1 = false;
    bool showEllipses : 1 = true;
    double valueToResetTo = 0.0;
    double valueToRevertTo = 0.0;
    Colour outlineColour, textColour;
    Font font;

    std::unique_ptr<TextEditor> editor;

    std::unique_ptr<NVGGraphicsContext> nvgCtx;

public:
    std::function<void()> onTextChange = [](){};
    std::function<void()> onEditorShow = [](){};
    std::function<void()> onEditorHide = [](){};

    std::function<void(double)> onValueChange = [](double) { };
    std::function<void(double)> onReturnKey = [](double) { };
    std::function<void()> dragStart = [] { };
    std::function<void()> dragEnd = [] { };

    std::function<void(bool)> onInteraction = [](bool) { };

    explicit DraggableNumber(bool const integerDrag);
    
    ~DraggableNumber();

    void colourChanged() override;

    void lookAndFeelChanged() override;

    void editorShown(TextEditor& editor);

    void focusGained(FocusChangeType const cause) override;

    void focusLost(FocusChangeType const cause) override;

    void setMinimumHorizontalScale(float newScale);

    void setText(String const& newText, NotificationType notification);

    TextEditor* getCurrentTextEditor();

    bool isBeingEdited();

    void setBorderSize(BorderSize<int> newBorder);

    String getText();

    void setFont(Font newFont);

    Font getFont();

    void setEditableOnClick(bool const editableOnClick, bool const editableOnDoubleClick = false, bool const handleFocusLossManually = false);

    void setMaximum(double const maximum);

    void setMinimum(double const minimum);

    void setLogarithmicHeight(double const logHeight);

    // Toggle between showing ellipses or ">" if number is too large to fit
    void setShowEllipsesIfTooLong(bool const shouldShowEllipses);

    void showEditor();

    void resized() override;

    bool updateFromTextEditorContents (TextEditor& ed);

    void hideEditor (bool discardCurrentEditorContents);

    void inputAttemptWhenModal() override;

    bool keyPressed(KeyPress const& key) override;

    void setValue(double newValue, NotificationType const notification = sendNotification, bool clip = true);

    double getValue() const;

    void setResetEnabled(bool const enableReset);

    void setResetValue(double const resetValue);

    // Make sure mouse cursor gets reset, sometimes this doesn't happen automatically
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void setDragMode(DragMode const newDragMode);

    Rectangle<float> getDraggedNumberBounds(int dragPosition);

    int getDecimalAtPosition(int const x, Rectangle<float>* position = nullptr) const;

    virtual void render(NVGcontext* nvg);

    void paint(Graphics& g) override;

    double limitValue(double valueToLimit) const;

    String formatNumber(double const value, int const precision = -1) const;

    static double parseExpression(String const& expression);

    void textEditorFocusLost(TextEditor& editor) override;

    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void textEditorReturnKeyPressed(TextEditor& editor) override;
};

struct DraggableListNumber final : public DraggableNumber {
    int numberStartIdx = 0;
    int numberEndIdx = 0;
    bool targetFound = false;

    explicit DraggableListNumber();

    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void paint(Graphics& g) override;

    void render(NVGcontext* nvg) override;

    void textEditorReturnKeyPressed(TextEditor& editor) override;

    std::tuple<int, int, double> getListItemAtPosition(int const x, Rectangle<float>* position = nullptr) const;
};
