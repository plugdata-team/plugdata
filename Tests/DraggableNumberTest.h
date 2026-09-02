#include "Components/DraggableNumber.h"

// Exercises DraggableNumber (Components/DraggableNumber.cpp) - the editable
// number control behind number boxes, the inspector's number fields, atoms and
// many sliders. The other tests create objects that contain it but never drag
// its value or type into it, leaving the drag/edit/clamp logic uncovered.
//
// DraggableNumber is a self-contained Component, so this drives it directly and
// synchronously (no canvas/selection timing involved):
//   - setValue/getValue round-trip and min/max clamping
//   - value-changing drags in each drag mode (regular, integer, logarithmic)
//   - the text editor: show, type a new value, commit on return, and the
//     revert-on-discard path
//   - reset-on-command-click and setText/ellipsis rendering

class DraggableNumberTest : public PlugDataUnitTest
{
public:
    DraggableNumberTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Draggable Number Test")
    {
    }

private:
    MouseEvent makeEvent(Component* c, Point<float> pos, Point<float> down, bool dragged, ModifierKeys mods = ModifierKeys::leftButtonModifier)
    {
        auto const now = Time::getCurrentTime();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), pos, mods,
                          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, c,
                          now, down, now, 1, dragged);
    }

    // Drags the control vertically and returns whether the value changed.
    bool dragValue(DraggableNumber& dn, float dy)
    {
        auto const centre = dn.getLocalBounds().getCentre().toFloat();
        auto const before = dn.getValue();
        dn.mouseDown(makeEvent(&dn, centre, centre, false));
        for (int i = 1; i <= 4; i++)
            dn.mouseDrag(makeEvent(&dn, centre + Point<float>(0, dy * i / 4.0f), centre, true));
        dn.mouseUp(makeEvent(&dn, centre + Point<float>(0, dy), centre, true));
        return !approximatelyEqual(dn.getValue(), before);
    }

    void perform() override
    {
        beginTest("Value set/get and clamping");

        DraggableNumber dn(false); // float drag
        editor->addAndMakeVisible(dn);
        dn.setBounds(0, 0, 120, 24);
        dn.setFont(Font(FontOptions(15)));

        int valueChangeCount = 0;
        dn.onValueChange = [&valueChangeCount](double) { valueChangeCount++; };

        dn.setMinimum(-10.0);
        dn.setMaximum(10.0);
        dn.setValue(3.5);
        expect(approximatelyEqual(dn.getValue(), 3.5), "value must round-trip");
        dn.setValue(999.0); // above maximum -> clamped
        expect(dn.getValue() <= 10.0, "value above maximum must be clamped");
        dn.setValue(-999.0); // below minimum -> clamped
        expect(dn.getValue() >= -10.0, "value below minimum must be clamped");

        beginTest("Value-changing drags in each drag mode");

        dn.setMinimum(-1000.0);
        dn.setMaximum(1000.0);
        dn.setValue(0.0);

        dn.setDragMode(DraggableNumber::Regular);
        expect(dragValue(dn, -60.0f), "a regular vertical drag must change the value");

        dn.setValue(0.0);
        dn.setDragMode(DraggableNumber::Integer);
        expect(dragValue(dn, -60.0f), "an integer-mode drag must change the value");

        dn.setValue(1.0);
        dn.setDragMode(DraggableNumber::Logarithmic);
        dn.setLogarithmicHeight(256.0);
        dragValue(dn, -40.0f); // logarithmic; just exercise the path
        dn.setDragMode(DraggableNumber::Regular);

        expect(valueChangeCount > 0, "drags must fire onValueChange");

        beginTest("Text editor: type and commit, type and discard");

        // Commit a typed value via the return key. The committed value is
        // reported through onReturnKey (the owner then updates its model);
        // getValue() reflects only notifying setValue() calls, by design.
        double returnedValue = 0.0;
        dn.onReturnKey = [&returnedValue](double v) { returnedValue = v; };
        dn.setValue(0.0);
        dn.showEditor();
        if (auto* textEditor = dn.getCurrentTextEditor()) {
            textEditor->setText("42.5", false);
            dn.textEditorReturnKeyPressed(*textEditor); // commit, like pressing return
            expect(approximatelyEqual(returnedValue, 42.5), "committing must report the typed value via onReturnKey");
        } else {
            expect(false, "showEditor must create a text editor");
        }

        // It also evaluates simple arithmetic expressions
        dn.showEditor();
        if (auto* textEditor = dn.getCurrentTextEditor()) {
            textEditor->setText("2 + 3", false);
            dn.textEditorReturnKeyPressed(*textEditor);
            expect(approximatelyEqual(returnedValue, 5.0), "the editor must evaluate arithmetic expressions");
        }

        // Discard a typed value: hideEditor(true) must not change the value
        dn.setValue(7.0);
        dn.showEditor();
        if (auto* textEditor = dn.getCurrentTextEditor()) {
            textEditor->setText("123", false);
            dn.hideEditor(true); // discard
            expect(approximatelyEqual(dn.getValue(), 7.0), "discarding the editor must keep the previous value");
        }

        beginTest("Reset-on-command-click, setText and rendering");

        dn.setResetValue(5.0);
        dn.setValue(2.0);
        // Command-click resets to the reset value (down + up without a drag)
        auto const centre = dn.getLocalBounds().getCentre().toFloat();
        dn.mouseDown(makeEvent(&dn, centre, centre, false, ModifierKeys::leftButtonModifier | ModifierKeys::commandModifier));
        dn.mouseUp(makeEvent(&dn, centre, centre, false, ModifierKeys::leftButtonModifier | ModifierKeys::commandModifier));

        dn.setText("custom text", dontSendNotification);
        dn.setShowEllipsesIfTooLong(true);
        dn.setText("a very long string that will not fit and must be truncated with ellipses", dontSendNotification);
        dn.createComponentSnapshot(dn.getLocalBounds());

        // Hover paints the up/down arrows / decimal highlight
        dn.mouseEnter(makeEvent(&dn, centre, centre, false, ModifierKeys()));
        dn.mouseMove(makeEvent(&dn, centre, centre, false, ModifierKeys()));
        dn.createComponentSnapshot(dn.getLocalBounds());
        dn.mouseExit(makeEvent(&dn, centre, centre, false, ModifierKeys()));

        editor->removeChildComponent(&dn);

        signalDone(true);
    }
};
