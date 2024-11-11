/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Object.h"
#include "Objects/ObjectBase.h"
#include "Objects/AllGuis.h"
#include <m_pd.h>
#include <m_imp.h>

extern "C" {
#include <g_all_guis.h>
}

static int srl_is_valid(t_symbol const* s)
{
    return (s != nullptr && s != gensym(""));
}

class OpenInspector : public Component {
    TextButton buttonOpenInspector;
public:
    OpenInspector()
    {
        auto backgroundColour = findColour(PlugDataColour::dialogBackgroundColourId);
        buttonOpenInspector.setColour(TextButton::buttonColourId, backgroundColour.contrasting(0.05f));
        buttonOpenInspector.setColour(TextButton::buttonOnColourId, backgroundColour.contrasting(0.1f));
        buttonOpenInspector.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        buttonOpenInspector.setButtonText("Open inspector");
        buttonOpenInspector.setTooltip("Open inspector for object");

        addAndMakeVisible(buttonOpenInspector);

        setSize(108, 33);
    };

    void setButtonOnClick(std::function<void()> onClick)
    {
        buttonOpenInspector.onClick = onClick;
    }

    void resized() override
    {
        buttonOpenInspector.setBounds(4, 4, 100, 25);
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenInspector);
};

class SearchPanelSettings : public Component {
public:
    struct SearchPanelSettingsButton : public TextButton {
        String const icon;
        String const description;

        SearchPanelSettingsButton(String iconString, String descriptionString)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
            setClickingTogglesState(true);

            auto sortLayerOrder = SettingsFile::getInstance()->getProperty<bool>("search_order");
            setToggleState(sortLayerOrder, dontSendNotification);

            onClick = [this]() {
                SettingsFile::getInstance()->setProperty("search_order", var(getToggleState()));
            };
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::toolbarTextColourId);
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(32), colour, 14);

            if (getToggleState()) {
                colour = findColour(PlugDataColour::toolbarActiveColourId);
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    SearchPanelSettings()
    {
        addAndMakeVisible(sortLayerOrder);

        setSize(150, 28);
    };

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int buttonHeight = buttonBounds.getHeight();

        sortLayerOrder.setBounds(buttonBounds.removeFromTop(buttonHeight));
    }

private:
    SearchPanelSettingsButton sortLayerOrder = SearchPanelSettingsButton(Icons::AutoScroll, "Display layer order");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPanelSettings);
};

class SearchPanel : public Component
    , public KeyListener
    , public Timer {
public:
    explicit SearchPanel(PluginEditor* pluginEditor)
        : editor(pluginEditor)
    {
        input.setBackgroundColour(PlugDataColour::sidebarActiveBackgroundColourId);
        input.setTextToShowWhenEmpty("Type to search in patch", findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));

        input.onTextChange = [this]() {
            patchTree.setFilterString(input.getText());
        };

        input.addKeyListener(this);
        patchTree.addKeyListener(this);

        patchTree.onClick = [this](ValueTree& tree) {
            auto* ptr = reinterpret_cast<void*>(static_cast<int64>(tree.getProperty("Object")));
            if (auto obj = editor->highlightSearchTarget(ptr, true)){
                auto launchInspector = [this, obj](){
                    SmallArray<ObjectParameters, 6> parameters = { obj->gui->getParameters() };
                    auto toShow = SmallArray<Component*>();
                    toShow.add(obj);
                    editor->sidebar->showParameters(toShow, parameters);
                };
                MessageManager::callAsync(launchInspector);
            }
        };

        patchTree.onSelect = [this](ValueTree& tree) {
            auto* ptr = reinterpret_cast<void*>(static_cast<int64>(tree.getProperty("TopLevel")));
            if (auto obj = editor->highlightSearchTarget(ptr, false)){
                auto launchInspector = [this, obj](){
                    SmallArray<ObjectParameters, 6> parameters = { obj->gui->getParameters() };
                    auto toShow = SmallArray<Component*>();
                    toShow.add(obj);
                    editor->sidebar->showParameters(toShow, parameters);
                };
                MessageManager::callAsync(launchInspector);
            }
        };

        patchTree.onRightClick = [this](ValueTree& tree) {
            auto* ptr = reinterpret_cast<void*>(static_cast<int64>(tree.getProperty("Object")));

            auto pos = Desktop::getInstance().getMousePosition();
            auto bounds = Rectangle<int>(pos.x, pos.y, 10, 10);

            auto openInspector = std::make_unique<OpenInspector>();
            auto* rawOpenInspectorPtr = openInspector.get();
            auto& callOutBox = CallOutBox::launchAsynchronously(std::move(openInspector), bounds, nullptr);

            SafePointer<CallOutBox> callOutBoxSafePtr(&callOutBox);

            auto onClick = [this, ptr, callOutBoxSafePtr](){
                if (auto obj = editor->highlightSearchTarget(ptr, true)){
                    // FIXME: We have to wait until EVERYTHING has setup on the new canvas
                    // So we call it on message thread, which should place this event after the previous
                    auto launchInspector = [this, obj](){
                        SmallArray<ObjectParameters, 6> parameters = { obj->gui->getParameters() };
                        auto toShow = SmallArray<Component*>();
                        toShow.add(obj);
                        editor->sidebar->showParameters(toShow, parameters);
                    };
                    MessageManager::callAsync(launchInspector);
                }
                if (callOutBoxSafePtr)
                    callOutBoxSafePtr->dismiss();
            };

            rawOpenInspectorPtr->setButtonOnClick(onClick);
        };

        addAndMakeVisible(patchTree);
        addAndMakeVisible(input);

        // TODO: dismiss this tooltip when the input text editor is active!
        input.setTooltip("Use \"send\" or \"receive\" keyword to search symbols, \"symbols\" show all symbols");
        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 5, 1 });
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        return false;
    }

    void clear()
    {
        patchTree.clearValueTree();
    }

    void timerCallback() override
    {
        auto* cnv = editor->getCurrentCanvas();
        if (cnv && (currentCanvas.getComponent() != cnv || cnv->needsSearchUpdate)) {
            currentCanvas = cnv;
            currentCanvas->needsSearchUpdate = false;
            updateResults();
        }
    }

    void visibilityChanged() override
    {
        if (isVisible()) {
            startTimer(100);
        } else {
            stopTimer();
        }
    }

    void lookAndFeelChanged() override
    {
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        input.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::sidebarTextColourId));
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarBackgroundColourId));
        g.fillRect(getLocalBounds());

        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
        g.fillRoundedRectangle(input.getBounds().reduced(6, 4).toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto backgroundColour = findColour(PlugDataColour::sidebarBackgroundColourId);
        auto transparentColour = backgroundColour.withAlpha(0.0f);

        // Draw a gradient to fade the content out underneath the search input
        g.setGradientFill(ColourGradient(backgroundColour, 0.0f, 30.0f, transparentColour, 0.0f, 42.0f, false));
        g.fillRect(Rectangle<int>(0, input.getBottom(), getWidth(), 12));

        auto colour = findColour(PlugDataColour::sidebarTextColourId);
        Fonts::drawIcon(g, Icons::Search, 2, 1, 32, colour, 12);
    }

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show search settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [settingsCalloutButton]() {
            auto bounds = settingsCalloutButton->getScreenBounds();
            auto docsSettings = std::make_unique<SearchPanelSettings>();
            CallOutBox::launchAsynchronously(std::move(docsSettings), bounds, nullptr);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

    void updateResults()
    {
        auto* cnv = editor->getCurrentCanvas();
        if (cnv && isVisible()) {
            cnv->pd->lockAudioThread(); // It locks inside of this anyway, so we might as well lock around it to prevent constantly locking/unlocking

            // Get the currently selected object
            auto selectedObj = patchTree.getSelectedNodeObject();

            patchTree.setValueTree(generatePatchTree(cnv->refCountedPatch));

            // If the object is still selected, reselect it
            auto numSelectedObject = 0;
            auto foundInCanvas = false;
            for (auto item : cnv->getLassoSelection())
            {
                if (auto* obj = dynamic_cast<Object*>(item.get())) {
                    numSelectedObject++;
                    if (selectedObj == obj->getPointer()){
                        foundInCanvas = true;
                    }
                }
            }
            if (foundInCanvas && numSelectedObject == 1)
                patchTree.setSelectedNode(selectedObj);
            else
                patchTree.setSelectedNode(nullptr);

            patchTree.filterNodes();
            cnv->pd->unlockAudioThread();
            patchTree.repaint();
        }
    }

    void grabFocus()
    {
        input.grabKeyboardFocus();
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(34);

        tableBounds.removeFromTop(2);

        input.setBounds(inputBounds.reduced(5, 4));
        patchTree.setBounds(tableBounds);
    }

    ValueTree generatePatchTree(pd::Patch::Ptr patch, void* topLevel = nullptr)
    {
        currentCanvas = editor->getCurrentCanvas();

        ValueTree patchTree("Patch");
        for (auto objectPtr : patch->getObjects()) {
            if (auto object = objectPtr.get<t_pd>()) {
                auto* top = topLevel ? topLevel : object.get();
                String type = String::fromUTF8(pd::Interface::getObjectClassName(object.get()));

                if (!pd::Interface::checkObject(object.get()))
                    continue;

                char* objectText;
                int len;
                pd::Interface::getObjectText(object.cast<t_text>(), &objectText, &len);

                int x, y, w, h;
                pd::Interface::getObjectBounds(patch->getPointer().get(), object.cast<t_gobj>(), &x, &y, &w, &h);

                auto name = String::fromUTF8(objectText, len);
                auto nameWithoutArgs = name.upToFirstOccurrenceOf(" ", false, false);
                auto positionText = " (" + String(x) + ":" + String(y) + ")";

                auto getFirstArgumentFromFullName = [](String const& fullName) -> String {
                    return fullName.fromFirstOccurrenceOf(" ", false, true).upToFirstOccurrenceOf(" ", false, true);
                };

                ValueTree element("Object");
                if (type == "canvas" || type == "graph") {
                    pd::Patch::Ptr subpatch = new pd::Patch(objectPtr, editor->pd, false);
                    ValueTree subpatchTree = generatePatchTree(subpatch, top);
                    element.copyPropertiesAndChildrenFrom(subpatchTree, nullptr);

                    if (auto patchPtr = subpatch->getPointer()) {
                        if (patchPtr->gl_list) {
                            t_class* c = patchPtr->gl_list->g_pd;
                            if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                                StringArray arrays;
                                auto arrayIt = patchPtr->gl_list;
                                while (arrayIt) {
                                    if (auto* array = reinterpret_cast<t_fake_garray*>(arrayIt))
                                        arrays.add(String::fromUTF8(array->x_name->s_name));
                                    arrayIt = arrayIt->g_next;
                                }
                                String formatedArraysText;
                                for (int i = 0; i < arrays.size(); i++) {
                                    formatedArraysText += arrays[i] + String(i < arrays.size() - 1 ? ", " : "");
                                }
                                name = "array: " + formatedArraysText;
                            } else if (patchPtr->gl_isgraph) {
                                name = nameWithoutArgs;
                            }
                        } else if (patchPtr->gl_isgraph) {
                            name = nameWithoutArgs;
                        }
                    }
#ifdef SHOW_PD_SUBPATCH_SYMBOL
                    if (nameWithoutArgs == "pd") {
                        auto arg = getFirstArgumentFromFullName(name);
                        if (arg.isNotEmpty())
                            element.setProperty("PDSymbol", nameWithoutArgs + "-" + arg, nullptr);
                    }
#endif
                    element.setProperty("Name", name, nullptr);
                    element.setProperty("RightText", positionText, nullptr);
                    element.setProperty("Icon", canvas_isabstraction(subpatch->getPointer().get()) ? Icons::File : Icons::Object, nullptr);
                    element.setProperty("Object", reinterpret_cast<int64>(object.cast<void>()), nullptr);
                    if (currentCanvas) {
                        for (auto comp: currentCanvas->getLassoSelection()) {
                            if (auto obj = dynamic_cast<Object *>(comp.get())) {
                                if (obj->getPointer() == object.cast<t_gobj>())
                                    element.setProperty("Selected", true, nullptr);
                            }
                        }
                    }
                    element.setProperty("TopLevel", reinterpret_cast<int64>(top), nullptr);
                } else {
                    String finalFormatedName;
                    String sendSymbol;
                    String receiveSymbol;

                    switch (hash(type)) {
                    // IEM send-receive symbols
                    case hash("bng"):
                    case hash("hsl"):
                    case hash("vsl"):
                    case hash("slider"):
                    case hash("tgl"):
                    case hash("nbx"):
                    case hash("vradio"):
                    case hash("hradio"):
                    case hash("vu"):
                    case hash("cnv"): {
                        if (auto iemgui = objectPtr.get<t_iemgui>()) {
                            t_symbol* srlsym[3];
                            iemgui_all_sym2dollararg(iemgui.get(), srlsym);
                            if (srl_is_valid(srlsym[0])) {
                                sendSymbol = String::fromUTF8(iemgui->x_snd_unexpanded->s_name);
                            }
                            if (srl_is_valid(srlsym[1])) {
                                receiveSymbol = String::fromUTF8(iemgui->x_rcv_unexpanded->s_name);
                            }
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("keyboard"): {
                        if (auto keyboardObject = object.cast<t_fake_keyboard>()) {
                            sendSymbol = String(keyboardObject->x_send->s_name);
                            receiveSymbol = String(keyboardObject->x_receive->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("pic"): {
                        if (auto picObject = object.cast<t_fake_pic>()) {
                            sendSymbol = String(picObject->x_send->s_name);
                            receiveSymbol = String(picObject->x_receive->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("scope~"): {
                        if (auto scopeObject = object.cast<t_fake_scope>()) {
                            receiveSymbol = String(scopeObject->x_receive->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("function"): {
                        if (auto keyboardObject = object.cast<t_fake_function>()) {
                            sendSymbol = String(keyboardObject->x_send->s_name);
                            receiveSymbol = String(keyboardObject->x_receive->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("note"): {
                        if (auto noteObject = object.cast<t_fake_note>()) {
                            receiveSymbol = String(noteObject->x_receive->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("knob"): {
                        if (auto knobObj = object.cast<t_fake_knob>()) {
                            sendSymbol = String(knobObj->x_snd->s_name);
                            receiveSymbol = String(knobObj->x_rcv->s_name);
                        }
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }
                    case hash("gatom"): {
                        auto gatomObject = object.cast<t_fake_gatom>();
                        String gatomName;
                        switch (gatomObject->a_flavor) {
                        case A_FLOAT:
                            gatomName = "floatbox";
                            break;
                        case A_SYMBOL:
                            gatomName = "symbolbox";
                            break;
                        case A_NULL:
                            gatomName = "listbox";
                            break;
                        default:
                            break;
                        }
                        receiveSymbol = String(gatomObject->a_symfrom->s_name);
                        sendSymbol = String(gatomObject->a_symto->s_name);
                        finalFormatedName = gatomName;
                        break;
                    }
                    case hash("message"): {
                        finalFormatedName = "msg: " + name;
                        break;
                    }
                    case hash("comment"): {
                        finalFormatedName = "comment: " + name;
                        break;
                    }
                    case hash("text"): {
                        switch (object.cast<t_fake_text_define>()->x_textbuf.b_ob.te_type) {
                        case T_TEXT: {
                            // if object & classname is text, then it's a comment
                            finalFormatedName = String("comment: ") + name;
                            break;
                        }
                        case T_OBJECT: {
                            // if object is T_OBJECT but classname is 'text' object is in error state
                            element.setProperty("IconColour", Colours::red.toString(), nullptr);

                            if (name.isEmpty())
                                finalFormatedName = String("empty");
                            else
                                finalFormatedName = String("unknown: ") + name;

                            break;
                        }
                        default:
                            break;
                        }
                        break;
                    }
                    case hash("canvas"):
                    case hash("bicoeff"):
                    case hash("messbox"):
                    case hash("pad"):
                    case hash("button"): {
                        finalFormatedName = nameWithoutArgs;
                        break;
                    }

                    default: {
                        switch (hash(nameWithoutArgs)) {
                        case hash("s"):
                        case hash("s~"):
                        case hash("send"):
                        case hash("send~"):
                        case hash("throw~"): {
                            sendSymbol = getFirstArgumentFromFullName(name);
                            element.setProperty("SendObject", 1, nullptr);
                            finalFormatedName = nameWithoutArgs;
                            break;
                        }
                        case hash("r"):
                        case hash("r~"):
                        case hash("receive"):
                        case hash("receive~"):
                        case hash("catch~"): {
                            receiveSymbol = getFirstArgumentFromFullName(name);
                            element.setProperty("SendObject", 1, nullptr);
                            finalFormatedName = nameWithoutArgs;
                            break;
                        }
                        case hash("t"):
                        case hash("trigger"):
                            element.setProperty("TriggerObject", 1, nullptr);
                            finalFormatedName = name;
                            break;
                        default:
                            finalFormatedName = name;
                            break;
                        }
                        break;
                    }
                    }

                    element.setProperty("Name", finalFormatedName, nullptr);
                    // Add send/receive tags if they exist
                    if (sendSymbol.isNotEmpty() && (sendSymbol != "empty") && (sendSymbol != "nosndno")) {
                        element.setProperty("SendSymbol", sendSymbol, nullptr);
                    }
                    if (receiveSymbol.isNotEmpty() && (receiveSymbol != "empty")) {
                        element.setProperty("ReceiveSymbol", receiveSymbol, nullptr);
                    }
                    element.setProperty("RightText", positionText, nullptr);
                    element.setProperty("Icon", Icons::Object, nullptr);
                    element.setProperty("Object", reinterpret_cast<int64>(object.cast<void>()), nullptr);
                    if (currentCanvas) {
                        for (auto comp: currentCanvas->getLassoSelection()) {
                            if (auto obj = dynamic_cast<Object *>(comp.get())) {
                                if (obj->getPointer() == object.cast<t_gobj>())
                                    element.setProperty("Selected", true, nullptr);
                            }
                        }
                    }
                    element.setProperty("TopLevel", reinterpret_cast<int64>(top), nullptr);
                }

                patchTree.appendChild(element, nullptr);
            }
        }

        return patchTree;
    }

    SafePointer<Canvas> currentCanvas;
    PluginEditor* editor;
    ValueTreeViewerComponent patchTree = ValueTreeViewerComponent("(Subpatch)");
    SearchEditor input;
};
