/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

extern "C"
{
#include <m_pd.h>
#include <g_all_guis.h>
}

#include <type_traits>
#include <utility>

#include "Pd/PdGui.h"
#include "Pd/PdPatch.h"
#include "PluginProcessor.h"
#include "x_libpd_extra_utils.h"

#include "Sidebar.h"

class Canvas;

class Box;

struct GUIComponent : public Component, public ComponentListener, public Value::Listener
{
    GUIComponent(const pd::Gui&, Box* parent, bool newObject);

    virtual ~GUIComponent() override;

    virtual std::pair<int, int> getBestSize() = 0;

    virtual void updateValue();

    virtual void update(){};

    virtual void initParameters(bool newObject);

    // Most objects ignore mouseclicks when locked
    // Objects can override this to do custom locking behaviour
    virtual void lock(bool isLocked);

    void componentMovedOrResized(Component& component, bool moved, bool resized) override;

    void paint(Graphics& g) override
    {
        g.setColour(findColour(TextButton::buttonColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        if (gui.isAtom())
        {
            g.setColour(findColour(Slider::thumbColourId));
            Path triangle;
            triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));

            g.fillPath(triangle);
        }
    }

    void closeOpenedSubpatchers();

    static GUIComponent* createGui(const String& name, Box* parent, bool newObject);

    virtual ObjectParameters defineParameters()
    {
        return {};
    };

    virtual ObjectParameters getParameters()
    {
        ObjectParameters params = defineParameters();

        if (gui.isIEM())
        {
            params.push_back({"Foreground", tColour, cAppearance, &primaryColour, {}});
            params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
            params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
            params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
            params.push_back({"Label", tString, cLabel, &labelText, {}});
            params.push_back({"Label Colour", tColour, cLabel, &labelColour, {}});
            params.push_back({"Label X", tInt, cLabel, &labelX, {}});
            params.push_back({"Label Y", tInt, cLabel, &labelY, {}});
            params.push_back({"Label Height", tInt, cLabel, &labelHeight, {}});
        }
        else if (gui.isAtom())
        {
            params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
            params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});

            params.push_back({"Label", tString, cLabel, &labelText, {}});
            params.push_back({"Label Position", tCombo, cLabel, &labelX, {"left", "right", "top", "bottom"}});
        }
        return params;
    }

    virtual pd::Patch* getPatch()
    {
        return nullptr;
    }

    virtual Canvas* getCanvas()
    {
        return nullptr;
    }

    virtual bool fakeGui()
    {
        return false;
    }

    std::unique_ptr<Label> label;

    void updateLabel();

    pd::Gui getGui();

    float getValueOriginal() const noexcept;

    void setValueOriginal(float v);

    float getValueScaled() const noexcept;

    void setValueScaled(float v);

    void startEdition() noexcept;

    void stopEdition() noexcept;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sendSymbol))
        {
            gui.setSendSymbol(sendSymbol.toString());
        }
        else if (value.refersToSameSourceAs(receiveSymbol))
        {
            gui.setReceiveSymbol(sendSymbol.toString());
        }
        else if (value.refersToSameSourceAs(primaryColour))
        {
            auto colour = Colour::fromString(primaryColour.toString());
            gui.setForegroundColour(colour);

            getLookAndFeel().setColour(TextButton::buttonOnColourId, colour);
            getLookAndFeel().setColour(Slider::thumbColourId, colour);
            repaint();
        }
        else if (value.refersToSameSourceAs(secondaryColour))
        {
            auto colour = Colour::fromString(secondaryColour.toString());
            gui.setBackgroundColour(colour);

            getLookAndFeel().setColour(TextEditor::backgroundColourId, colour);
            getLookAndFeel().setColour(TextButton::buttonColourId, colour);
            getLookAndFeel().setColour(Slider::backgroundColourId, colour.brighter(0.14f));

            repaint();
        }

        else if (value.refersToSameSourceAs(labelColour))
        {
            gui.setLabelColour(Colour::fromString(labelColour.toString()));
            updateLabel();
        }
        else if (value.refersToSameSourceAs(labelX))
        {
            if (gui.isAtom())
            {
                gui.setLabelPosition(static_cast<int>(labelX.getValue()) - 1);
                updateLabel();
            }
            else
            {
                gui.setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
                updateLabel();
            }
        }
        else if (value.refersToSameSourceAs(labelY))
        {
            gui.setLabelPosition({static_cast<int>(labelX.getValue()), static_cast<int>(labelY.getValue())});
            updateLabel();
        }
        else if (value.refersToSameSourceAs(labelHeight))
        {
            gui.setFontHeight(static_cast<int>(labelHeight.getValue()));
            updateLabel();
        }
        else if (value.refersToSameSourceAs(labelText))
        {
            gui.setLabelText(labelText.toString());
            updateLabel();
        }
    }

    Box* box;

   protected:
    bool inspectorWasVisible = false;

    const std::string stringGui = std::string("gui");
    const std::string stringMouse = std::string("mouse");

    PlugDataAudioProcessor& processor;
    pd::Gui gui;
    std::atomic<bool> edited;
    float value = 0;
    Value min = Value(0.0f);
    Value max = Value(0.0f);
    int width = 6;

    Value sendSymbol;
    Value receiveSymbol;

    Value primaryColour = Value(findColour(Slider::thumbColourId).toString());
    Value secondaryColour = Value(findColour(ComboBox::backgroundColourId).toString());

    Value labelColour = Value(Colours::white.toString());
    Value labelX = Value(0.0f);
    Value labelY = Value(0.0f);
    Value labelHeight = Value(18.0f);

    Value labelText;
};

struct BangComponent : public GUIComponent
{
    uint32_t lastBang;

    Value bangInterrupt = Value(40.0f);
    Value bangHold = Value(40.0f);

    TextButton bangButton;

    BangComponent(const pd::Gui&, Box* parent, bool newObject);

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    ObjectParameters defineParameters() override
    {
        return {
            {"Interrupt", tInt, cGeneral, &bangInterrupt, {}},
            {"Hold", tInt, cGeneral, &bangHold, {}},
        };

        /*
         [this](int item) {
         if(item == 0) {
         static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold = static_cast<float>(bangInterrupt.getValue());
         }
         else if(item == 1) {
         static_cast<t_bng*>(gui.getPointer())->x_flashtime_break = static_cast<float>(bangHold.getValue());
         }
         }}; */
    }

    void initParameters(bool newObject) override
    {
        GUIComponent::initParameters(newObject);

        bangInterrupt = static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold;
    };

    void update() override;

    void resized() override;
};

struct ToggleComponent : public GUIComponent
{
    TextButton toggleButton;

    ToggleComponent(const pd::Gui&, Box* parent, bool newObject);

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    void resized() override;

    void update() override;
};

struct MessageComponent : public GUIComponent
{
    bool isDown = false;
    bool isLocked = false;

    Label input;

    std::string lastMessage;

    MessageComponent(const pd::Gui&, Box* parent, bool newObject);

    std::pair<int, int> getBestSize() override
    {
        updateValue();  // make sure text is loaded

        // auto [x, y, w, h] = gui.getBounds();
        int stringLength = std::max(10, input.getFont().getStringWidth(input.getText()));
        return {stringLength + 20, numLines * 21};
    };

    void lock(bool isLocked) override;

    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        // Edit messages when unlocked
        if (e.getNumberOfClicks() == 2 && !isLocked && !gui.isAtom())
        {
            input.showEditor();
        }
        // Edit atoms when locked
        else if (e.getNumberOfClicks() == 2 && isLocked && gui.isAtom())
        {
            input.showEditor();
        }

        if (!gui.isAtom())
        {
            isDown = true;
            repaint();

            startEdition();
            gui.click();
            stopEdition();
        }
    }

    void mouseUp(const MouseEvent& e) override
    {
        isDown = false;
        repaint();
    }

    void updateValue() override;

    void resized() override;

    void update() override;

    void paint(Graphics& g) override;

    void paintOverChildren(Graphics& g) override;

    int numLines = 1;
    int longestLine = 7;
};

struct NumboxComponent : public GUIComponent
{
    Label input;

    float downValue = 0.0f;
    bool shift = false;

    NumboxComponent(const pd::Gui&, Box* parent, bool newObject);

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (!input.isBeingEdited())
        {
            startEdition();
            shift = e.mods.isShiftDown();
            downValue = input.getText().getFloatValue();
        }
    }

    void mouseUp(const MouseEvent& e) override
    {
        if (!input.isBeingEdited())
        {
            stopEdition();
        }
    }

    void mouseDrag(const MouseEvent& e) override
    {
        input.mouseDrag(e);

        if (!input.isBeingEdited())
        {
            auto const inc = static_cast<float>(-e.getDistanceFromDragStartY()) * 0.5f;
            if (std::abs(inc) < 1.0f) return;

            // Logic for dragging, where the x position decides the precision
            auto currentValue = input.getText();
            if (!currentValue.containsChar('.')) currentValue += '.';
            if (currentValue.getCharPointer()[0] == '-') currentValue = currentValue.substring(1);
            currentValue += "00000";

            // Get position of all numbers
            Array<int> glyphs;
            Array<float> xOffsets;
            input.getFont().getGlyphPositions(currentValue, glyphs, xOffsets);

            // Find the number closest to the mouse down
            auto position = static_cast<float>(gui.isAtom() ? e.getMouseDownX() - 4 : e.getMouseDownX() - 15);
            auto precision = static_cast<int>(std::lower_bound(xOffsets.begin(), xOffsets.end(), position) - xOffsets.begin());
            precision -= currentValue.indexOfChar('.');

            // I case of integer dragging
            if (shift || precision <= 0)
            {
                precision = 0;
            }
            else
            {
                // Offset for the decimal point character
                precision -= 1;
            }

            // Calculate increment multiplier
            float multiplier = powf(10.0f, static_cast<float>(-precision));

            float minimum = static_cast<float>(min.getValue());
            float maximum = static_cast<float>(max.getValue());
            // Calculate new value as string
            auto newValue = String(std::clamp(downValue + inc * multiplier, minimum, maximum), precision);

            if (precision == 0) newValue = newValue.upToFirstOccurrenceOf(".", true, false);

            setValueOriginal(newValue.getFloatValue());
            input.setText(newValue, NotificationType::dontSendNotification);
        }
    }

    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tFloat, cGeneral, &min, {}}, {"Maximum", tFloat, cGeneral, &max, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        }
        if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(TextEditor::backgroundColourId));
        g.fillRect(getLocalBounds().reduced(1));
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);

        if (!gui.isAtom())
        {
            g.setColour(Colour(gui.getForegroundColor()));

            Path triangle;
            triangle.addTriangle({0.0f, 0.0f}, {10.0f, static_cast<float>(getHeight()) / 2.0f}, {0.0f, static_cast<float>(getHeight())});

            g.fillPath(triangle);
        }
    }

    void resized() override;

    void update() override;
};

struct ListComponent : public GUIComponent
{
    ListComponent(const pd::Gui& gui, Box* parent, bool newObject);

    void paint(Graphics& g) override;

    void update() override;

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

   private:
    Label label;
};

struct SliderComponent : public GUIComponent
{
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    Slider slider;

    SliderComponent(bool vertical, const pd::Gui& gui, Box* parent, bool newObject);

    ~SliderComponent();

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    ObjectParameters defineParameters() override
    {
        return {
            {"Minimum", tFloat, cGeneral, &min, {}},
            {"Maximum", tFloat, cGeneral, &max, {}},
            {"Logarithmic", tBool, cGeneral, &isLogarithmic, {"off", "on"}},
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
        }
        else if (value.refersToSameSourceAs(isLogarithmic))
        {
            gui.setLogScale(isLogarithmic == true);
            min = gui.getMinimum();
            max = gui.getMaximum();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void resized() override;

    void update() override;
};

struct RadioComponent : public GUIComponent
{
    int lastState = 0;

    Value minimum = Value(0.0f), maximum = Value(8.0f);

    bool isVertical;

    RadioComponent(bool vertical, const pd::Gui& gui, Box* parent, bool newObject);

    OwnedArray<TextButton> radioButtons;

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tInt, cGeneral, &minimum, {}}, {"Maximum", tInt, cGeneral, &maximum, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
            updateRange();
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
            updateRange();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void resized() override;

    void update() override;

    void updateRange();
};

struct GraphicalArray : public Component, public Timer
{
   public:
    GraphicalArray(PlugDataAudioProcessor* pd, pd::Array& graph);

    void paint(Graphics& g) override;

    void mouseDown(const MouseEvent& event) override;

    void mouseDrag(const MouseEvent& event) override;

    void mouseUp(const MouseEvent& event) override;

    size_t getArraySize() const noexcept;

   private:
    void timerCallback() override;

    template <typename T>
    T clip(const T& n, const T& lower, const T& upper)
    {
        return std::max(std::min(n, upper), lower);
    }

    pd::Array array;
    std::vector<float> vec;
    std::vector<float> temp;
    std::atomic<bool> edited;
    bool error = false;
    const std::string stringArray = std::string("array");

    PlugDataAudioProcessor* pd;
};

struct ArrayComponent : public GUIComponent
{
   public:
    ArrayComponent(const pd::Gui& gui, Box* box, bool newObject);

    void paint(Graphics&) override
    {
    }

    void resized() override;

    void updateValue() override
    {
    }

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

   private:
    pd::Array graph;
    GraphicalArray array;
};

struct GraphOnParent : public GUIComponent
{
    bool isLocked = false;

   public:
    GraphOnParent(const pd::Gui& gui, Box* box, bool newObject);

    ~GraphOnParent() override;

    void resized() override;

    void lock(bool isLocked) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;

    void updateValue() override;

    // override to make transparent
    void paint(Graphics& g) override
    {
    }

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    Canvas* getCanvas() override
    {
        return canvas.get();
    }

    void updateCanvas();

   private:
    pd::Patch subpatch;
    std::unique_ptr<Canvas> canvas;
};

struct Subpatch : public GUIComponent
{
    Subpatch(const pd::Gui& gui, Box* box, bool newObject);

    ~Subpatch() override;

    std::pair<int, int> getBestSize() override
    {
        return {0, 3};
    };

    void resized() override{};

    void updateValue() override;

    pd::Patch* getPatch() override
    {
        return &subpatch;
    }

    bool fakeGui() override
    {
        return true;
    }

   private:
    pd::Patch subpatch;
};

struct CommentComponent : public GUIComponent
{
    CommentComponent(const pd::Gui& gui, Box* box, bool newObject);

    void paint(Graphics& g) override;

    void updateValue() override{};

    void resized() override
    {
        gui.setSize(getWidth(), getHeight());
    }

    std::pair<int, int> getBestSize() override
    {
        return {120, 4};
    };

    bool fakeGui() override
    {
        return true;
    }
};

struct VUMeter : public GUIComponent
{
    VUMeter(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject)
    {
    }

    ~VUMeter() override
    {
    }

    void resized() override
    {
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colours::white);

        auto rms = gui.getValue();
        auto peak = gui.getPeak();

        g.drawFittedText(String(rms, 2) + " dB", Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);

        int height = getHeight();
        int width = getWidth();

        auto outerBorderWidth = 2.0f;
        auto totalBlocks = 15;
        auto spacingFraction = 0.03f;

        auto doubleOuterBorderWidth = 2.0f * outerBorderWidth;

        auto blockWidth = (width - doubleOuterBorderWidth) / static_cast<float>(totalBlocks);
        auto blockHeight = height - doubleOuterBorderWidth;
        auto blockRectWidth = (1.0f - 2.0f * spacingFraction) * blockWidth;
        auto blockRectSpacing = spacingFraction * blockWidth;
        auto blockCornerSize = 0.1f * blockWidth;
        auto c = findColour(Slider::thumbColourId);

        float lvl = (float)std::exp(std::log(peak) / 3.0) * (peak > 0.002);
        auto numBlocks = roundToInt(totalBlocks * lvl);

        for (auto i = 0; i < totalBlocks; ++i)
        {
            if (i >= numBlocks)
                g.setColour(c.withAlpha(0.5f));
            else
                g.setColour(i < totalBlocks - 1 ? c : Colours::red);

            g.fillRoundedRectangle(outerBorderWidth + (i * blockWidth) + blockRectSpacing, outerBorderWidth, blockRectWidth, blockHeight, blockCornerSize);
        }
    }

    void updateValue() override
    {
        auto rms = gui.getValue();
        auto peak = gui.getPeak();
    };

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };
};

struct PanelComponent : public GUIComponent
{
    PanelComponent(const pd::Gui& gui, Box* box, bool newObject);

    void paint(Graphics& g) override
    {
        g.fillAll(Colour::fromString(secondaryColour.toString()));
    }

    void resized() override
    {
        gui.setSize(getWidth(), getHeight());
    }

    void updateValue() override{};

    ObjectParameters getParameters() override
    {
        ObjectParameters params;
        params.push_back({"Background", tColour, cAppearance, &secondaryColour, {}});
        params.push_back({"Send Symbol", tString, cGeneral, &sendSymbol, {}});
        params.push_back({"Receive Symbol", tString, cGeneral, &receiveSymbol, {}});
        return params;
    }

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };
};

// ELSE mousepad
struct MousePad : public GUIComponent
{
    bool isLocked = false;
    bool isPressed = false;

    typedef struct _pad
    {
        t_object x_obj;
        t_glist* x_glist;
        void* x_proxy;  // dont have this object and dont need it
        t_symbol* x_bindname;
        int x_x;
        int x_y;
        int x_w;
        int x_h;
        int x_sel;
        int x_zoom;
        int x_edit;
        unsigned char x_color[3];
    } t_pad;

    MousePad(const pd::Gui& gui, Box* box, bool newObject);

    ~MousePad() override;

    void paint(Graphics& g) override;

    void lock(bool isLocked) override;

    void updateValue() override;

    void mouseDown(const MouseEvent& e) override;

    void mouseMove(const MouseEvent& e) override;

    void mouseUp(const MouseEvent& e) override;

    void mouseDrag(const MouseEvent& e) override;

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };
};

// Else "mouse" component
struct MouseComponent : public GUIComponent
{
    typedef struct _mouse
    {
        t_object x_obj;
        int x_hzero;
        int x_vzero;
        int x_zero;
        int x_wx;
        int x_wy;
        t_glist* x_glist;
        t_outlet* x_horizontal;
        t_outlet* x_vertical;
    } t_mouse;

    MouseComponent(const pd::Gui& gui, Box* box, bool newObject);

    ~MouseComponent() override;

    std::pair<int, int> getBestSize() override
    {
        return {0, 3};
    };

    void resized() override{};

    void updateValue() override;

    bool fakeGui() override
    {
        return true;
    }

    void mouseDown(const MouseEvent& e) override;

    void mouseMove(const MouseEvent& e) override;

    void mouseUp(const MouseEvent& e) override;

    void mouseDrag(const MouseEvent& e) override;
};

// ELSE keyboard
struct KeyboardComponent : public GUIComponent, public MidiKeyboardStateListener
{
    typedef struct _edit_proxy
    {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _keyboard* p_cnv;
    } t_edit_proxy;

    typedef struct _keyboard
    {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int* x_tgl_notes;  // to store which notes should be played
        int x_velocity;    // to store velocity
        int x_last_note;   // to store last note
        float x_vel_in;    // to store the second inlet values
        float x_space;
        int x_width;
        int x_height;
        int x_octaves;
        int x_first_c;
        int x_low_c;
        int x_toggle_mode;
        int x_norm;
        int x_zoom;
        int x_shift;
        int x_xpos;
        int x_ypos;
        int x_snd_set;
        int x_rcv_set;
        int x_flag;
        int x_s_flag;
        int x_r_flag;
        int x_edit;
        t_symbol* x_receive;
        t_symbol* x_rcv_raw;
        t_symbol* x_send;
        t_symbol* x_snd_raw;
        t_symbol* x_bindsym;
        t_outlet* x_out;
    } t_keyboard;

    KeyboardComponent(const pd::Gui& gui, Box* box, bool newObject);

    void paint(Graphics& g) override{};

    void updateValue() override;

    void resized() override;

    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w - 28, h};
    };

    MidiKeyboardState state;
    MidiKeyboardComponent keyboard;
};

struct _fielddesc
{
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union
    {
        t_float fd_float;    /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1; /* min and max values */
    float fd_v2;
    float fd_screen1; /* min and max screen values */
    float fd_screen2;
    float fd_quantum; /* quantization in value */
};

struct TemplateDraw
{
    struct t_curve
    {
        t_object x_obj;
        int x_flags; /* CLOSED, BEZ, NOMOUSERUN, NOMOUSEEDIT */
        t_fielddesc x_fillcolor;
        t_fielddesc x_outlinecolor;
        t_fielddesc x_width;
        t_fielddesc x_vis;
        int x_npoints;
        t_fielddesc* x_vec;
        t_canvas* x_canvas;
    };

    static void paintOnCanvas(Graphics& g, Canvas* canvas, t_scalar* scalar, t_gobj* obj, int baseX, int baseY);
};
