/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

struct NVGcontext;
class NVGGraphicsContext;
class DraggableNumber : public Component
    , public TextEditor::Listener {
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
    int maxPrecision = 6;
    Colour outlineColour, textColour;
    Font font = Font(FontOptions());

    std::unique_ptr<TextEditor> editor;

public:
    std::function<void()> onTextChange = [] { };
    std::function<void()> onEditorShow = [] { };
    std::function<void()> onEditorHide = [] { };

    std::function<void(double)> onValueChange = [](double) { };
    std::function<void(double)> onReturnKey = [](double) { };
    std::function<void()> dragStart = [] { };
    std::function<void()> dragEnd = [] { };

    std::function<void(bool)> onInteraction = [](bool) { };

    explicit DraggableNumber(bool integerDrag);

    ~DraggableNumber() override;

    void colourChanged() override;

    void lookAndFeelChanged() override;

    void editorShown(TextEditor& editor);

    void focusGained(FocusChangeType cause) override;

    void focusLost(FocusChangeType cause) override;

    void setMinimumHorizontalScale(float newScale);

    void setText(String const& newText, NotificationType notification);

    TextEditor* getCurrentTextEditor();

    bool isBeingEdited() const;

    void setBorderSize(BorderSize<int> newBorder);

    String getText();

    void setFont(Font newFont);

    Font getFont();

    void setEditableOnClick(bool editableOnClick, bool editableOnDoubleClick = false, bool handleFocusLossManually = false);

    void setMaximum(double maximum);

    void setMinimum(double minimum);

    void setLogarithmicHeight(double logHeight);

    void setPrecision(int precision);

    // Toggle between showing ellipses or ">" if number is too large to fit
    void setShowEllipsesIfTooLong(bool shouldShowEllipses);

    void showEditor();

    void resized() override;

    bool updateFromTextEditorContents(TextEditor const& ed);

    void hideEditor(bool discardCurrentEditorContents);

    void inputAttemptWhenModal() override;

    bool keyPressed(KeyPress const& key) override;

    void setValue(double newValue, NotificationType notification = sendNotification, bool clip = true);

    double getValue() const;

    void setResetEnabled(bool enableReset);

    void setResetValue(double resetValue);

    // Make sure mouse cursor gets reset, sometimes this doesn't happen automatically
    void mouseEnter(MouseEvent const& e) override;
    void mouseExit(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void setDragMode(DragMode newDragMode);

    Rectangle<float> getDraggedNumberBounds(int dragPosition) const;

    int getDecimalAtPosition(int x, Rectangle<float>* position = nullptr) const;

    virtual void render(NVGcontext* nvg, NVGGraphicsContext* llgc);

    void paint(Graphics& g) override;

    double limitValue(double valueToLimit) const;

    String formatNumber(double value, int precision = -1) const;

    static double parseExpression(String const& expression);

    void textEditorFocusLost(TextEditor& editor) override;

    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void textEditorReturnKeyPressed(TextEditor& editor) override;
};

class DraggableListNumber final : public DraggableNumber {
    int numberStartIdx = 0;
    int numberEndIdx = 0;
    bool targetFound = false;
    
public:
    explicit DraggableListNumber();

    void mouseDown(MouseEvent const& e) override;
    void mouseMove(MouseEvent const& e) override;
    void mouseDrag(MouseEvent const& e) override;
    void mouseUp(MouseEvent const& e) override;

    void paint(Graphics& g) override;

    void render(NVGcontext* nvg, NVGGraphicsContext* llgc) override;

    void textEditorReturnKeyPressed(TextEditor& editor) override;

    std::tuple<int, int, double> getListItemAtPosition(int x, Rectangle<float>* position = nullptr) const;
};
