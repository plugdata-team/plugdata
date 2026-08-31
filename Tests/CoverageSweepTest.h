#include "Components/ConnectionMessageDisplay.h"
#include "Components/BouncingViewport.h"
#include "Components/ObjectDragAndDrop.h"
#include "Components/SuggestionComponent.h"
#include "Components/TouchPopupMenu.h"
#include "Components/TouchSelectionHelper.h"
#include "Components/WelcomePanel.h"
#include "Dialogs/Dialogs.h"
#include "Dialogs/AboutPanel.h"
#include "Dialogs/SettingsDialog.h"
#include "Dialogs/AudioSettingsPanel.h"
#include "Dialogs/KeyMappingSettingsPanel.h"
#include "Dialogs/ObjectBrowserDialog.h"
#include "Heavy/ExportingProgressView.h"
#include "Heavy/ExporterBase.h"
#include "Heavy/CppExporter.h"
#include "Heavy/DaisyExporter.h"
#include "Heavy/DPFExporter.h"
#include "Heavy/OWLExporter.h"
#include "Heavy/PdExporter.h"
#include "Heavy/ToolchainInstaller.h"
#include "Heavy/WASMExporter.h"
#include "PluginMode.h"
#include "Objects/TextObject.h"
extern "C" {
void canvas_setgraph(t_glist* x, int flag, int nogoprect);
}
#include "Objects/GraphOnParent.h"
#include "Objects/ArrayObject.h"
#include "Objects/BicoeffObject.h"
#include "Objects/ButtonObject.h"
#include "Objects/DropzoneObject.h"
#include "Objects/KeyboardObject.h"
#include "Objects/KnobObject.h"
#include "Sidebar/AutomationPanel.h"
#include "Sidebar/CommandInput.h"
#include "Sidebar/Palettes.h"
#include "Utility/ValueTreeViewer.h"
#include "Sidebar/SearchPanel.h"
#include "Statusbar.h"
#include "Toolbar.h"
#include "Utility/AudioMidiFifo.h"
#include "Utility/Autosave.h"
#include "Utility/ModifierKeyListener.h"
#include "Utility/Recorder.h"

// One broad, deterministic sweep for code paths that are expensive to cover
// with isolated tests. It drives legal values through object properties and
// settings controls, traverses every sidebar, exercises application command
// states, and opens the otherwise dormant plugin-mode and utility UIs.
class CoverageSweepTest : public PlugDataUnitTest
{
public:
    CoverageSweepTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Coverage Sweep Test")
    {
    }

private:
    void perform() override
    {
        beginTest("Object properties, commands and canvas tools");

        cnv = editor->getTabComponent().openPatch(String(
            "#N canvas 100 100 1100 760 12;\n"
            "#X obj 20 20 bng 25 250 50 0 empty empty bang 17 7 0 10 #fcfcfc #000000 #000000;\n"
            "#X obj 70 20 tgl 25 0 empty empty toggle 17 7 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 120 20 hsl 128 17 0 127 0 0 empty empty hslider -2 -8 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 270 20 vsl 17 128 0 127 0 0 empty empty vslider 0 -9 0 10 #fcfcfc #000000 #000000 0 1;\n"
            "#X obj 320 20 nbx 5 14 -10 10 0 0 empty empty number 0 -8 0 10 #fcfcfc #000000 #000000 0 256;\n"
            "#X obj 400 20 hradio 17 1 0 8 empty empty hradio 0 -8 0 10 #fcfcfc #000000 #000000 0;\n"
            "#X obj 560 20 vradio 17 1 0 8 empty empty vradio 0 -8 0 10 #fcfcfc #000000 #000000 0;\n"
            "#X obj 620 20 cnv 15 120 60 empty empty canvas 20 12 0 14 #e0e0e0 #404040 0;\n"
            "#X obj 760 20 vu 17 120 empty empty -1 -8 0 10 #404040 #000000 1 0;\n"
            "#X floatatom 20 190 5 0 0 0 float - - 0;\n"
            "#X symbolatom 120 190 10 0 0 0 symbol - - 0;\n"
            "#X listbox 280 190 20 0 0 0 list - - 0;\n"
            "#X msg 20 240 message coverage;\n"
            "#X text 180 240 comment coverage;\n"
            "#X obj 20 300 knob;\n"
            "#X obj 100 300 function;\n"
            "#X obj 260 300 scope~;\n"
            "#X obj 430 300 keyboard;\n"
            "#X obj 650 300 note;\n"
            "#X obj 20 430 button;\n"
            "#X obj 100 430 pad;\n"
            "#X obj 250 430 popmenu;\n"
            "#X obj 400 430 messbox;\n"
            "#X obj 560 430 bicoeff;\n"
            "#X obj 720 430 numbox~;\n"
            "#X obj 20 540 array define coverage_array 64;\n"
            "#X obj 250 540 colors;\n"
            "#X obj 400 540 openfile -h;\n"
            "#X obj 560 540 notein;\n"
            "#X obj 680 540 noteout;\n"
            "#X obj 20 620 osc~ 220;\n"
            "#X obj 180 620 *~ 0.25;\n"
            "#X obj 350 620 print coverage;\n"
            "#X obj 470 620 canvas.mouse;\n"
            "#X obj 600 620 canvas.vis;\n"
            "#X obj 720 620 canvas.zoom;\n"
            "#X obj 840 620 canvas.edit;\n"
            "#X obj 470 660 keycode;\n"
            "#X obj 570 660 mouse;\n"
            "#X obj 650 660 mousestate;\n"
            "#X obj 770 660 mousefilter;\n"
            "#X obj 900 660 pd~;\n"
            "#X obj 970 660 clone 2 osc~ 440;\n"
            "#X obj 800 540 hello-gui;\n"
            "#X obj 800 580 props;\n"
            "#X obj 920 100 receive coverage_bus;\n"
            "#X obj 920 140 send coverage_bus;\n"
            "#X obj 820 620 dropzone 160 80;\n"
            "#N canvas 0 50 450 250 (subpatch) 0;\n"
            "#X array coverage_graph 64 float 3;\n"
            "#A 0 0 0.1 0.2 0.3 0.4 0.5 0.4 0.3 0.2 0.1 0 -0.1 -0.2 -0.3 -0.4 -0.5;\n"
            "#X coords 0 1 63 -1 180 100 1;\n"
            "#X restore 800 480 graph;\n"
            "#X connect 30 0 31 0;\n"
            "#X connect 12 0 32 0;\n"
            "#X coords 0 -1 1 1 900 680 1 0 0;\n"));

        if (!cnv) {
            signalDone(false);
            return;
        }

        cnv->locked.setValue(false);
        cnv->performSynchronise();
        exerciseCommandsAndCanvas();
        exerciseObjectProperties();
        exerciseObjectMessages();
        exerciseTabsAndFileDrops();

        Timer::callAfterDelay(100, [this] { runSidebar(0); });
    }

    static MouseEvent mouseEvent(Component* target, Point<float> position, ModifierKeys modifiers = ModifierKeys::leftButtonModifier, int clicks = 1)
    {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(),
            position, modifiers, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, target, target,
            Time::getCurrentTime(), position, Time::getCurrentTime(), clicks, false);
    }

    static MouseEvent dragEvent(Component* target, Point<float> position, Point<float> mouseDownPosition,
        ModifierKeys modifiers = ModifierKeys::leftButtonModifier)
    {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(),
            position, modifiers, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, target, target,
            Time::getCurrentTime(), mouseDownPosition, Time::getCurrentTime(), 1, true);
    }

    static void collectComponents(Component* root, HeapArray<Component::SafePointer<Component>>& result)
    {
        if (!root)
            return;
        for (auto* child : root->getChildren()) {
            result.add(child);
            collectComponents(child, result);
        }
    }

    void exerciseControlTree(Component* root, bool mutateComboBoxes = true)
    {
        if (!root)
            return;

        root->createComponentSnapshot(root->getLocalBounds());

        HeapArray<Component::SafePointer<Component>> components;
        collectComponents(root, components);
        for (auto& safe : components) {
            auto* component = safe.getComponent();
            if (!component || !component->isVisible() || !component->isEnabled())
                continue;

            auto const originalBounds = component->getBounds();
            component->setBounds(originalBounds);

            auto const centre = component->getLocalBounds().getCentre().toFloat();
            auto move = mouseEvent(component, centre, ModifierKeys());
            component->mouseEnter(move);
            component->mouseMove(move);
            component->mouseExit(move);

            if (auto* toggle = dynamic_cast<ToggleButton*>(component)) {
                auto const original = toggle->getToggleState();
                toggle->setToggleState(!original, sendNotificationSync);
                toggle->setToggleState(original, sendNotificationSync);
            } else if (auto* combo = dynamic_cast<ComboBox*>(component); combo && mutateComboBoxes) {
                auto const original = combo->getSelectedId();
                if (combo->getNumItems() > 0) {
                    combo->setSelectedId(combo->getItemId(0), sendNotificationSync);
                    combo->setSelectedId(combo->getItemId(combo->getNumItems() - 1), sendNotificationSync);
                    combo->setSelectedId(original, sendNotificationSync);
                }
            } else if (auto* slider = dynamic_cast<Slider*>(component)) {
                auto const original = slider->getValue();
                auto const minimum = slider->getMinimum();
                auto const maximum = slider->getMaximum();
                if (std::isfinite(minimum) && std::isfinite(maximum) && maximum > minimum) {
                    slider->setValue(minimum, sendNotificationSync);
                    slider->setValue(maximum, sendNotificationSync);
                    slider->setValue(original, sendNotificationSync);
                }
            }
        }
    }

    void exerciseCommandsAndCanvas()
    {
        Array<CommandID> commands;
        editor->getAllCommands(commands);
        for (auto const command : commands) {
            ApplicationCommandInfo info(command);
            editor->getCommandInfo(command, info);
        }

        cnv->deselectAll();
        for (int i = 0; i < jmin(5, static_cast<int>(cnv->objects.size())); ++i)
            cnv->setSelected(cnv->objects[i], true, false);
        if (!cnv->connections.empty())
            cnv->setSelected(cnv->connections.front(), true, false);

        for (auto const command : commands) {
            ApplicationCommandInfo info(command);
            editor->getCommandInfo(command, info);
        }

        for (int align = Align::Left; align <= Align::VDistribute; ++align)
            cnv->alignObjects(static_cast<Align>(align));

        for (auto const command : {
                 CommandIDs::ZoomIn, CommandIDs::ZoomOut, CommandIDs::ZoomNormal,
                 CommandIDs::ZoomToFitAll, CommandIDs::GoToOrigin,
                 CommandIDs::ConnectionStyle, CommandIDs::ConnectionPathfind,
                 CommandIDs::SelectAll, CommandIDs::ToggleSnapping,
                 CommandIDs::ToggleLeftSidebar, CommandIDs::ToggleRightSidebar,
                 CommandIDs::ShowBrowser, CommandIDs::Search, CommandIDs::ClearConsole,
                 CommandIDs::NextTab, CommandIDs::PreviousTab, CommandIDs::PanDragKey }) {
            editor->commandManager.invokeDirectly(command, false);
        }
        editor->commandManager.invokeDirectly(CommandIDs::ToggleSnapping, false);
        editor->commandManager.invokeDirectly(CommandIDs::ToggleLeftSidebar, false);
        editor->commandManager.invokeDirectly(CommandIDs::ToggleRightSidebar, false);

        editor->commandManager.invokeDirectly(CommandIDs::Lock, false);
        editor->commandManager.invokeDirectly(CommandIDs::Lock, false);
        editor->commandManager.invokeDirectly(CommandIDs::TogglePresentationMode, false);
        editor->commandManager.invokeDirectly(CommandIDs::TogglePresentationMode, false);
        editor->commandManager.invokeDirectly(CommandIDs::ToggleDSP, false);
        editor->commandManager.invokeDirectly(CommandIDs::ToggleDSP, false);

        cnv->shiftKeyChanged(true);
        cnv->shiftKeyChanged(false);
        cnv->commandKeyChanged(true);
        cnv->commandKeyChanged(false);
        cnv->middleMouseChanged(true);
        cnv->middleMouseChanged(false);
        cnv->altKeyChanged(true);
        cnv->altKeyChanged(false);
        cnv->cycleSelection();
        cnv->keyPressed(KeyPress(KeyPress::tabKey));
        cnv->keyPressed(KeyPress(KeyPress::escapeKey));

        if (!cnv->objects.empty()) {
            cnv->activateCanvasSearchHighlight({ 40.0f, 40.0f }, cnv->objects.front());
            cnv->removeCanvasSearchHighlight();
        }

        cnv->dragAndDropPaste(
            "#N canvas 0 0 100 100 12;\n#X obj 20 20 osc~ 110;\n",
            { 850, 620 }, 100, 100);
        cnv->performSynchronise();
        exerciseObjectCreationCommands();
    }

    void exerciseObjectCreationCommands()
    {
        for (int command = ObjectIDs::NewComment; command < ObjectIDs::NumObjects; ++command) {
            editor->commandManager.invokeDirectly(command, false);
            cnv->performSynchronise();
        }

        cnv->deselectAll();
        for (int i = 0; i < jmin(3, static_cast<int>(cnv->objects.size())); ++i)
            cnv->setSelected(cnv->objects[i], true, false);

        for (auto const command : {
                 CommandIDs::Copy, CommandIDs::Duplicate, CommandIDs::Tidy,
                 CommandIDs::Triggerize, CommandIDs::Undo, CommandIDs::Redo }) {
            editor->commandManager.invokeDirectly(command, false);
            cnv->performSynchronise();
        }
    }

    void exerciseDirectCommands()
    {
        auto invoke = [this](CommandID const command) {
            ApplicationCommandTarget::InvocationInfo info(command);
            editor->perform(info);
            if (cnv)
                cnv->performSynchronise();
        };

        cnv->deselectAll();
        for (int i = 0; i < jmin(3, static_cast<int>(cnv->objects.size())); ++i)
            cnv->setSelected(cnv->objects[i], true, false);
        if (!cnv->connections.empty())
            cnv->setSelected(cnv->connections.front(), true, false);

        for (auto const command : {
                 CommandIDs::Copy, CommandIDs::Duplicate, CommandIDs::Tidy,
                 CommandIDs::Triggerize, CommandIDs::SelectAll,
                 CommandIDs::ConnectionStyle, CommandIDs::ConnectionPathfind,
                 CommandIDs::ZoomIn, CommandIDs::ZoomOut, CommandIDs::ZoomNormal,
                 CommandIDs::ZoomToFitAll, CommandIDs::GoToOrigin,
                 CommandIDs::PanDragKey, CommandIDs::Undo, CommandIDs::Redo,
                 CommandIDs::NextTab, CommandIDs::PreviousTab })
            invoke(command);

        invoke(CommandIDs::Lock);
        invoke(CommandIDs::Lock);
        invoke(CommandIDs::TogglePresentationMode);
        invoke(CommandIDs::TogglePresentationMode);

        cnv->deselectAll();
        if (!cnv->objects.empty())
            cnv->setSelected(cnv->objects.front(), true, false);

        invoke(CommandIDs::ShowReference);
        editor->openedDialog.reset(nullptr);
        invoke(CommandIDs::OpenObjectBrowser);
        editor->openedDialog.reset(nullptr);
        invoke(CommandIDs::ShowSettings);
        invoke(CommandIDs::ShowSettings);

        for (auto const command : {
                 CommandIDs::ShowBrowser, CommandIDs::Search, CommandIDs::ClearConsole,
                 CommandIDs::ToggleLeftSidebar, CommandIDs::ToggleRightSidebar,
                 CommandIDs::ToggleSnapping })
            invoke(command);

        invoke(CommandIDs::ToggleLeftSidebar);
        invoke(CommandIDs::ToggleRightSidebar);
        invoke(CommandIDs::ToggleSnapping);
    }

    static var alternateValue(ObjectParameter const& parameter)
    {
        auto const original = parameter.valuePtr->getValue();
        switch (parameter.type) {
        case tBool:
            return !static_cast<bool>(original);
        case tInt: {
            auto const value = static_cast<int>(original);
            auto const minimum = static_cast<int>(parameter.min);
            auto const maximum = static_cast<int>(parameter.max);
            return jlimit(minimum, maximum, value == maximum ? minimum : value + 1);
        }
        case tFloat: {
            auto const value = static_cast<double>(original);
            auto const candidate = value + 0.5;
            return parameter.clip ? jlimit(parameter.min, parameter.max, candidate) : candidate;
        }
        case tCombo:
            return parameter.options.size() > 1 ? 1 : 0;
        case tString:
            return original.toString() + "_coverage";
        case tColour:
        case tColourAlpha:
            return Colour(0xff336699).toString();
        case tRangeFloat:
        case tRangeInt: {
            if (auto const* values = original.getArray(); values && values->size() >= 2) {
                Array<var> changed(*values);
                changed.set(0, static_cast<double>(changed[0]) + 1.0);
                changed.set(1, static_cast<double>(changed[1]) + 2.0);
                return changed;
            }
            return Array<var> { 1, 2 };
        }
        case tFont:
            return original;
        case tCustom:
            return {};
        }
        return original;
    }

    void exerciseObjectProperties()
    {
        HeapArray<Component::SafePointer<Object>> objects;
        for (auto* object : cnv->objects)
            objects.add(object);

        for (auto& safe : objects) {
            auto* object = safe.getComponent();
            if (!object || !object->gui)
                continue;

            auto* gui = object->gui.get();
            auto const type = gui->getType();
            // The keyboard external rebroadcasts its size/property messages as
            // note data, so generic mutation feeds its outlet back into its
            // own inlet. It is exercised separately by the object fuzz tests.
            if (type == "keyboard")
                continue;

            gui->updateProperties();
            gui->getPdBounds();
            gui->getSelectableBounds();
            gui->setPdBounds(gui->getPdBounds());
            gui->lock(true);
            gui->lock(false);
            gui->canReceiveMouseEvent(1, 1);
            gui->checkHvccCompatibility();

            PopupMenu menu;
            gui->getMenuOptions(menu);

            auto parameters = gui->getParameters().getParameters();
            for (auto& parameter : parameters) {
                if (parameter.type == tCustom && parameter.createFn) {
                    std::unique_ptr<Component> custom(parameter.createFn());
                    custom->setBounds(0, 0, 500, 240);
                    exerciseControlTree(custom.get());
                    continue;
                }
                if (!parameter.valuePtr)
                    continue;

                auto const original = parameter.valuePtr->getValue();
                if (parameter.interactionFn)
                    parameter.interactionFn(true);
                parameter.valuePtr->setValue(alternateValue(parameter));
                if (parameter.interactionFn)
                    parameter.interactionFn(false);
                parameter.valuePtr->setValue(original);
            }

            auto const centre = gui->getLocalBounds().getCentre().toFloat();
            auto event = mouseEvent(gui, centre);
            gui->mouseEnter(event);
            gui->mouseMove(event);
            gui->mouseDown(event);
            gui->mouseDrag(mouseEvent(gui, centre + Point<float>(2, 2)));
            gui->mouseUp(event);
            gui->mouseDoubleClick(mouseEvent(gui, centre, ModifierKeys::leftButtonModifier, 2));
            gui->mouseExit(event);
            gui->keyPressed(KeyPress(KeyPress::upKey));
            gui->keyPressed(KeyPress(KeyPress::downKey));
            gui->keyPressed(KeyPress(KeyPress::returnKey));
            gui->focusGained(Component::focusChangedDirectly);
            gui->focusLost(Component::focusChangedDirectly);
        }

        cnv->performSynchronise();
    }

    void exerciseObjectMessages()
    {
        beginTest("Object messages, graphical arrays and drop targets");

        auto const number = SmallArray<pd::Atom> { pd::Atom(1.0f) };
        auto const pair = SmallArray<pd::Atom> { pd::Atom(2.0f), pd::Atom(5.0f) };
        auto const symbol = SmallArray<pd::Atom> { pd::Atom(gensym("coverage")) };

        for (auto* object : cnv->objects) {
            if (!object->gui)
                continue;

            if (auto* knob = dynamic_cast<KnobObject*>(object->gui.get())) {
                for (auto const selector : {
                         "float", "list", "set", "reload", "inc", "dec", "shift",
                         "angle", "offset", "arc", "start", "discrete", "circular",
                         "square", "readonly", "number", "numbersize", "active",
                         "jump", "arcstart", "exp", "log", "ticks", "transparent" })
                    knob->receiveObjectMessage(hash(selector), number);
                knob->receiveObjectMessage(hash("range"), pair);
                knob->receiveObjectMessage(hash("numberpos"), pair);
                knob->receiveObjectMessage(hash("send"), symbol);
                knob->receiveObjectMessage(hash("receive"), symbol);
                knob->receiveObjectMessage(hash("var"), symbol);
                knob->receiveObjectMessage(hash("param"), symbol);
                knob->receiveObjectMessage(hash("readonly"), { pd::Atom(0.0f) });
                for (auto const selector : { "fgcolor", "bgcolor", "colors", "arccolor", "init" })
                    knob->receiveObjectMessage(hash(selector), {});

                if (auto* control = TestHelpers::findChildOfType<Knob>(knob)) {
                    auto const centre = control->getLocalBounds().getCentre().toFloat();
                    control->mouseDown(mouseEvent(control, centre));
                    control->mouseDrag(dragEvent(control, { centre.x + 12.0f, centre.y - 18.0f }, centre));
                    control->mouseDrag(dragEvent(control, { 2.0f, 2.0f }, centre));
                    control->mouseDrag(dragEvent(control, { control->getWidth() - 2.0f, 2.0f }, centre));
                    control->mouseUp(mouseEvent(control, centre));
                    MouseWheelDetails wheel;
                    wheel.deltaY = 0.25f;
                    control->grabKeyboardFocus();
                    control->mouseWheelMove(mouseEvent(control, centre), wheel);
                }

                knob->grabKeyboardFocus();
                for (auto const key : {
                         KeyPress(KeyPress::upKey), KeyPress(KeyPress::rightKey),
                         KeyPress(KeyPress::downKey), KeyPress(KeyPress::leftKey),
                         KeyPress('1'), KeyPress('.'), KeyPress('5'),
                         KeyPress(KeyPress::backspaceKey), KeyPress(KeyPress::returnKey) })
                    knob->keyPressed(key);
                knob->canReceiveMouseEvent(1, 1);
            }

            if (auto* keyboard = dynamic_cast<KeyboardObject*>(object->gui.get())) {
                keyboard->receiveObjectMessage(hash("float"), number);
                keyboard->receiveObjectMessage(hash("list"), pair);
                keyboard->receiveObjectMessage(hash("set"), {});
                keyboard->receiveObjectMessage(hash("on"), pair);
                keyboard->receiveObjectMessage(hash("off"), pair);
                for (auto const selector : { "lowc", "width", "oct", "8ves", "toggle" })
                    keyboard->receiveObjectMessage(hash(selector), number);
                keyboard->receiveObjectMessage(hash("send"), symbol);
                keyboard->receiveObjectMessage(hash("receive"), symbol);
                keyboard->receiveObjectMessage(hash("flush"), {});
                keyboard->createConstrainer();
            }

            if (auto* button = dynamic_cast<ButtonObject*>(object->gui.get())) {
                for (auto const selector : {
                         "bgcolor", "fgcolor", "readonly", "transparent", "oval",
                         "float", "click", "latch", "bang", "toggle" })
                    button->receiveObjectMessage(hash(selector), number);
            }

            if (auto* filter = dynamic_cast<BicoeffObject*>(object->gui.get())) {
                for (auto const selector : {
                         "allpass", "lowpass", "highpass", "bandpass", "bandstop",
                         "resonant", "eq", "lowshelf", "highshelf" }) {
                    filter->receiveObjectMessage(hash(selector), {});
                    filter->createComponentSnapshot(filter->getLocalBounds());
                }
            }

            if (auto* dropzone = dynamic_cast<DropzoneObject*>(object->gui.get())) {
                StringArray files { "/tmp/coverage-one.txt", "/tmp/coverage-two.txt" };
                dropzone->isLocked();
                dropzone->isInterestedInFileDrag(files);
                dropzone->fileDragEnter(files, 2, 3);
                dropzone->fileDragMove(files, 4, 5);
                dropzone->fileDragExit(files);
                dropzone->filesDropped(files, 6, 7);
                dropzone->isInterestedInTextDrag("coverage text");
                dropzone->textDragEnter("coverage text", 2, 3);
                dropzone->textDragMove("coverage text", 4, 5);
                dropzone->textDragExit("coverage text");
                dropzone->textDropped("coverage text", 6, 7);
                dropzone->createComponentSnapshot(dropzone->getLocalBounds());
            }

            if (auto* array = dynamic_cast<ArrayObject*>(object->gui.get()))
                exerciseGraphicalArray(array, object);
        }
    }

    void exerciseGraphicalArray(ArrayObject* array, Object* object)
    {
        auto const arrays = array->getArrays();
        SmallArray<GraphicalArray*> graphs;
        for (auto* child : array->getChildren()) {
            if (auto* graph = dynamic_cast<GraphicalArray*>(child))
                graphs.add(graph);
        }

        array->canReceiveMouseEvent(1, 1);
        array->getPdBounds();
        array->setPdBounds(array->getPdBounds());
        array->updateGraphs();
        array->update();
        array->updateSizeProperty();
        array->resized();
        array->receiveObjectMessage(hash("redraw"), {});
        for (auto const selector : { "yticks", "xticks", "ylabel", "xlabel" })
            array->receiveObjectMessage(hash(selector), {});

        HeapArray<float> empty;
        GraphicalArray::rescale(empty, 4);
        HeapArray<float> points { 0.0f, 0.25f, -0.25f, 0.5f, -0.5f, 0.0f };
        GraphicalArray::rescale(points, 12);
        for (auto const style : { GraphicalArray::Points, GraphicalArray::Polygon, GraphicalArray::Curve })
            GraphicalArray::createArrayPath(points, style, { -1.0f, 1.0f }, 3.0f, 80.0f, 1.0f);

        for (auto* graph : graphs) {
            graph->setBounds(0, 0, 240, 120);
            graph->update();
            graph->updateParameters();
            graph->getUnexpandedName();
            graph->getArraySize();
            graph->getLineWidth();
            graph->getDrawType();
            graph->getScale();
            graph->willSaveContent();

            graph->receiveMessage(gensym("edit"), { pd::Atom(1.0f) });
            graph->receiveMessage(gensym("color"), {});
            graph->receiveMessage(gensym("width"), {});
            graph->receiveMessage(gensym("style"), { pd::Atom(0.0f) });
            graph->receiveMessage(gensym("style"), { pd::Atom(1.0f) });
            graph->receiveMessage(gensym("style"), { pd::Atom(2.0f) });
            graph->receiveMessage(gensym("xticks"), {});
            graph->receiveMessage(gensym("yticks"), {});
            graph->receiveMessage(gensym("xlabel"), {});
            graph->receiveMessage(gensym("ylabel"), {});
            graph->receiveMessage(gensym("vis"), { pd::Atom(0.0f) });
            graph->receiveMessage(gensym("vis"), { pd::Atom(1.0f) });
            graph->receiveMessage(gensym("resize"), { pd::Atom(80.0f) });

            graph->range = var(VarArray { -2.0f, 2.0f });
            graph->drawMode = 1;
            graph->drawMode = 2;
            graph->drawMode = 3;
            graph->saveContents = !static_cast<bool>(graph->saveContents.getValue());
            graph->saveContents = !static_cast<bool>(graph->saveContents.getValue());

            auto down = mouseEvent(graph, { 10.0f, 30.0f });
            graph->mouseDown(down);
            graph->mouseDrag(mouseEvent(graph, { 180.0f, 90.0f }));
            graph->mouseUp(mouseEvent(graph, { 180.0f, 90.0f }));
            graph->createComponentSnapshot(graph->getLocalBounds());
        }

        ArrayPropertiesPanel properties([array] { array->addArray(); }, [this] { cnv->synchronise(); });
        SmallArray<Component::SafePointer<GraphicalArray>> safeGraphs;
        for (auto* graph : graphs)
            safeGraphs.add(graph);
        properties.setBounds(0, 0, 420, 500);
        properties.reloadGraphs(safeGraphs);
        properties.resized();
        properties.createComponentSnapshot(properties.getLocalBounds());
        properties.addButton.mouseEnter(mouseEvent(&properties.addButton, { 10.0f, 10.0f }));
        properties.addButton.mouseExit(mouseEvent(&properties.addButton, { 10.0f, 10.0f }));
        properties.addButton.hitTest(10, 10);

        if (arrays.not_empty()) {
            ArrayListView list(editor->pd, arrays.front());
            list.setBounds(0, 0, 500, 320);
            list.parentSizeChanged();
            list.update();
            exerciseControlTree(&list);

            ArrayEditorDialog dialog(editor->pd, arrays, object);
            dialog.setBounds(0, 0, 620, 420);
            dialog.resized();
            dialog.updateGraphs();
            dialog.updateVisibleGraph();
            exerciseControlTree(&dialog);
            dialog.createComponentSnapshot(dialog.getLocalBounds());
            dialog.removeFromDesktop();
        }
    }

    void exerciseTabsAndFileDrops()
    {
        beginTest("Tabs, split views and file drops");

        auto& tabs = editor->getTabComponent();
        auto* second = tabs.openPatch("#N canvas 0 0 320 240 12;\n#X obj 20 20 print second;\n");
        auto* third = tabs.openPatch("#N canvas 0 0 320 240 12;\n#X obj 20 20 print third;\n");
        expect(second && third, "additional patches must open");

        if (second)
            second->patch.splitViewIndex = 1;
        if (third)
            third->patch.splitViewIndex = 0;
        tabs.updateNow();
        tabs.setBounds(tabs.getBounds());
        tabs.showTab(cnv, 0);
        if (second)
            tabs.showTab(second, 1);
        tabs.setActiveSplit(cnv);
        tabs.nextTab();
        tabs.previousTab();
        tabs.getCanvases();
        tabs.getVisibleCanvases();
        tabs.getCanvasAtScreenPosition(tabs.localPointToGlobal(tabs.getLocalBounds().getCentre()));
        tabs.createComponentSnapshot(tabs.getLocalBounds());

        HeapArray<Component::SafePointer<Component>> tabChildren;
        collectComponents(&tabs, tabChildren);
        for (auto& safe : tabChildren) {
            auto* component = safe.getComponent();
            if (!component)
                continue;

            if (auto* button = dynamic_cast<Button*>(component);
                button && component->getParentComponent() == &tabs) {
                button->triggerClick();
                PopupMenu::dismissAllActiveMenus();
            }

            if (component->getWidth() < 60 || component->getHeight() > 40)
                continue;
            auto const centre = component->getLocalBounds().getCentre().toFloat();
            component->mouseDown(mouseEvent(component, centre));
            component->mouseDrag(mouseEvent(component, centre + Point<float>(20.0f, 0.0f)));
            component->mouseUp(mouseEvent(component, centre + Point<float>(20.0f, 0.0f)));
            component->mouseDown(mouseEvent(component, centre, ModifierKeys::rightButtonModifier));
            PopupMenu::dismissAllActiveMenus();
        }

        tabs.showTab(cnv, 0);
        tabs.setActiveSplit(cnv);

        auto dropDir = File::getSpecialLocation(File::tempDirectory).getChildFile("plugdata_coverage_drop");
        dropDir.createDirectory();
        auto const patch = dropDir.getChildFile("dropped.pd");
        patch.replaceWithText("#N canvas 0 0 200 150 12;\n#X obj 20 20 print dropped;\n");
        auto const text = dropDir.getChildFile("dropped.txt");
        text.replaceWithText("coverage");

        StringArray patchFiles { patch.getFullPathName() };
        StringArray textFiles { text.getFullPathName() };
        editor->isInterestedInFileDrag(patchFiles);
        editor->isInterestedInFileDrag(textFiles);
        editor->isInterestedInFileDrag({ "/not/a/file" });
        editor->fileDragEnter(patchFiles, editor->getWidth() / 2, editor->getHeight() / 2);
        editor->fileDragMove(patchFiles, editor->getWidth() / 2, editor->getHeight() / 2);
        editor->fileDragExit(patchFiles);
        editor->filesDropped(textFiles, editor->getWidth() / 2, editor->getHeight() / 2);
        editor->filesDropped(patchFiles, editor->getWidth() / 2, editor->getHeight() / 2);
        tabs.updateNow();
        tabs.showTab(cnv, 0);
        tabs.setActiveSplit(cnv);

        exerciseArrayFileDrop(dropDir);
    }

    void exerciseArrayFileDrop(File const& directory)
    {
        auto const wav = directory.getChildFile("coverage.wav");
        {
            auto stream = std::unique_ptr<OutputStream>(wav.createOutputStream());
            WavAudioFormat format;
            AudioFormatWriterOptions options;
            options = options.withSampleRate(44100.0).withNumChannels(1).withBitsPerSample(16);
            auto writer = format.createWriterFor(stream, options);
            if (writer) {
                AudioBuffer<float> audio(1, 128);
                for (int i = 0; i < audio.getNumSamples(); ++i)
                    audio.setSample(0, i, std::sin(MathConstants<float>::twoPi * i / 32.0f) * 0.25f);
                writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples());
            }
        }

        for (auto* object : cnv->objects) {
            if (!object->gui)
                continue;

            HeapArray<Component::SafePointer<Component>> components;
            components.add(object->gui.get());
            collectComponents(object->gui.get(), components);
            for (auto& safe : components) {
                auto* graph = dynamic_cast<GraphicalArray*>(safe.getComponent());
                if (!graph)
                    continue;
                StringArray files { wav.getFullPathName() };
                graph->isInterestedInFileDrag(files);
                graph->fileDragEnter(files, 2, 2);
                graph->fileDragExit(files);
                graph->filesDropped(files, 5, 5);
            }
        }
        cnv->performSynchronise();
    }

    void runSidebar(int panel)
    {
        if (panel >= Sidebar::NumSidePanels) {
            runSettingsPanel(0);
            return;
        }

        beginTest("Sidebar panel " + String(panel));
        auto const id = static_cast<Sidebar::SidePanel>(panel);
        editor->movePanelToSide(id, Sidebar::Side::Left);
        editor->movePanelToSide(id, Sidebar::Side::Right);
        if (auto* sidebar = editor->getSidebarForPanel(id)) {
            sidebar->showSidebar(true);
            sidebar->showPanel(id);
            sidebar->updateConsole(3, true);
            sidebar->updateSearch(false);
            sidebar->updateAutomationParameters();
            sidebar->setCommandTarget("coverage");
            exerciseControlTree(sidebar);
            sidebar->showSidebar(false);
            sidebar->showSidebar(true);
        }

        Timer::callAfterDelay(40, [this, panel] { runSidebar(panel + 1); });
    }

    void runSettingsPanel(int panel)
    {
        if (panel >= 6) {
            exerciseTargetedComponents();
            return;
        }

        beginTest("Interactive settings panel " + String(panel));
        Dialogs::showSettingsDialog(editor, panel);
        Timer::callAfterDelay(100, [this, panel] {
            exerciseControlTree(editor->openedDialog.get(), false);
            editor->openedDialog.reset(nullptr);
            Timer::callAfterDelay(30, [this, panel] { runSettingsPanel(panel + 1); });
        });
    }

    void exerciseTargetedComponents()
    {
        beginTest("Search, palettes, automation and settings callbacks");

        SearchPanel search(editor);
        search.setBounds(0, 0, 520, 420);
        search.setVisible(true);
        auto tree = search.generatePatchTree(cnv->refCountedPatch);
        expect(tree.getNumChildren() > 0, "search tree must contain the sweep objects");
        search.updateResults();
        search.timerCallback();
        search.grabFocus();
        search.lookAndFeelChanged();
        search.createComponentSnapshot(search.getLocalBounds());
        auto searchSettings = search.getExtraSettingsComponent();
        searchSettings->setBounds(0, 0, 30, 30);
        exerciseControlTree(searchSettings.get());
        search.clear();

        Palettes palettes(editor);
        palettes.setBounds(0, 0, 420, 500);
        palettes.updateSearch("osc");
        palettes.updateSearch("not-present");
        palettes.updateSearch("");
        Array<var> paletteState;
        palettes.initialisePalettes(paletteState);
        palettes.populateValueTree(paletteState);
        palettes.updateSearch("slider");
        palettes.createComponentSnapshot(palettes.getLocalBounds());
        exercisePalettes(palettes);

        themeSettings = std::make_unique<ThemeSettingsPanel>(editor->pd);
        auto& themes = *themeSettings;
        themes.setBounds(0, 0, 760, 560);
        themes.updateSwatches();
        themes.handleAsyncUpdate();
        themes.updateThemeNames("light", "dark");
        themes.resized();
        exerciseControlTree(themes.getPropertiesPanel());
        HeapArray<Component::SafePointer<Component>> themeControls;
        collectComponents(themes.getPropertiesPanel(), themeControls);
        for (auto& safe : themeControls) {
            if (auto* boolean = dynamic_cast<PropertiesPanel::BoolComponent*>(safe.getComponent())) {
                auto const centre = boolean->getLocalBounds().getCentre().toFloat();
                boolean->mouseUp(mouseEvent(boolean, centre));
                boolean->mouseUp(mouseEvent(boolean, centre));
            }
        }
        themes.resetDefaults();
        themes.createComponentSnapshot(themes.getLocalBounds());

        int newThemeResult = -1;
        NewThemeDialog newTheme(nullptr, [&newThemeResult](int result, String, String) {
            newThemeResult = result;
        });
        newTheme.setBounds(0, 0, 400, 170);
        newTheme.resized();
        if (auto* ok = TestHelpers::findButtonWithText(&newTheme, "OK")) {
            ok->triggerClick();
            newTheme.createComponentSnapshot(newTheme.getLocalBounds());
            if (auto* name = TestHelpers::findChildOfType<TextEditor>(&newTheme)) {
                name->setText(PlugDataLook::getAllThemes()[0], sendNotificationSync);
                ok->triggerClick();
                newTheme.createComponentSnapshot(newTheme.getLocalBounds());
            }
        }
        ignoreUnused(newThemeResult);

        int themeSelectorChanges = 0;
        ThemeSelectorProperty selector("Coverage theme", [&themeSelectorChanges](String const&) {
            ++themeSelectorChanges;
        });
        selector.setOptions(PlugDataLook::getAllThemes());
        selector.setBounds(0, 0, 300, 30);
        selector.resized();
        selector.setSelectedItem(0);
        if (selector.comboBox.getNumItems() > 1)
            selector.comboBox.setSelectedItemIndex(1, sendNotificationSync);
        std::unique_ptr<PropertiesPanelProperty> selectorCopy(selector.createCopy());
        selector.createComponentSnapshot(selector.getLocalBounds());
        ignoreUnused(themeSelectorChanges);

        SearchPathPanel paths;
        paths.setBounds(0, 0, 760, 520);
        paths.onChange = [this] { ++settingsCallbackCount; };
        auto const tempDirectory = File::getSpecialLocation(File::tempDirectory);
        paths.isInterestedInFileDrag({ tempDirectory.getFullPathName() });
        paths.filesDropped({ tempDirectory.getFullPathName(), "/not/a/directory" }, 10, 10);
        paths.selectedRowsChanged(0);
        paths.deleteKeyPressed(10000);
        paths.createComponentSnapshot(paths.getLocalBounds());
        exercisePathEditors();
        exerciseCoverageGaps();

        if (SystemStats::getEnvironmentVariable("PLUGDATA_COVERAGE_GAPS_ONLY", {}) == "1") {
            signalDone(true);
            return;
        }

        exerciseSuggestions();
        exerciseWelcomePanel();
        exerciseDialogsAndMenus();
        exerciseExporters();
        exerciseUtilitiesAndChrome();
        exerciseAutomation();
        exerciseProcessor();
        exerciseDirectCommands();

        Timer::callAfterDelay(100, [this] { exerciseValueTree(); });
    }

    void exercisePalettes(Palettes& palettes)
    {
        HeapArray<Component::SafePointer<Component>> paletteControls;
        collectComponents(&palettes, paletteControls);
        for (auto& safe : paletteControls) {
            auto* selector = dynamic_cast<PaletteSelector*>(safe.getComponent());
            if (!selector)
                continue;

            auto const originalName = selector->getButtonText();
            selector->getTree();
            selector->setTextToShow("Coverage Palette");
            selector->animateToPosition(selector->getBounds().translated(4, 0));
            selector->getTargetBounds();
            selector->cancelAnimation(selector->getBounds());

            if (auto* label = TestHelpers::findChildOfType<Label>(selector)) {
                label->showEditor();
                if (auto* input = label->getCurrentTextEditor())
                    input->setText("Coverage Palette Renamed", sendNotificationSync);
                label->hideEditor(false);

                label->showEditor();
                if (auto* input = label->getCurrentTextEditor())
                    input->setText(originalName, sendNotificationSync);
                label->hideEditor(false);
            }

            selector->exportClicked();

            auto const centre = selector->getLocalBounds().getCentre().toFloat();
            Component* paletteComponent = &palettes;
            paletteComponent->mouseDrag(dragEvent(selector, centre + Point<float>(8.0f, 0.0f), centre));
            paletteComponent->mouseUp(mouseEvent(selector, centre));
            break;
        }

        ValueTree category("Category");
        category.setProperty("Name", "Coverage", nullptr);
        for (int i = 0; i < 3; ++i) {
            ValueTree item("Item");
            item.setProperty("Name", "Item " + String(i), nullptr);
            item.setProperty("Patch", "#X obj 0 0 print coverage;", nullptr);
            category.appendChild(item, nullptr);
        }

        BouncingViewport viewport;
        PaletteList list(editor, category);
        viewport.setViewedComponent(&list, false);
        viewport.setBounds(0, 0, 360, 180);
        list.setBounds(0, 0, 340, 180);
        list.resized();

        auto const previousClipboard = SystemClipboard::getTextFromClipboard();
        SystemClipboard::copyTextToClipboard(
            "#N canvas 0 0 200 150 12;\n"
            "#N canvas 0 0 100 80 coverage_subpatch 0;\n"
            "#X obj 10 10 osc~ 220;\n"
            "#X restore 20 20 pd coverage_item;\n");
        list.pasteButton.mouseUp(mouseEvent(&list.pasteButton, { 10.0f, 10.0f }));
        SystemClipboard::copyTextToClipboard(previousClipboard);

        if (list.items.size() > 1) {
            auto* item = list.items[0];
            auto* reorder = item->reorderButton.get();
            auto const centre = reorder->getLocalBounds().getCentre().toFloat();
            list.mouseDown(mouseEvent(reorder, centre));
            list.mouseDrag(dragEvent(reorder, centre + Point<float>(0.0f, 12.0f), centre));
            list.updateItems();
            list.mouseDrag(dragEvent(reorder, centre + Point<float>(0.0f, 70.0f), centre));
            list.mouseUp(mouseEvent(reorder, centre));
        }
        list.createComponentSnapshot(list.getLocalBounds());
    }

    void exerciseSuggestions()
    {
        auto* blank = cnv->objects.add(cnv, "", Point<int>(900, 650));
        blank->showEditor();
        if (auto* input = TestHelpers::findChildOfType<TextEditor>(blank)) {
            auto* suggestor = cnv->suggestor.get();
            suggestor->setSize(720, 340);

            {
                input->setText("osc", sendNotificationSync);
                AutoCompleteComponent completion(input, cnv);
                completion.setBounds(input->getBounds());
                completion.setSuggestion("osc~");
                completion.hasGhostText();
                completion.getCompletedText();
                completion.createComponentSnapshot(completion.getLocalBounds());
                if (auto* context = editor->getNanoLLGC(); context && context->getContext())
                    completion.render(context->getContext());
                completion.accept();
                completion.setSuggestion("not-a-prefix");
                completion.clear();
                completion.setEnabled(false);
                completion.setSuggestion("osc~");
                completion.setEnabled(true);
                input->setBounds(input->getBounds().translated(1, 0));
            }

            for (auto const& query : {
                     String("osc"), String("osc~ 440"), String("send cov"),
                     String("receive cov"), String("zzzz_no_such_object"), String() }) {
                input->setText(query, sendNotificationSync);
                suggestor->updateSuggestions(query);
                suggestor->updateBounds();
                suggestor->isShowingDetailPanel();
                suggestor->setVisible(true);
                auto* keyListener = static_cast<KeyListener*>(suggestor);
                keyListener->keyPressed(KeyPress(KeyPress::downKey), input);
                keyListener->keyPressed(KeyPress(KeyPress::upKey), input);
                keyListener->keyPressed(KeyPress(KeyPress::rightKey), input);
                keyListener->keyPressed(KeyPress(KeyPress::tabKey), input);
                keyListener->keyPressed(KeyPress(KeyPress::returnKey), input);
            }
            input->setText("trigger", sendNotificationSync);
            suggestor->updateSuggestions("trigger");
            suggestor->setSize(360, 190);
            static_cast<Component*>(suggestor)->resized();
            suggestor->setSize(720, 340);
            static_cast<Component*>(suggestor)->resized();
            suggestor->createComponentSnapshot(suggestor->getLocalBounds());
        }
        blank->hideEditor();

        for (auto* object : cnv->objects) {
            if (object->getType(false) != "msg")
                continue;

            object->showEditor();
            if (auto* input = TestHelpers::findChildOfType<TextEditor>(object)) {
                input->setText("b", sendNotificationSync);
                cnv->suggestor->updateSuggestions("b");
                cnv->suggestor->setSize(360, 190);
                cnv->suggestor->createComponentSnapshot(cnv->suggestor->getLocalBounds());
            }
            object->hideEditor();
            break;
        }
    }

    void exerciseWelcomePanel()
    {
        auto* welcome = editor->welcomePanel.get();
        auto& recentlyOpened = SettingsFile::getInstance()->getProperty<VarArray>("recently_opened");
        auto const originalRecentlyOpened = recentlyOpened;
        auto const tempPatch = File::getSpecialLocation(File::tempDirectory)
                                   .getChildFile("plugdata-welcome-coverage.pd");
        tempPatch.replaceWithText("#N canvas 0 0 200 120 12;\n#X obj 20 20 osc~ 440;\n");
        DynamicObject::Ptr recent(new DynamicObject());
        recent->setProperty("path", tempPatch.getFullPathName());
        recent->setProperty("time", static_cast<int64>(Time::getCurrentTime().toMilliseconds()));
        recent->setProperty("pinned", true);
        recent->setProperty("removable", true);
        recentlyOpened.add(var(recent.get()));

        welcome->setBounds(0, 0, 1000, 700);
        welcome->show();
        welcome->handleAsyncUpdate();
        welcome->resized();
        if (auto* context = editor->getNanoLLGC(); context && context->getContext())
            welcome->render(context->getContext());
        HeapArray<Component::SafePointer<Component>> welcomeControls;
        collectComponents(welcome, welcomeControls);
        for (auto& safe : welcomeControls) {
            auto* component = safe.getComponent();
            if (!component || !component->isVisible() || component->getWidth() < 100 || component->getHeight() < 80)
                continue;
            auto const centre = component->getLocalBounds().getCentre().toFloat();
            component->hitTest(static_cast<int>(centre.x), static_cast<int>(centre.y));
            component->mouseDown(mouseEvent(component, centre, ModifierKeys::rightButtonModifier));
            component->mouseEnter(mouseEvent(component, centre, ModifierKeys()));
            component->mouseExit(mouseEvent(component, centre, ModifierKeys()));
            PopupMenu::dismissAllActiveMenus();
        }
        welcome->setSearchQuery("coverage");
        welcome->setSearchQuery("");
        welcome->setShownTab(WelcomePanel::Library);
        welcome->handleAsyncUpdate();
        if (auto* context = editor->getNanoLLGC(); context && context->getContext())
            welcome->render(context->getContext());
        welcome->setSearchQuery("not-present");
        welcome->findLibraryPatches();
        welcome->setShownTab(WelcomePanel::Home);
        welcome->handleAsyncUpdate();
        welcome->lookAndFeelChanged();
        welcome->hide();

        recentlyOpened.clear();
        recentlyOpened.addArray(originalRecentlyOpened);
        welcome->handleAsyncUpdate();
        tempPatch.deleteFile();
    }

    void exercisePathEditors()
    {
        auto* settings = SettingsFile::getInstance();
        auto& savedPaths = settings->getProperty<VarArray>("paths");
        auto& savedLibraries = settings->getProperty<VarArray>("libraries");
        auto const originalPaths = savedPaths;
        auto const originalLibraries = savedLibraries;

        auto const tempDirectory = File::getSpecialLocation(File::tempDirectory)
                                       .getChildFile("plugdata-path-coverage");
        tempDirectory.createDirectory();

        SearchPathPanel searchPaths;
        searchPaths.setBounds(0, 0, 760, 420);
        searchPaths.filesDropped({ tempDirectory.getFullPathName() }, 0, 0);
        searchPaths.resized();
        searchPaths.getContentXAndWidth();
        searchPaths.createComponentSnapshot(searchPaths.getLocalBounds());
        if (auto* list = TestHelpers::findChildOfType<ListBox>(&searchPaths); list && searchPaths.getNumRows() > 0) {
            auto const row = searchPaths.getNumRows() - 1;
            list->selectRow(row);
            searchPaths.selectedRowsChanged(row);
            searchPaths.returnKeyPressed(row);
            if (auto* input = TestHelpers::findChildOfType<TextEditor>(&searchPaths)) {
                input->setText(tempDirectory.getFullPathName(), sendNotificationSync);
                static_cast<TextEditor::Listener*>(&searchPaths)->textEditorReturnKeyPressed(*input);
            }
            searchPaths.deleteKeyPressed(row);
        }

        savedLibraries.addIfNotAlreadyThere("coverage_library");
        LibraryLoadPanel libraries;
        libraries.setBounds(0, 0, 760, 320);
        libraries.externalChange();
        libraries.resized();
        libraries.getContentXAndWidth();
        libraries.createComponentSnapshot(libraries.getLocalBounds());
        if (auto* list = TestHelpers::findChildOfType<ListBox>(&libraries); list && libraries.getNumRows() > 0) {
            auto const row = libraries.getNumRows() - 1;
            list->selectRow(row);
            libraries.selectedRowsChanged(row);
            libraries.returnKeyPressed(row);
            if (auto* input = TestHelpers::findChildOfType<TextEditor>(&libraries)) {
                input->setText("coverage_library_edited", sendNotificationSync);
                static_cast<TextEditor::Listener*>(&libraries)->textEditorReturnKeyPressed(*input);
            }
            libraries.deleteKeyPressed(row);
        }

        int actionClicks = 0;
        ActionButton action(Icons::Add, "Coverage action", true);
        action.onClick = [&actionClicks] { ++actionClicks; };
        action.setBounds(0, 0, 300, 32);
        auto const centre = action.getLocalBounds().getCentre().toFloat();
        action.mouseEnter(mouseEvent(&action, centre, ModifierKeys()));
        action.createComponentSnapshot(action.getLocalBounds());
        action.mouseUp(mouseEvent(&action, centre));
        action.mouseExit(mouseEvent(&action, centre, ModifierKeys()));
        expectEquals(actionClicks, 1, "path action button must invoke its callback");

        EnableGemToggle gemToggle;
        gemToggle.setBounds(0, 0, 500, 78);
        static_cast<Component*>(&gemToggle)->resized();
        gemToggle.createComponentSnapshot(gemToggle.getLocalBounds());

        PathsSettingsPanel panel;
        panel.setBounds(0, 0, 760, 560);
        static_cast<Component*>(&panel)->resized();
        panel.createComponentSnapshot(panel.getLocalBounds());

        savedPaths.clear();
        savedPaths.addArray(originalPaths);
        savedLibraries.clear();
        savedLibraries.addArray(originalLibraries);
        tempDirectory.deleteRecursively();
    }

    void exerciseCoverageGaps()
    {
        beginTest("High-yield uncovered public paths");

        exerciseLuaExpressions();
        exerciseDropzone();
        exerciseAboutAndSettings();
        exerciseAudioAndToolchainControls();
        exerciseMidiDeviceManager();
        exerciseObjectOrdering();

        auto* settings = SettingsFile::getInstance();
        ValueTree legacyTheme("Theme");
        legacyTheme.setProperty("theme", "Coverage", nullptr);
        legacyTheme.setProperty("connection_style", 2, nullptr);
        legacyTheme.setProperty("straight_connections", 1, nullptr);
        legacyTheme.setProperty("canvas_background", "ff102030", nullptr);
        auto convertedTheme = SettingsFile::xmlThemeToJson(legacyTheme);
        expect(convertedTheme != nullptr, "legacy theme conversion must produce an object");

        auto const originalBrowserPath = settings->getLastBrowserPathForId("Coverage");
        auto const tempDirectory = File::getSpecialLocation(File::tempDirectory);
        settings->setLastBrowserPathForId("Coverage", tempDirectory);
        expectEquals(settings->getLastBrowserPathForId("Coverage").getFullPathName(),
            tempDirectory.getFullPathName(), "browser path must round-trip");
        settings->setLastBrowserPathForId("Coverage", originalBrowserPath);
        settings->hasProperty("theme");
        settings->wantsNativeDialog();
        settings->getSettingsState();
        settings->resetSettingsState();
    }

    void exerciseLuaExpressions()
    {
        CommandInput commandInput(editor);
        commandInput.setBounds(0, 0, 420, 80);
        commandInput.handleURL("ls");
        commandInput.parseExpressions("value { 1 + 2 }");
        commandInput.parseExpressions("{ pd.post('coverage') }");
        commandInput.parseExpressions("{ pd.post(7) }");
        commandInput.parseExpressions("{ pd.eval('ls') }");
        commandInput.parseExpressions("{ true }");
        commandInput.parseExpressions("{ 'coverage' }");
        commandInput.parseExpressions("{ {} }");
        commandInput.parseExpressions("{ error('coverage') }");
        commandInput.parseExpressions("unmatched { 1 + 2");
        commandInput.executeCommand(editor->pd, "osc~* >");
        commandInput.executeCommand(editor->pd, "osc~_1 > frequency");
        commandInput.executeCommand(editor->pd, "osc~_1 > list 1 coverage");

        auto const scriptDirectory = File::getSpecialLocation(File::tempDirectory)
                                         .getChildFile("plugdata-command-coverage");
        scriptDirectory.createDirectory();
        auto const validScript = scriptDirectory.getChildFile("valid.lua");
        auto const invalidScript = scriptDirectory.getChildFile("invalid.lua");
        validScript.replaceWithText("coverage_script_value = 42");
        invalidScript.replaceWithText("this is not valid lua");

        LuaExpressionParser parser(editor->pd);
        parser.setCommandProcessor(&commandInput);
        parser.executeExpression("1 + 2", true);
        parser.executeExpression("true", true);
        parser.executeExpression("'coverage'", true);
        parser.executeExpression("{}", true);
        parser.executeExpression("pd.post('coverage')", false);
        parser.executeExpression("pd.post(7)", false);
        parser.executeExpression("pd.eval('ls')", true);
        parser.executeExpression("error('coverage')", false);
        parser.executeScript(validScript.getFullPathName());
        parser.executeScript(invalidScript.getFullPathName());
        parser.executeScript(scriptDirectory.getChildFile("missing.lua").getFullPathName());
        scriptDirectory.deleteRecursively();
    }

    void exerciseDropzone()
    {
        for (auto* object : cnv->objects) {
            if (!object->gui || !object->gui->getText().startsWith("dropzone"))
                continue;

            DropzoneObject dropzone(pd::WeakReference(object->getPointer(), editor->pd), object);
            dropzone.setBounds(dropzone.getPdBounds());
            dropzone.update();
            dropzone.updateSizeProperty();
            dropzone.getParameters();
            dropzone.isLocked();

            StringArray files { "/tmp/coverage-one.txt", "/tmp/coverage-two.txt" };
            dropzone.isInterestedInFileDrag(files);
            dropzone.fileDragEnter(files, 2, 3);
            dropzone.fileDragMove(files, 4, 5);
            if (auto* context = editor->getNanoLLGC(); context && context->getContext())
                dropzone.render(context->getContext());
            dropzone.fileDragExit(files);
            dropzone.filesDropped(files, 6, 7);
            dropzone.isInterestedInTextDrag("coverage text");
            dropzone.textDragEnter("coverage text", 2, 3);
            dropzone.textDragMove("coverage text", 4, 5);
            dropzone.textDragExit("coverage text");
            dropzone.textDropped("coverage text", 6, 7);
            break;
        }
    }

    void exerciseAboutAndSettings()
    {
        AboutPanel about;
        about.setBounds(0, 0, 420, 620);
        about.resized();
        about.createComponentSnapshot(about.getLocalBounds());
        if (auto* credits = TestHelpers::findButtonWithText(&about, "Credits")) {
            credits->onClick();
            HeapArray<Component::SafePointer<Component>> components;
            collectComponents(&about, components);
            for (auto& safe : components) {
                if (auto* component = safe.getComponent(); component && component->isVisible()
                    && component->getWidth() > 100 && component->getHeight() > 100)
                    component->createComponentSnapshot(component->getLocalBounds());
            }
        }
        if (auto* license = TestHelpers::findButtonWithText(&about, "License")) {
            license->onClick();
            HeapArray<Component::SafePointer<Component>> components;
            collectComponents(&about, components);
            for (auto& safe : components) {
                if (auto* component = safe.getComponent(); component && component->isVisible()
                    && component->getWidth() > 100 && component->getHeight() > 100)
                    component->createComponentSnapshot(component->getLocalBounds());
            }
        }

        DAWAudioSettingsPanel dawAudio(editor->pd);
        dawAudio.setBounds(0, 0, 520, 240);
        dawAudio.resized();
        dawAudio.getPropertiesPanel();
        dawAudio.createComponentSnapshot(dawAudio.getLocalBounds());

        auto* mappings = editor->commandManager.getKeyMappings();
        auto originalMappings = mappings->createXml(true);
        keyMappingSettings = std::make_unique<KeyMappingSettingsPanel>(mappings);
        keyMappingSettings->setBounds(0, 0, 700, 520);
        keyMappingSettings->resized();
        keyMappingSettings->getCommandManager();
        KeyMappingSettingsPanel::getDescriptionForKeyPress(KeyPress('k', ModifierKeys::commandModifier, 'k'));
        KeyMappingSettingsPanel::resetKeyMappingsToPdCallback(1, keyMappingSettings.get());
        KeyMappingSettingsPanel::resetKeyMappingsToPdCallback(0, nullptr);
        KeyMappingSettingsPanel::resetKeyMappingsToMaxCallback(1, keyMappingSettings.get());
        KeyMappingSettingsPanel::resetKeyMappingsToMaxCallback(0, nullptr);
        KeyMappingSettingsPanel::resetKeyMappingsToMaxCallback(0, keyMappingSettings.get());
        mappings->restoreFromXml(*originalMappings);
        mappings->sendSynchronousChangeMessage();

        HeapArray<Component::SafePointer<Component>> keyControls;
        collectComponents(keyMappingSettings->getPropertiesPanel(), keyControls);
        for (auto& safe : keyControls) {
            auto* button = dynamic_cast<Button*>(safe.getComponent());
            if (!button || button->getTooltip() != "Adds a new key-mapping")
                continue;
            button->triggerClick();
            if (auto* alert = dynamic_cast<AlertWindow*>(Component::getCurrentlyModalComponent())) {
                static_cast<Component*>(alert)->keyPressed(KeyPress('j', ModifierKeys::commandModifier, 'j'));
                alert->keyStateChanged(true);
                alert->exitModalState(0);
            }
            break;
        }
    }

    void exerciseAudioAndToolchainControls()
    {
        int comboChanges = 0;
        CallbackComboProperty combo("Coverage device", { "First", "Second" }, "First",
            [&comboChanges](String) { ++comboChanges; });
        combo.setBounds(0, 0, 320, 32);
        combo.resized();
        combo.comboBox.setSelectedId(2, sendNotificationSync);
        std::unique_ptr<PropertiesPanelProperty> comboCopy(combo.createCopy());
        expectEquals(comboChanges, 1);

        int channelChanges = 0;
        ChannelToggleProperty channel("Input 1", false, [&channelChanges](bool) {
            ++channelChanges;
        });
        channel.setBounds(0, 0, 320, 32);
        Value channelValue(var(true));
        channel.valueChanged(channelValue);
        channel.setEnabled(false);
        channel.createComponentSnapshot(channel.getLocalBounds());
        expectEquals(channelChanges, 1);

        DeviceManagerLevelMeter meter(ProjectInfo::getDeviceManager()->getOutputLevelGetter());
        meter.setBounds(0, 0, 200, 12);
        meter.level = 1.0f;
        meter.timerCallback();
        meter.createComponentSnapshot(meter.getLocalBounds());

        std::unique_ptr<Dialog> dialog;
        dialog = std::make_unique<Dialog>(&dialog, editor, 520, 380, true);
        ToolchainInstaller installer(editor, dialog.get());
        installer.setBounds(0, 0, 520, 380);
        installer.resized();
        installer.needsUpdate = false;
        installer.createComponentSnapshot(installer.getLocalBounds());
        installer.needsUpdate = true;
        installer.installProgress = 0.5f;
        installer.errorMessage = "Coverage error";
        installer.startTimer(1000);
        installer.createComponentSnapshot(installer.getLocalBounds());
        installer.stopTimer();
        installer.installButton.mouseEnter(mouseEvent(&installer.installButton, { 10.0f, 10.0f }));
        installer.installButton.mouseExit(mouseEvent(&installer.installButton, { 10.0f, 10.0f }));
        installer.installButton.mouseUp(mouseEvent(&installer.installButton, { 10.0f, 10.0f }, ModifierKeys::rightButtonModifier));
        installer.run();
    }

    void exerciseMidiDeviceManager()
    {
        auto& manager = editor->pd->getMidiDeviceManager();
        manager.prepareToPlay(48000.0f);
        manager.updateMidiDevices();

        auto inputs = manager.getInputDevices();
        auto outputs = manager.getOutputDevices();
        for (auto& input : inputs)
            manager.getMidiDevicePort(true, input);
        for (auto& output : outputs)
            manager.getMidiDevicePort(false, output);

        String virtualInput;
        String virtualOutput;
        for (auto const& input : inputs) {
            if (input.name.containsIgnoreCase("plugdata")) {
                virtualInput = input.identifier;
                break;
            }
        }
        for (auto const& output : outputs) {
            if (output.name.containsIgnoreCase("plugdata")) {
                virtualOutput = output.identifier;
                break;
            }
        }

        if (virtualInput.isNotEmpty())
            manager.setMidiDevicePort(true, virtualInput, 0);
        if (virtualOutput.isNotEmpty())
            manager.setMidiDevicePort(false, virtualOutput, 0);

        manager.getPortDescription(true, 0);
        manager.getPortDescription(false, 0);

        MidiBuffer input;
        input.addEvent(MidiMessage::noteOn(1, 60, static_cast<uint8_t>(100)), 0);
        manager.enqueueMidiInput(0, input);
        int inputCallbacks = 0;
        manager.dequeueMidiInput(64, [&inputCallbacks](int, MidiBuffer const&) {
            ++inputCallbacks;
        });

        manager.setInternalSynthPort(0);
        manager.enqueueMidiOutput(0, MidiMessage::noteOn(1, 64, static_cast<uint8_t>(90)), 4);
        manager.enqueueMidiOutput(0, MidiMessage::noteOff(1, 64), 24);
        MidiBuffer dawOutput;
        manager.sendAndCollectMidiOutput(dawOutput);
        MidiBuffer synthOutput;
        manager.dequeueMidiOutput(0, synthOutput, 64);
        manager.getInputHistory();
        manager.getOutputHistory();
        manager.clearMidiOutputBuffers(64);
        ignoreUnused(inputCallbacks);

        if (virtualInput.isNotEmpty())
            manager.setMidiDevicePort(true, virtualInput, -1);
        if (virtualOutput.isNotEmpty())
            manager.setMidiDevicePort(false, virtualOutput, -1);
        manager.cancelPendingUpdate();
    }

    void exerciseObjectOrdering()
    {
        if (cnv->objects.size() < 3)
            return;

        auto* object = cnv->objects[1];
        if (!object->gui)
            return;

        object->gui->moveToFront();
        object->gui->moveBackward();
        object->gui->moveForward();
        object->gui->moveToBack();
        cnv->performSynchronise();
    }

    void exerciseDialogsAndMenus()
    {
        beginTest("Object browser, reference and application menus");

        {
            ObjectBrowserDialog browser(editor);
            browser.setBounds(0, 0, 750, 480);
            browser.resized();
            exerciseControlTree(&browser);
            if (auto* search = TestHelpers::findChildOfType<TextEditor>(&browser)) {
                search->setText("osc", sendNotificationSync);
                search->keyPressed(KeyPress(KeyPress::downKey));
                search->keyPressed(KeyPress(KeyPress::returnKey));
                search->setText("", sendNotificationSync);
            }
            browser.createComponentSnapshot(browser.getLocalBounds());
            browser.dismiss(false);
        }

        Dialogs::showObjectBrowserDialog(&editor->openedDialog, editor);
        exerciseControlTree(editor->openedDialog.get());
        editor->openedDialog.reset(nullptr);

        Dialogs::showObjectReferenceDialog(&editor->openedDialog, editor, "osc~");
        exerciseControlTree(editor->openedDialog.get());
        editor->openedDialog.reset(nullptr);

        int dialogChoice = -1;
        Dialogs::showMultiChoiceDialog(&editor->openedDialog, editor, "Coverage choice",
            [&dialogChoice](int choice) { dialogChoice = choice; }, { "First", "Second", "Third" });
        if (auto* dialog = editor->openedDialog.get()) {
            exerciseControlTree(dialog);
            HeapArray<Component::SafePointer<Component>> controls;
            collectComponents(dialog, controls);
            for (auto& safe : controls) {
                if (auto* button = dynamic_cast<TextButton*>(safe.getComponent())) {
                    button->triggerClick();
                    break;
                }
            }
        }
        ignoreUnused(dialogChoice);
        editor->openedDialog.reset(nullptr);

        {
            AboutPanel about;
            about.setBounds(0, 0, 360, 490);
            exerciseControlTree(&about);
        }

        auto* settings = SettingsFile::getInstance();
        auto const wasTouch = settings->getProperty<bool>("touch_mode");
        settings->setProperty("touch_mode", true);
        Dialogs::showMainMenu(editor, editor);
        if (auto* modal = Component::getCurrentlyModalComponent())
            exerciseControlTree(modal);
        ModalComponentManager::getInstance()->cancelAllModalComponents();
        settings->setProperty("touch_mode", wasTouch);
    }

    class ModifierProbe final : public ModifierKeyListener {
    public:
        void shiftKeyChanged(bool) override { ++callbacks; }
        void commandKeyChanged(bool) override { ++callbacks; }
        void altKeyChanged(bool) override { ++callbacks; }
        void ctrlKeyChanged(bool) override { ++callbacks; }
        void spaceKeyChanged(bool) override { ++callbacks; }
        void middleMouseChanged(bool) override { ++callbacks; }
        int callbacks = 0;
    };

    void exerciseUtilitiesAndChrome()
    {
        beginTest("Audio FIFO, recorder, modifiers, viewport and status bar");

        AudioMidiFifo fifo(2, 8);
        AudioBuffer<float> first(2, 6);
        for (int channel = 0; channel < first.getNumChannels(); ++channel)
            for (int sample = 0; sample < first.getNumSamples(); ++sample)
                first.setSample(channel, sample, static_cast<float>(channel * 10 + sample));
        MidiBuffer firstMidi;
        firstMidi.addEvent(MidiMessage::noteOn(1, 60, static_cast<uint8>(100)), 2);
        dsp::AudioBlock<float> firstBlock(first);
        fifo.writeAudioAndMidi(firstBlock, firstMidi);

        AudioBuffer<float> firstRead(2, 4);
        MidiBuffer firstReadMidi;
        dsp::AudioBlock<float> firstReadBlock(firstRead);
        fifo.readAudioAndMidi(firstReadBlock, firstReadMidi);

        AudioBuffer<float> wrapped(2, 6);
        wrapped.clear();
        MidiBuffer wrappedMidi;
        wrappedMidi.addEvent(MidiMessage::controllerEvent(1, 7, 100), 4);
        fifo.writeAudioAndMidi(wrapped, wrappedMidi);
        AudioBuffer<float> wrappedRead(2, 5);
        MidiBuffer wrappedReadMidi;
        fifo.readAudioAndMidi(wrappedRead, wrappedReadMidi);
        fifo.writeSilence(5);
        AudioBuffer<float> finalRead(2, 8);
        MidiBuffer finalMidi;
        fifo.readAudioAndMidi(finalRead, finalMidi);
        expectEquals(fifo.getNumSamplesAvailable(), 0, "FIFO must drain after wraparound reads");
        fifo.setSize(1, 4);
        fifo.clear();

        {
            Recorder recorder;
            recorder.prepare(44100.0, 2);
            expectWithinAbsoluteError(recorder.getElapsedSeconds(), 0.0, 0.0001);
            recorder.toggleRecording(nullptr);
            AudioBuffer<float> recorded(2, 128);
            recorded.clear();
            recorder.write(recorded);
            expect(recorder.isRecording(), "recorder must start with a null editor");
            expect(recorder.getElapsedSeconds() > 0.0, "recorder must count written samples");
        }

        ModifierProbe modifierProbe;
        ModifierKeyBroadcaster modifierBroadcaster;
        modifierBroadcaster.addModifierKeyListener(&modifierProbe);
        modifierBroadcaster.setModifierKeys(ModifierKeys(
            ModifierKeys::shiftModifier | ModifierKeys::commandModifier | ModifierKeys::altModifier
            | ModifierKeys::ctrlModifier | ModifierKeys::middleButtonModifier));
        modifierBroadcaster.setModifierKeys(ModifierKeys());
        modifierBroadcaster.removeModifierKeyListener(&modifierProbe);
        expect(modifierProbe.callbacks >= 10, "modifier transitions must reach listeners");

        Component viewed;
        viewed.setSize(300, 1200);
        BouncingViewport viewport;
        viewport.setBounds(0, 0, 300, 240);
        viewport.setViewedComponent(&viewed, false);
        viewport.setScrollBarsShown(true, false);
        viewport.setBounce(true);
        BouncingViewportAttachment bounce(&viewport);
        bounce.settingsChanged("unrelated", {});
        bounce.settingsChanged("touch_mode", true);
        bounce.settingsChanged("touch_mode", false);
        bounce.doesMouseEventComponentBlockViewportDrag(&viewed);
        viewed.setViewportIgnoreDragFlag(true);
        bounce.doesMouseEventComponentBlockViewportDrag(&viewed);
        viewed.setViewportIgnoreDragFlag(false);
        bounce.setBounce(false);
        MouseWheelDetails wheel;
        wheel.deltaY = -0.5f;
        wheel.isSmooth = true;
        bounce.mouseWheelMove(mouseEvent(&viewed, { 50.0f, 50.0f }, ModifierKeys()), wheel);
        bounce.setBounce(true);
        bounce.mouseWheelMove(mouseEvent(&viewed, { 50.0f, 50.0f }, ModifierKeys()), wheel);
        wheel.isInertial = true;
        wheel.deltaY = -0.01f;
        bounce.mouseWheelMove(mouseEvent(&viewed, { 50.0f, 50.0f }, ModifierKeys()), wheel);
        auto dragStart = mouseEvent(&viewed, { 50.0f, 50.0f });
        bounce.mouseDrag(dragStart);
        bounce.mouseDrag(mouseEvent(&viewed, { 50.0f, 150.0f }));
        bounce.mouseUp(mouseEvent(&viewed, { 50.0f, 150.0f }));

        if (auto* statusbar = TestHelpers::findChildOfType<Statusbar>(editor)) {
            statusbar->setBounds(0, 0, 420, Statusbar::getStatusbarHeight());
            statusbar->updateZoomLevel();
            statusbar->setEditButtonState(false);
            statusbar->setEditButtonState(true);
            statusbar->setEditButtonState(true, true);
            statusbar->createComponentSnapshot(statusbar->getLocalBounds());

            HeapArray<Component::SafePointer<Component>> controls;
            collectComponents(statusbar, controls);
            auto findButtonWithTooltip = [&controls](String const& tooltip) -> Button* {
                for (auto& safe : controls) {
                    if (auto* button = dynamic_cast<Button*>(safe.getComponent());
                        button && button->getTooltip() == tooltip)
                        return button;
                }
                return nullptr;
            };

            if (auto* zoomMenu = findButtonWithTooltip("Zoom options")) {
                if (auto* zoomLabel = zoomMenu->getParentComponent()) {
                    MouseWheelDetails zoomIn;
                    zoomIn.deltaY = 0.25f;
                    zoomLabel->mouseWheelMove(mouseEvent(zoomLabel, zoomLabel->getLocalBounds().getCentre().toFloat()), zoomIn);
                    zoomIn.deltaY = -0.25f;
                    zoomLabel->mouseWheelMove(mouseEvent(zoomLabel, zoomLabel->getLocalBounds().getCentre().toFloat()), zoomIn);
                    zoomLabel->mouseDown(mouseEvent(zoomLabel, zoomLabel->getLocalBounds().getCentre().toFloat()));
                    zoomLabel->setEnabled(false);
                    zoomLabel->createComponentSnapshot(zoomLabel->getLocalBounds());
                    zoomLabel->setEnabled(true);
                }
                if (zoomMenu->onClick)
                    zoomMenu->onClick();
                PopupMenu::dismissAllActiveMenus();
            }

            for (auto const& tooltip : {
                     "Toggle grid", "Toggle overlay alt-mode", "Toggle edit/run mode" }) {
                if (auto* button = findButtonWithTooltip(tooltip); button && button->onClick)
                    button->onClick();
            }

            if (auto* modeButton = findButtonWithTooltip("Other canvas modes");
                modeButton && modeButton->onClick) {
                modeButton->onClick();

                HeapArray<Component::SafePointer<Component>> calloutControls;
                collectComponents(editor->getCalloutAreaComponent(), calloutControls);
                for (auto& safe : calloutControls) {
                    auto* mode = dynamic_cast<CalloutMenuButton*>(safe.getComponent());
                    if (!mode)
                        continue;

                    mode->getParentComponent()->createComponentSnapshot(mode->getParentComponent()->getLocalBounds());
                    if (mode->description == "Presentation mode" && mode->onClick) {
                        mode->onClick();
                        break;
                    }
                }
            }

            cnv->presentationMode = false;
            cnv->locked = false;
            ModalComponentManager::getInstance()->cancelAllModalComponents();
            PopupMenu::dismissAllActiveMenus();

            for (auto& safe : controls) {
                auto* button = dynamic_cast<Button*>(safe.getComponent());
                if (!button || !button->isEnabled())
                    continue;
                button->createComponentSnapshot(button->getLocalBounds());
            }
            editor->openedDialog.reset(nullptr);
        }
    }

    void exerciseExporters()
    {
        beginTest("Exporter configurations and progress console");

        beginTest("Exporter progress view");
        exporterView = std::make_unique<ExportingProgressView>();
        editor->addAndMakeVisible(exporterView.get());
        exporterView->setBounds(0, 0, 700, 500);

        for (auto const state : {
                 ExportingProgressView::Exporting,
                 ExportingProgressView::Flashing,
                 ExportingProgressView::Success,
                 ExportingProgressView::Failure,
                 ExportingProgressView::BootloaderFlashSuccess,
                 ExportingProgressView::BootloaderFlashFailure,
                 ExportingProgressView::NotExporting }) {
            exporterView->showState(state);
            exporterView->timerCallback();
            exporterView->createComponentSnapshot(exporterView->getLocalBounds());
        }

        if (auto* viewport = TestHelpers::findChildOfType<Viewport>(exporterView.get())) {
            if (auto* console = dynamic_cast<ExporterConsole*>(viewport->getViewedComponent())) {
                console->setSize(620, 240);
                console->append("plain text and a verylongwordthatmustwrapacrossaline\n");
                console->append("\x1b[1;31mred\x1b[0m \x1b[32mgreen\x1b[94m blue");
                console->append("\x1b[");
                console->append("0m reset\rreplacement\nsecond line");
                console->createComponentSnapshot(console->getLocalBounds());

                auto const start = mouseEvent(console, { 4.0f, 4.0f });
                console->mouseDown(start);
                console->mouseDrag(mouseEvent(console, { 180.0f, 18.0f }));
                console->mouseDoubleClick(mouseEvent(console, { 40.0f, 4.0f }, ModifierKeys::leftButtonModifier, 2));
                console->keyPressed(KeyPress('a', ModifierKeys::commandModifier, 0));
                console->keyPressed(KeyPress('c', ModifierKeys::commandModifier, 0));
                console->mouseDown(mouseEvent(console, { 40.0f, 4.0f }, ModifierKeys::rightButtonModifier));
                console->clear();
            }
        }

        exporterView->allConsoleOutput = "coverage exporter output";
        expect(exporterView->hasConsoleMessage({ "missing", "exporter output" }),
            "export progress output must be searchable");
        exporterView->continueButton.triggerClick();

        auto const outputDirectory = File::getSpecialLocation(File::tempDirectory)
                                         .getChildFile("plugdata_exporter_coverage");
        outputDirectory.deleteRecursively();
        outputDirectory.createDirectory();
        auto const customBoard = outputDirectory.getChildFile("custom-board.json");
        auto const customLinker = outputDirectory.getChildFile("custom-linker.lds");
        customBoard.replaceWithText("{}");
        customLinker.replaceWithText("MEMORY {}");
        StringArray const searchPaths { outputDirectory.getFullPathName().quoted() };

        auto exerciseCommon = [this](ExporterBase& exporter) {
            exporter.setBounds(0, 0, 560, 480);
            exporter.blockDialog = true;
            exporter.inputPatchValue = 1;
            exporter.inputPatchValue = 2;
            exporter.inputPatchValue = 1;
            exporter.projectNameValue = "coverage-project";
            exporter.projectCopyrightValue = "coverage copyright";
            exerciseControlTree(&exporter, false);

            DynamicObject::Ptr emptyState(new DynamicObject());
            exporter.setState(emptyState);
            DynamicObject::Ptr state(new DynamicObject());
            exporter.getState(state);
            exporter.setState(state);
            exporter.createComponentSnapshot(exporter.getLocalBounds());
        };

        auto const originalHeavyExecutable = ExporterBase::heavyExecutable;
        ExporterBase::heavyExecutable = File("/usr/bin/true");

        {
            beginTest("C++ exporter");
            CppExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.shouldQuit = true;
            expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                       "coverage", "copyright", searchPaths),
                "cancelled C++ export must stop before generation");
        }

        {
            beginTest("Daisy exporter");
            DaisyExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.customBoardDefinition = customBoard;
            exporter.customLinker = customLinker;

            for (int exportType = 1; exportType <= 4; ++exportType)
                exporter.exportTypeValue = exportType;
            for (int board = 1; board <= 10; ++board)
                exporter.targetBoardValue = board;
            for (int size = 1; size <= 6; ++size)
                exporter.patchSizeValue = size;
            for (int appType = 1; appType <= 3; ++appType)
                exporter.appTypeValue = appType;
            for (int rate = 1; rate <= 5; ++rate)
                exporter.samplerateValue = rate;
            exporter.usbMidiValue = 1;
            exporter.debugPrintValue = 1;
            exporter.blocksizeValue = 64;
            exporter.resized();

            struct DaisyCase {
                int board;
                int size;
                int appType;
                int usbMidi;
                int debugPrint;
                int blockSize;
                int sampleRate;
            };
            for (auto const& test : {
                     DaisyCase { 1, 1, 1, 0, 0, 48, 4 },
                     DaisyCase { 7, 2, 1, 1, 0, 64, 1 },
                     DaisyCase { 8, 3, 1, 0, 1, 48, 2 },
                     DaisyCase { 9, 4, 1, 0, 0, 48, 3 },
                     DaisyCase { 1, 5, 1, 0, 0, 48, 5 },
                     DaisyCase { 10, 6, 2, 0, 1, 64, 4 },
                     DaisyCase { 10, 6, 3, 1, 0, 64, 4 } }) {
                exporter.targetBoardValue = test.board;
                exporter.patchSizeValue = test.size;
                exporter.appTypeValue = test.appType;
                exporter.usbMidiValue = test.usbMidi;
                exporter.debugPrintValue = test.debugPrint;
                exporter.blocksizeValue = test.blockSize;
                exporter.samplerateValue = test.sampleRate;
                exporter.resized();
                exporter.exportTypeValue = 1;
                exporter.shouldQuit = true;
                expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                           "coverage", "copyright", searchPaths),
                    "cancelled Daisy export must stop before copying the toolchain");
            }
        }

        {
            beginTest("DPF exporter");
            DPFExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.shouldQuit = true;

            exporter.makerNameValue = "";
            exporter.projectLicenseValue = "";
            exporter.pluginTypeValue = 1;
            exporter.exportTypeValue = 1;
            expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                       "coverage", "", searchPaths),
                "cancelled DPF export must stop before generation");

            exporter.makerNameValue = "Coverage Maker";
            exporter.projectLicenseValue = "MIT";
            exporter.pluginTypeValue = 2;
            exporter.pluginTypeValue = 3;
            exporter.midiinEnableValue = 1;
            exporter.midioutEnableValue = 1;
            exporter.lv2EnableValue = 0;
            exporter.vst2EnableValue = 1;
            exporter.vst3EnableValue = 0;
            exporter.clapEnableValue = 1;
            exporter.jackEnableValue = 1;
            exporter.disableSIMD = 1;
            for (int exportType = 2; exportType <= 4; ++exportType) {
                exporter.exportTypeValue = exportType;
                expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                           "coverage", "copyright", searchPaths),
                    "cancelled DPF export must stop before generation");
            }
        }

        {
            beginTest("OWL exporter");
            OWLExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.shouldQuit = true;
            for (int target = 1; target <= 3; ++target)
                exporter.targetBoardValue = target;
            for (int exportType = 1; exportType <= 4; ++exportType) {
                exporter.exportTypeValue = exportType;
                exporter.storeSlotValue = exportType;
            }
            exporter.resized();
            expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                       "coverage", "copyright", searchPaths),
                "cancelled OWL export must stop after generation");
        }

        {
            beginTest("Pd exporter");
            PdExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.exportTypeValue = 1;
            exporter.copyToPath = 1;
            exporter.exportTypeValue = 2;
            exporter.copyToPath = 1;
            exporter.shouldQuit = true;
            expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                       "coverage", "copyright", searchPaths),
                "cancelled Pd export must stop before generation");
        }

        {
            beginTest("WASM exporter");
            WASMExporter exporter(editor, exporterView.get());
            exerciseCommon(exporter);
            exporter.emsdkPathValue = "";
            exporter.emsdkPathValue = outputDirectory.getFullPathName();
            exporter.shouldQuit = true;
            expect(exporter.performExport("coverage.pd", outputDirectory.getFullPathName(),
                       "coverage", "copyright", searchPaths),
                "cancelled WASM export must stop before generation");
        }

        ExporterBase::heavyExecutable = originalHeavyExecutable;
        ExporterBase::deleteTempFiles();
        outputDirectory.deleteRecursively();
        exporterView->showState(ExportingProgressView::NotExporting);
        editor->removeChildComponent(exporterView.get());
    }

    struct ParameterState {
        PlugDataParameter* parameter = nullptr;
        String name;
        NormalisableRange<float> range;
        float value = 0.0f;
        float defaultValue = 0.0f;
        int index = 0;
        bool enabled = false;
    };

    static PlugDataParameter::Mode modeForRange(NormalisableRange<float> const& range)
    {
        if (range.interval == 1.0f)
            return PlugDataParameter::Integer;
        if (range.skew == 4.0f)
            return PlugDataParameter::Logarithmic;
        if (range.skew == 0.25f)
            return PlugDataParameter::Exponential;
        return PlugDataParameter::Float;
    }

    void exerciseAutomation()
    {
        SmallArray<ParameterState> states;
        auto parameters = editor->pd->getParameters();
        for (int i = 1; i < jmin(5, parameters.size()); ++i) {
            auto* parameter = dynamic_cast<PlugDataParameter*>(parameters[i]);
            states.add({ parameter, parameter->getTitle().toString(), parameter->getNormalisableRange(),
                parameter->getValue(), parameter->getDefaultValue(), parameter->getIndex(), parameter->isEnabled() });
            parameter->setEnabled(true);
            parameter->setName("coverage_param_" + String(i));
            parameter->setIndex(i - 1);
            parameter->setRange(-10.0f * i, 10.0f * i);
            parameter->setMode(static_cast<PlugDataParameter::Mode>((i - 1) % 4 + 1), false);
            parameter->setUnscaledValueNotifyingHost(static_cast<float>(i));
        }

        editor->pd->updateEnabledParameters();
        {
            AutomationPanel automation(editor->pd);
            automation.setBounds(0, 0, 360, 620);
            automation.resized();
            automation.sliders.updateSliders();
            automation.sliders.getNewParameterName();
            automation.sliders.getParameters();
            automation.sliders.checkMaxNumParameters();
            for (auto* row : automation.sliders.rows) {
                row->setBounds(0, 0, 340, 120);
                row->update();
                row->slider.setValue(row->slider.getMinimum(), sendNotificationSync);
                row->slider.setValue(row->slider.getMaximum(), sendNotificationSync);
                row->slider.setValue(0.0, sendNotificationSync);
                row->getItemHeight();
                row->isParameterEnabled();
                row->cancelAnimation({ 0, 0, 340, 56 });
                row->getTargetBounds();
                exerciseControlTree(row);

                HeapArray<Component::SafePointer<Component>> controls;
                collectComponents(row, controls);
                for (auto& safe : controls) {
                    auto* component = safe.getComponent();
                    if (auto* button = dynamic_cast<Button*>(component);
                        button && button->getTooltip() == "Expand settings") {
                        button->triggerClick();
                        row->resized();
                    }
                }

                controls.clear();
                collectComponents(row, controls);
                for (auto& safe : controls) {
                    auto* component = safe.getComponent();
                    if (auto* number = dynamic_cast<DraggableNumber*>(component)) {
                        auto const original = number->getValue();
                        number->setValue(original + 1.0, sendNotificationSync);
                        number->dragEnd();
                        number->setValue(original, sendNotificationSync);
                        number->dragEnd();
                    } else if (auto* label = dynamic_cast<Label*>(component);
                               label && label->getTooltip() == "Drag to add [param] to canvas") {
                        auto const original = label->getText();
                        label->showEditor();
                        if (auto* input = label->getCurrentTextEditor())
                            input->setText(original + "_renamed", sendNotificationSync);
                        label->hideEditor(false);
                    }
                }

                row->hitTest(10, 10);
                row->mouseEnter(mouseEvent(row, { 10.0f, 10.0f }));
                row->mouseDrag(dragEvent(row, { 30.0f, 30.0f }, { 10.0f, 10.0f }));
                row->mouseExit(mouseEvent(row, { 10.0f, 10.0f }));
            }

            if (automation.sliders.rows.size() > 1) {
                auto* row = automation.sliders.rows[1];
                auto& reorder = row->reorderButton;
                auto const centre = reorder.getLocalBounds().getCentre().toFloat();
                automation.sliders.mouseDown(mouseEvent(&reorder, centre));
                automation.sliders.mouseDrag(dragEvent(&reorder, centre + Point<float>(0.0f, 70.0f), centre));
                automation.sliders.mouseUp(mouseEvent(&reorder, centre));
            }

            if (automation.sliders.rows.size() > 1)
                automation.sliders.rows.getLast()->deleteButton.triggerClick();

            auto& addButton = automation.sliders.addParameterButton;
            auto const addCentre = addButton.getLocalBounds().getCentre().toFloat();
            addButton.hitTest(addCentre.x, addCentre.y);
            addButton.mouseEnter(mouseEvent(&addButton, addCentre));
            addButton.mouseUp(mouseEvent(&addButton, addCentre));
            addButton.mouseExit(mouseEvent(&addButton, addCentre));

            automation.updateParameterValue(states.front().parameter);
            automation.scrollBarMoved(nullptr, 10.0);
            automation.handleAsyncUpdate();
            automation.createComponentSnapshot(automation.getLocalBounds());
        }

        for (auto const& state : states) {
            state.parameter->setEnabled(state.enabled);
            state.parameter->setName(state.name);
            state.parameter->setIndex(state.index);
            state.parameter->setRange(state.range.start, state.range.end);
            state.parameter->setMode(modeForRange(state.range), false);
            state.parameter->setDefaultValue(state.defaultValue);
            state.parameter->setValue(state.value);
        }
        editor->pd->updateEnabledParameters();
    }

    class ToolbarProbe final : public ToolbarSource::Listener {
    public:
        void midiReceivedChanged(bool) override { ++callbacks; }
        void midiSentChanged(bool) override { ++callbacks; }
        void midiMessageReceived(MidiMessage const&) override { ++callbacks; }
        void midiMessageSent(MidiMessage const&) override { ++callbacks; }
        void audioProcessedChanged(bool) override { ++callbacks; }
        void audioLevelChanged(SmallArray<float>) override { ++callbacks; }
        void cpuUsageChanged(float) override { ++callbacks; }
        void recordingTimeChanged(float) override { ++callbacks; }
        int callbacks = 0;
    };

    void exerciseProcessor()
    {
        auto* pd = editor->pd;
        PluginProcessor::buildBusesProperties();
        pd->getName();
        pd->hasEditor();
        pd->acceptsMidi();
        pd->producesMidi();
        pd->isMidiEffect();
        pd->getTailLengthSeconds();
        pd->getNumPrograms();
        pd->getCurrentProgram();
        pd->getProgramName(0);
        pd->getProgramName(-1);
        pd->changeProgramName(0, "coverage");
        pd->setCurrentProgram(-1);
        pd->getMidiDeviceManager();
        pd->getEnabledParameters();
        pd->settingsChanged("unrelated", {});
        pd->settingsFileReloaded();
        pd->findLostPatch("${PATCHES_DIR}/coverage-missing.pd");

#ifndef JucePlugin_PreferredChannelConfigurations
        AudioProcessor::BusesLayout supported;
        supported.inputBuses.add(AudioChannelSet::stereo());
        supported.outputBuses.add(AudioChannelSet::stereo());
        pd->isBusesLayoutSupported(supported);
        AudioProcessor::BusesLayout unsupported;
        unsupported.inputBuses.add(AudioChannelSet::mono());
        unsupported.outputBuses.add(AudioChannelSet::stereo());
        pd->isBusesLayoutSupported(unsupported);
#endif

        MidiBuffer output;
        output.addEvent(MidiMessage::noteOn(1, 60, static_cast<uint8>(100)), 0);
        output.addEvent(MidiMessage::noteOff(1, 60), 1);
        output.addEvent(MidiMessage::controllerEvent(1, 7, 100), 2);
        output.addEvent(MidiMessage::pitchWheel(1, 9000), 3);
        output.addEvent(MidiMessage::channelPressureChange(1, 80), 4);
        output.addEvent(MidiMessage::aftertouchChange(1, 60, 80), 5);
        output.addEvent(MidiMessage::programChange(1, 3), 6);
        uint8 const sysex[] { 0x01, 0x02, 0x03 };
        output.addEvent(MidiMessage::createSysExMessage(sysex, 3), 7);
        output.addEvent(MidiMessage::midiClock(), 8);
        output.addEvent(MidiMessage::midiStart(), 9);
        output.addEvent(MidiMessage::midiContinue(), 10);
        output.addEvent(MidiMessage::midiStop(), 11);
        uint8 const activeSense = 0xfe;
        output.addEvent(MidiMessage(&activeSense, 1, 0.0), 12);
        pd->sendMidiBuffer(0, output);

        ToolbarProbe probe;
        pd->toolbarSource->addListener(&probe);
        pd->toolbarSource->setSampleRate(44100.0);
        pd->toolbarSource->setBufferSize(96);
        pd->toolbarSource->prepareToPlay(2);
        AudioBuffer<float> peaks(2, 128);
        for (int i = 0; i < peaks.getNumSamples(); ++i) {
            peaks.setSample(0, i, i / 128.0f);
            peaks.setSample(1, i, 1.0f - i / 128.0f);
        }
        pd->toolbarSource->peakBuffer.write(peaks);
        pd->toolbarSource->peakBuffer.getPeak();
        pd->toolbarSource->setCPUUsage(42.0f);
        pd->toolbarSource->setRecorderTime(65.0f);
        pd->toolbarSource->process(output, output);
        pd->toolbarSource->timerCallback();
        pd->toolbarSource->removeListener(&probe);
        expect(probe.callbacks > 0, "toolbar source must notify its listener");

        editor->audioToolbar->showDSPState(true);
        editor->audioToolbar->showDSPState(false);
        editor->audioToolbar->showLimiterState(true);
        editor->audioToolbar->showLimiterState(false);
        editor->audioToolbar->setLatencyDisplay(128);
        editor->audioToolbar->setLatencyDisplay(0);
        editor->audioToolbar->recordingTimeChanged(65.0f);
        editor->audioToolbar->recordingTimeChanged(0.0f);
        editor->audioToolbar->updateOversampling();
        editor->audioToolbar->createComponentSnapshot(editor->audioToolbar->getLocalBounds());
        exerciseControlTree(editor->audioToolbar.get());
        exerciseToolbarPopups();

        auto const originalSampleRate = pd->getSampleRate();
        auto const originalBlockSize = pd->AudioProcessor::getBlockSize();
        auto const originalOversampling = SettingsFile::getInstance()->getProperty<int>("oversampling");
        auto const originalVolume = pd->volume->load();
        auto const originalLimiter = pd->getEnableLimiter();
        auto const wasSuspended = pd->isSuspended();

        pd->suspendProcessing(true);
        pd->prepareToPlay(44100.0, pd::Instance::getBlockSize());
        AudioBuffer<float> audio(2, pd::Instance::getBlockSize());
        audio.clear();
        MidiBuffer midi(output);
        pd->processBlock(audio, midi);
        pd->processBlock(audio, midi);
        pd->processBlockBypassed(audio, midi);

        pd->prepareToPlay(44100.0, 96);
        AudioBuffer<float> variableAudio(2, 96);
        variableAudio.clear();
        variableAudio.setSample(0, 0, std::numeric_limits<float>::infinity());
        variableAudio.setSample(1, 1, std::numeric_limits<float>::quiet_NaN());
        MidiBuffer variableMidi(output);
        pd->volume->store(1.0f);
        pd->setEnableLimiter(false);
        pd->processBlock(variableAudio, variableMidi);
        pd->setEnableLimiter(true);
        pd->processBlock(variableAudio, variableMidi);

        expect(pd->toggleRecording(nullptr), "processor recorder must start");
        pd->processBlock(variableAudio, variableMidi);
        expect(!pd->toggleRecording(editor), "processor recorder must stop");
        exerciseControlTree(editor->openedDialog.get());
        editor->openedDialog.reset(nullptr);

        pd->volume->store(originalVolume);
        pd->setEnableLimiter(originalLimiter);
        pd->prepareToPlay(originalSampleRate > 0.0 ? originalSampleRate : 44100.0,
            originalBlockSize > 0 ? originalBlockSize : pd::Instance::getBlockSize());
        pd->setOversampling(originalOversampling);
        pd->suspendProcessing(wasSuspended);

        MemoryBlock state;
        pd->getStateInformation(state);
        expect(state.getSize() > 0, "processor state must serialize");
    }

    void exerciseToolbarPopups()
    {
        HeapArray<Component::SafePointer<Component>> controls;
        collectComponents(editor->audioToolbar.get(), controls);
        for (auto& safe : controls) {
            auto* component = safe.getComponent();
            if (!component || !component->isVisible() || !component->isEnabled())
                continue;

            auto const centre = component->getLocalBounds().getCentre().toFloat();
            if (auto* button = dynamic_cast<Button*>(component)) {
                auto const state = button->getToggleState();
                button->triggerClick();
                button->triggerClick();
                button->setToggleState(state, sendNotificationSync);
            } else {
                component->mouseDown(mouseEvent(component, centre));
                component->mouseUp(mouseEvent(component, centre));
                if (auto* modal = Component::getCurrentlyModalComponent()) {
                    modal->setBounds(modal->getBounds());
                    exerciseControlTree(modal);
                    ModalComponentManager::getInstance()->cancelAllModalComponents();
                }
            }
        }
    }

    void exerciseValueTree()
    {
        beginTest("Value tree viewer and autosave history");

        ValueTree root("Root");
        for (int i = 0; i < 5; ++i) {
            ValueTree child("Object");
            child.setProperty("Name", i == 0 ? "trigger object" : "object " + String(i), nullptr);
            child.setProperty("ObjectName", i == 0 ? "trigger" : "osc~", nullptr);
            child.setProperty("Object", static_cast<int64>(i + 1), nullptr);
            child.setProperty("Index", i, nullptr);
            child.setProperty("RightText", String(i * 10) + ", " + String(i * 20), nullptr);
            child.setProperty("SendSymbol", "send_" + String(i), nullptr);
            child.setProperty("ReceiveSymbol", "receive_" + String(i), nullptr);
            child.setProperty("Icon", Icons::Object, nullptr);
            child.setProperty("Selected", i == 2, nullptr);

            for (int j = 0; j < 3; ++j) {
                ValueTree nested("Nested");
                nested.setProperty("Name", "nested " + String(j), nullptr);
                nested.setProperty("ObjectName", j == 0 ? "float" : "value", nullptr);
                nested.setProperty("Object", static_cast<int64>(100 + i * 10 + j), nullptr);
                child.appendChild(nested, nullptr);
            }
            root.appendChild(child, nullptr);
        }

        ValueTreeViewerComponent viewer("(Subpatch)");
        viewer.setBounds(0, 0, 500, 400);
        viewer.onReturn = [](ValueTree&) { };
        viewer.onClick = [](ValueTree&) { };
        viewer.onSelect = [](ValueTree&) { };
        viewer.onDragStart = [](ValueTree&) { };
        viewer.onRightClick = [](ValueTree&) { };
        viewer.setValueTree(root);
        viewer.setSelectedNode(reinterpret_cast<void*>(static_cast<intptr_t>(2)));
        viewer.makeNodeActive(reinterpret_cast<void*>(static_cast<intptr_t>(2)));
        viewer.getSelectedNodeObject();
        viewer.keyPressed(KeyPress(KeyPress::rightKey), &viewer);
        viewer.keyPressed(KeyPress(KeyPress::downKey), &viewer);
        viewer.keyPressed(KeyPress(KeyPress::upKey), &viewer);
        viewer.keyPressed(KeyPress(KeyPress::leftKey), &viewer);
        viewer.keyPressed(KeyPress(KeyPress::returnKey), &viewer);
        viewer.settingsChanged("search_order", true);
        viewer.settingsChanged("search_xy_show", true);
        viewer.settingsChanged("search_index_show", true);

        for (auto const& filter : { "object", "\"trigger\"", "object:osc~", "send", "receive",
                 "symbols", "trigger", "value", "int", "float", "does-not-exist", "" }) {
            viewer.setFilterString(filter);
            viewer.createComponentSnapshot(viewer.getLocalBounds());
        }

        root.getChild(0).setProperty("Name", "updated", nullptr);
        root.removeChild(root.getNumChildren() - 1, nullptr);
        viewer.setValueTree(root);
        viewer.clearValueTree();

        AutosaveHistoryComponent history(editor);
        history.setBounds(0, 0, 600, 400);
        exerciseControlTree(&history);

        exerciseConnectionDisplay();
    }

    void exerciseConnectionDisplay()
    {
        beginTest("Connection display and touch controls");

        for (auto* connection : cnv->connections) {
            editor->connectionMessageDisplay->setConnection(connection, editor->getScreenBounds().getCentre());
            editor->connectionMessageDisplay->showDisplay();
            editor->connectionMessageDisplay->updateSignalData();
            editor->connectionMessageDisplay->createComponentSnapshot(editor->connectionMessageDisplay->getLocalBounds());
            editor->connectionMessageDisplay->setConnection(nullptr);
        }

        TouchSelectionHelper selectionHelper(editor);
        cnv->deselectAll();
        for (int i = 0; i < jmin(3, static_cast<int>(cnv->objects.size())); ++i)
            cnv->setSelected(cnv->objects[i], true, false);
        if (!cnv->connections.empty())
            cnv->setSelected(cnv->connections.front(), true, false);
        selectionHelper.setBounds(0, 0, 220, 50);
        selectionHelper.show();
        selectionHelper.showObjectProperties();
        exerciseControlTree(&selectionHelper);
        if (auto* more = dynamic_cast<Button*>(selectionHelper.getChildComponent(3)))
            more->onClick();
        ModalComponentManager::getInstance()->cancelAllModalComponents();

        touchCallbackCount = 0;
        TouchPopupMenu submenu;
        submenu.addItem("Sub action", [this] { ++touchCallbackCount; });

        touchMenu = std::make_unique<TouchPopupMenu>();
        touchMenu->addSubMenu("Submenu", submenu);
        touchMenu->addItem("Disabled", [] { }, false);
        touchMenu->addItem("Action", [this] { ++touchCallbackCount; });
        touchMenu->showMenu(editor, editor, "Coverage menu");

        Timer::callAfterDelay(100, [this] { exerciseTouchMenu(); });
    }

    void exerciseTouchMenu()
    {
        if (auto* modal = Component::getCurrentlyModalComponent()) {
            exerciseControlTree(modal);
            HeapArray<Component::SafePointer<Component>> children;
            collectComponents(modal, children);
            for (auto& safe : children) {
                if (auto* component = safe.getComponent(); component && component->getHeight() > 80) {
                    component->mouseDown(mouseEvent(component, { 30.0f, 56.0f }));
                    component->mouseDrag(mouseEvent(component, { 30.0f, 58.0f }));
                    component->mouseUp(mouseEvent(component, { 30.0f, 56.0f }));
                    component->createComponentSnapshot(component->getLocalBounds());
                    component->mouseUp(mouseEvent(component, { 30.0f, 56.0f }));
                    break;
                }
            }
        }
        ModalComponentManager::getInstance()->cancelAllModalComponents();
        touchMenu.reset();

        beginTest("Object drag and plugin mode");
        ObjectDragAndDrop::attachToMouse(editor, "osc~ 440");
        ObjectDragAndDrop::attachToMouse(editor, "ignored");
        Timer::callAfterDelay(100, [this] {
            auto& desktop = Desktop::getInstance();
            for (int i = desktop.getNumComponents(); --i >= 0;) {
                if (auto* drag = dynamic_cast<ObjectDragAndDrop*>(desktop.getComponent(i))) {
                    drag->timerCallback();
                    drag->getMouseCursor();
                    drag->setVisible(false);
                    break;
                }
            }
            exercisePluginMode();
            finish();
        });
    }

    void exercisePluginMode()
    {
        beginTest("Plugin mode");

        auto const pluginDirectory = File::getSpecialLocation(File::tempDirectory)
                                         .getChildFile("plugdata_plugin_mode_coverage");
        pluginDirectory.createDirectory();
        auto const patchFile = pluginDirectory.getChildFile("coverage.pd");
        cnv->refCountedPatch->savePatch(URL(patchFile));
        pluginDirectory.getChildFile("meta.json").replaceWithText(R"({"Scale":1.0,"Zoom":2})");

        auto const originalPluginModeTheme = editor->pd->pluginModeTheme;
        auto const normalCanvasColour = editor->getEditorLookAndFeel().getColours().canvasBackgroundColour;
        auto const normalTheme = editor->getEditorLookAndFeel().getCurrentTheme();
        auto const pluginModeThemeName = PlugDataLook::selectedThemes[0] == normalTheme ? PlugDataLook::selectedThemes[1] : PlugDataLook::selectedThemes[0];
        editor->pd->pluginModeTheme = SettingsFile::getInstance()->getTheme(pluginModeThemeName);
        auto const pluginModeCanvasColour = PlugDataLook::getThemeColour(editor->pd->pluginModeTheme, PlugDataColour::canvasBackgroundColourId);
        {
            auto pluginMode = std::make_unique<PluginMode>(editor, cnv->refCountedPatch);
            expect(getThemeColours(*pluginMode).canvasBackgroundColour == pluginModeCanvasColour, "Plugin mode must use its editor-local colours");
            expect(getThemeColours(*pluginMode->getCanvas()).canvasBackgroundColour == pluginModeCanvasColour, "Plugin mode canvas must inherit the custom colours");
            expect(&pluginMode->getLookAndFeel() == &pluginMode->getCanvas()->getLookAndFeel(), "Plugin mode and its canvas must share the custom look-and-feel");
            expect(editor->getEditorLookAndFeel().getColours().canvasBackgroundColour == normalCanvasColour, "Plugin mode must not mutate the normal editor colours");
            pluginMode->setBounds(editor->getLocalBounds());
            pluginMode->parentSizeChanged();
            pluginMode->resized();
            pluginMode->updateSize();
            pluginMode->setWidthAndHeight(0.75f);
            pluginMode->setWidthAndHeight(1.0f);

            expect(pluginMode->getPatch() == cnv->refCountedPatch);
            expect(pluginMode->getCanvas() != nullptr);
            expect(!pluginMode->isWindowFullscreen());

            HeapArray<Component::SafePointer<Component>> controls;
            collectComponents(pluginMode.get(), controls);
            for (auto& safe : controls) {
                if (auto* combo = dynamic_cast<ComboBox*>(safe.getComponent());
                    combo && combo->getTooltip() == "Change plugin scale") {
                    combo->setSelectedId(1, sendNotificationSync);
                    combo->setSelectedId(7, sendNotificationSync);
                    break;
                }
            }

            pluginMode->createComponentSnapshot(pluginMode->getLocalBounds());
            if (auto* context = editor->getNanoLLGC(); context && context->getContext())
                pluginMode->render(context->getContext());

            auto const centre = pluginMode->getLocalBounds().getCentre().toFloat();
            pluginMode->mouseDown(mouseEvent(pluginMode.get(), centre));
            pluginMode->mouseDrag(mouseEvent(pluginMode.get(), centre + Point<float>(4.0f, 4.0f)));
            pluginMode->mouseUp(mouseEvent(pluginMode.get(), centre));
            expect(!pluginMode->keyPressed(KeyPress('x')));

            pluginMode->setKioskMode(true);
            pluginMode->createComponentSnapshot(pluginMode->getLocalBounds());
            if (auto* context = editor->getNanoLLGC(); context && context->getContext())
                pluginMode->render(context->getContext());
            expect(pluginMode->keyPressed(KeyPress(KeyPress::escapeKey)));

            pluginMode->closePluginMode();
        }
        expect(getThemeColours(*editor).canvasBackgroundColour == normalCanvasColour, "Closing plugin mode must restore the editor-local colours");
        editor->pd->pluginModeTheme = originalPluginModeTheme;
        pluginDirectory.deleteRecursively();
    }

    void finish()
    {
        PopupMenu::dismissAllActiveMenus();
        ModalComponentManager::getInstance()->cancelAllModalComponents();
        Dialogs::dismissFileDialog();
        editor->openedDialog.reset(nullptr);

        if (editor->isInPluginMode() && editor->pluginMode)
            editor->pluginMode->closePluginMode();

        if (auto* sidebar = editor->getSidebarForPanel(Sidebar::PatchSearchPanel))
            sidebar->showPanel(Sidebar::ConsolePanel);

        auto& tabs = editor->getTabComponent();
        while (auto* canvas = tabs.getCurrentCanvas())
            tabs.closeTab(canvas);

        signalDone(true);
    }

    Component::SafePointer<Canvas> cnv;
    std::unique_ptr<TouchPopupMenu> touchMenu;
    std::unique_ptr<ExportingProgressView> exporterView;
    std::unique_ptr<ThemeSettingsPanel> themeSettings;
    std::unique_ptr<KeyMappingSettingsPanel> keyMappingSettings;
    int touchCallbackCount = 0;
    int settingsCallbackCount = 0;
};
