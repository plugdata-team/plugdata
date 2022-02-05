/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <m_pd.h>
#include <g_all_guis.h>

#include "../Libraries/ff_meters/ff_meters.h"

#include "LookAndFeel.h"
#include "Pd/PdGui.h"
#include "Pd/PdPatch.h"
#include "x_libpd_extra_utils.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <m_pd.h>
#include <type_traits>
#include <utility>

class Canvas;
class Box;
struct GUIComponent : public Component, public ComponentListener {
    
    GUIComponent(pd::Gui gui, Box* parent);
    
    virtual ~GUIComponent();
    virtual std::pair<int, int> getBestSize() = 0;
    
    virtual void updateValue();
    virtual void update() {};
    
    virtual void initParameters();
    
    void componentMovedOrResized(Component& component, bool moved, bool resized) override;
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(TextButton::buttonColourId));
        g.fillRect(getLocalBounds().reduced(2));
    }
    
    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawLine(0, 0, getWidth(), 0);
    }
    
    void closeOpenedSubpatchers();
    
    static GUIComponent* createGui(String name, Box* parent);
    
    void setForeground(Colour colour);
    void setBackground(Colour colour);
    
    virtual ObjectParameters defineParamters() { return {}; };
    
    ObjectParameters getParameters()
    {
        ObjectParameters params = defineParamters();
        
        auto& [parameters, callback] = params;
        
        if (gui.isIEM()) {
            parameters.insert(parameters.begin(), { "Foreground", tColour, (void*)&primaryColour });
            parameters.insert(parameters.begin() + 1, { "Background", tColour, (void*)&secondaryColour });
            parameters.insert(parameters.begin() + 2, { "Send Symbol", tString, (void*)&sendSymbol });
            parameters.insert(parameters.begin() + 3, { "Receive Symbol", tString, (void*)&receiveSymbol });
            
            auto oldCallback = callback;
            callback = [this, oldCallback](int changedParameter) {
                if (changedParameter == 0) {
                    setForeground(Colour::fromString(primaryColour));
                    repaint();
                } else if (changedParameter == 1) {
                    setBackground(Colour::fromString(secondaryColour));
                    repaint();
                }
                
                else if (changedParameter == 2) {
                    gui.setSendSymbol(sendSymbol.toStdString());
                    repaint();
                } else if (changedParameter == 3) {
                    gui.setReceiveSymbol(receiveSymbol.toStdString());
                    repaint();
                } else {
                    oldCallback(changedParameter - 4);
                }
            };
        }
        else if (gui.isAtom()) {
            auto& [parameters, callback] = params;
            
            parameters.insert(parameters.begin(), { "Width", tInt, (void*)&width });
            parameters.insert(parameters.begin() + 1, { "Send Symbol", tString, (void*)&sendSymbol });
            parameters.insert(parameters.begin() + 2, { "Receive Symbol", tString, (void*)&receiveSymbol });
            
            auto oldCallback = callback;
            callback = [this, oldCallback](int changedParameter) {
                if (changedParameter == 0) {
                    // Width
                }
                else if (changedParameter == 1) {
                    gui.setSendSymbol(sendSymbol.toStdString());
                    repaint();
                } else if (changedParameter == 2) {
                    gui.setReceiveSymbol(receiveSymbol.toStdString());
                    repaint();
                } else {
                    oldCallback(changedParameter - 3);
                }
            };
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
    
    virtual bool fakeGUI()
    {
        return false;
    }
    
    std::unique_ptr<Label> label;
    
    void updateLabel();
    pd::Gui getGUI();
    
    float getValueOriginal() const noexcept;
    void setValueOriginal(float v, bool sendNotification = true);
    float getValueScaled() const noexcept;
    void setValueScaled(float v);
    
    void startEdition() noexcept;
    void stopEdition() noexcept;
    
    Box* box;
    
protected:
    const std::string stringGui = std::string("gui");
    const std::string stringMouse = std::string("mouse");
    
    PlugDataAudioProcessor& processor;
    pd::Gui gui;
    std::atomic<bool> edited;
    float value = 0;
    float min = 0;
    float max = 0;
    int width = 6;
    
    String sendSymbol;
    String receiveSymbol;
    
    String primaryColour = MainLook::highlightColour.toString();
    String secondaryColour = MainLook::firstBackground.toString();
    
    PdGuiLook guiLook;
};

struct BangComponent : public GUIComponent, public Timer {
    
    int bangInterrupt = 40;
    
    TextButton bangButton;
    
    BangComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
    ObjectParameters defineParamters() override
    {
        return { {
            { "Interrupt", tInt, (void*)&bangInterrupt },
        },
            [this](int) {
                ((t_bng*)gui.getPointer())->x_flashtime_hold = bangInterrupt;
            } };
    }
    
    void initParameters() override {
        GUIComponent::initParameters();
        bangInterrupt = ((t_bng*)gui.getPointer())->x_flashtime_hold;
    };
    
    void update() override;
    
    void resized() override;
    
    void timerCallback() override
    {
        bangButton.setToggleState(false, dontSendNotification);
        stopTimer();
    };
};

struct ToggleComponent : public GUIComponent {
    
    TextButton toggleButton;
    
    ToggleComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
    void resized() override;
    
    void update() override;
};

struct MessageComponent : public GUIComponent, public ChangeListener {
    
    
    bool isDown = false;
    bool isLocked = false;
    
    TextEditor input;
    TextButton bangButton;
    
    std::string lastMessage = "";
    
    MessageComponent(pd::Gui gui, Box* parent);
    
    ~MessageComponent();
    
    std::pair<int, int> getBestSize() override
    {
        updateValue(); // make sure text is loaded
        
        //auto [x, y, w, h] = gui.getBounds();
        int stringLength = std::max(10, input.getFont().getStringWidth(input.getText()));
        return { stringLength + 20, numLines * 21 };
    };
    
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    void mouseDown(const MouseEvent& e) override {
        if(!gui.isAtom()) {
            isDown = true;
        }
        
        startEdition();
        gui.click();
        stopEdition();
    }
    
    
    void mouseUp(const MouseEvent& e) override {
        isDown = false;
    }
    
    void updateValue() override;
    
    void resized() override;
    
    void update() override;
    void paint(Graphics& g) override;
    
    int numLines = 1;
    int longestLine = 7;
};

struct NumboxComponent : public GUIComponent {
    
    Label input;
    
    float last = 0.0f;
    bool shift = false;
    
    NumboxComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override { return { 60, 22 }; };
    
    void mouseDown(const MouseEvent& event) override
    {
        if(!input.isBeingEdited())
        {
            startEdition();
            shift = event.mods.isShiftDown();
            last  = input.getText().getFloatValue();
            //setValueOriginal(last);
        }
    }
    
    void mouseUp(const MouseEvent& event) override
    {
        if(!input.isBeingEdited()){
            stopEdition();
        }
    }
    
    void mouseDrag(const MouseEvent& e) override
    {
        
        input.mouseDrag(e);
        
        if(!input.isBeingEdited())
        {
            auto const inc = static_cast<float>(-e.getDistanceFromDragStartY()) * 0.5f;
            if(std::abs(inc) < 1.0f) return;

            // Logic for dragging, where the x position decides the precision
            auto currentValue = input.getText();
            if(!currentValue.containsChar('.')) currentValue += '.';
            if(currentValue.getCharPointer()[0] == '-') currentValue = currentValue.substring(1);
            currentValue += "00000";
            
            // Get position of all numbers
            Array<int> glyphs;
            Array<float> xOffsets;
            input.getFont().getGlyphPositions(currentValue, glyphs, xOffsets);
            
            // Find the number closest to the mouse down
            float position = gui.isAtom() ? e.getMouseDownX() - 4 : e.getMouseDownX() - 15;
            int precision = std::lower_bound(xOffsets.begin(), xOffsets.end(), position) - xOffsets.begin();
            precision -= currentValue.indexOfChar('.');
            
            // I case of integer dragging
            if(shift || precision <= 0) {
                precision = 0;
            }
            else {
                // Offset for the decimal point character
                precision -= 1;
            }
            
            // Calculate increment multiplier
            float multiplier = pow(10, -precision);
            
            // Calculate new value as string
            auto newValue = String(std::clamp(last + inc * multiplier, min, max), precision);
            
            if(precision == 0) newValue = newValue.upToFirstOccurrenceOf(".", true, false);
            
            setValueOriginal(newValue.getFloatValue());
            input.setText(newValue, NotificationType::dontSendNotification);
            
        }
    }
    
    ObjectParameters defineParamters() override
    {
        auto callback = [this](int changedParameter) {
            if (changedParameter == 0) {
                gui.setMinimum(min);
                updateValue();
            }
            if (changedParameter == 1) {
                gui.setMaximum(max);
                updateValue();
            }
        };

        return { { { "Minimum", tFloat, (void*)&min },
                     { "Maximum", tFloat, (void*)&max } },
            callback };
    }
    
    void paint(Graphics& g) override {
        g.setColour(findColour(TextEditor::backgroundColourId));
        g.fillRect(getLocalBounds().reduced(1));
    }
    
    void paintOverChildren(Graphics& g) override {
        GUIComponent::paintOverChildren(g);
        
        if(!gui.isAtom()) {

            g.setColour(Colour(gui.getForegroundColor()));
            
            Path triangle;
            triangle.addTriangle({0.0f, 0.0f}, {10, getHeight() / 2.0f}, {0.0f, (float)getHeight()});
            
            g.fillPath(triangle);
        }
        
    }
    void resized() override;
    
    void update() override;
};

struct ListComponent : public GUIComponent {
    
    ListComponent(pd::Gui gui, Box* parent);
    void paint(juce::Graphics& g) override;
    void update() override;
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
private:
    Label label;
};

struct SliderComponent : public GUIComponent {
    
    bool isVertical;
    bool isLogarithmic;
    
    Slider slider;
    
    SliderComponent(bool vertical, pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };
    
    ObjectParameters defineParamters() override
    {
        
        auto callback = [this](int changedParameter) {
            if (changedParameter == 0) {
                gui.setMinimum(min);
            } else if (changedParameter == 1) {
                gui.setMaximum(max);
            } else if (changedParameter == 2) {
                gui.setLogScale(isLogarithmic);
                min = gui.getMinimum();
                max = gui.getMaximum();
            }
        };
        
        return { {{ "Minimum",     tFloat, (void*)&min },
            { "Maximum",     tFloat, (void*)&max },
            { "Logarithmic", tBool,  (void*)&isLogarithmic },
        },
            callback };
    }
    
    void resized() override;
    
    void update() override;
};

struct RadioComponent : public GUIComponent {
    
    int last_state = 0;
    
    int minimum = 0, maximum = 8;
    
    bool isVertical;
    RadioComponent(bool vertical, pd::Gui gui, Box* parent);
    
    OwnedArray<TextButton> radioButtons;
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return {w, h};
    };
    
    ObjectParameters defineParamters() override
    {
        
        auto callback = [this](int changedParameter) {
            if (changedParameter == 0) {
                gui.setMinimum(minimum);
                updateRange();
            } else if (changedParameter == 1) {
                gui.setMaximum(maximum);
                updateRange();
            }
        };
        
        return { {
            { "Minimum", tInt, (void*)&minimum },
            { "Maximum", tInt, (void*)&maximum },
        },
            callback };
    }
    
    void resized() override;
    
    void update() override;
    
    void updateRange();
};

struct GraphicalArray : public Component, public Timer {
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

struct ArrayComponent : public GUIComponent {
public:
    ArrayComponent(pd::Gui gui, Box* box);
    void paint(Graphics&) override { }
    void resized() override;
    void updateValue() override { }
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
private:
    pd::Array graph;
    GraphicalArray array;
};

struct GraphOnParent : public GUIComponent {
public:
    GraphOnParent(pd::Gui gui, Box* box);
    ~GraphOnParent();
    
    void paint(Graphics& g) override;
    void resized() override;
    void updateValue() override;
    
    int bestW = 410;
    int bestH = 270;
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
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

struct Subpatch : public GUIComponent {
    
    Subpatch(pd::Gui gui, Box* box);
    
    ~Subpatch();
    
    std::pair<int, int> getBestSize() override { return { 0, 3 }; };
    
    void resized() override {};
    void updateValue() override;
    
    pd::Patch* getPatch() override
    {
        return &subpatch;
    }
    
    bool fakeGUI() override
    {
        return true;
    }
    
private:
    pd::Patch subpatch;
};

struct CommentComponent : public GUIComponent {
    CommentComponent(pd::Gui gui, Box* box);
    void paint(Graphics& g) override;
    
    void updateValue() override {};
    
    void resized() override {
        gui.setSize(getWidth(), getHeight());
    }
    
    std::pair<int, int> getBestSize() override { return { 120, 4 }; };
    
    bool fakeGUI() override
    {
        return true;
    }
};

struct VUMeter : public GUIComponent {
    
    VUMeter(pd::Gui gui, Box* box) : GUIComponent(gui, box) {
        lnf.setColour(foleys::LevelMeter::lmTextColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTextClipColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTextDeactiveColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmTicksColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmOutlineColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmBackgroundColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmBackgroundClipColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmMeterForegroundColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterOutlineColour, juce::Colours::transparentBlack);
        lnf.setColour(foleys::LevelMeter::lmMeterBackgroundColour, juce::Colours::darkgrey);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientMidColour, MainLook::highlightColour);
        lnf.setColour(foleys::LevelMeter::lmMeterGradientMaxColour, juce::Colours::red);
        
        
        meter.setLookAndFeel(&lnf);
        addAndMakeVisible(meter);
        
        meter.setSelectedChannel(0);
        
        source.resize(1, 3);
        meter.setMeterSource(&source);
    }
    
    ~VUMeter(){
        meter.setLookAndFeel(nullptr);
    }
    
    void resized() override {
        meter.setBounds(getLocalBounds().removeFromTop(getHeight() - 20));
    }
    
    void paint(Graphics& g) override {
        
        g.setColour(Colours::white);
        
        auto rms = gui.getValue();
        g.drawFittedText(String(rms, 2) + " dB", Rectangle<int>(getLocalBounds().removeFromBottom(20)).reduced(2), Justification::centred, 1, 0.6f);
    }
    
    void updateValue() override {
        auto rms = gui.getValue();
        auto peak = gui.getPeak();
        
        source.pushRMS(0, Decibels::decibelsToGain(rms), Decibels::decibelsToGain(peak));
    };
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
    foleys::LevelMeter meter = foleys::LevelMeter(foleys::LevelMeter::Minimal);
    foleys::LevelMeterSource source;
    foleys::LevelMeterLookAndFeel lnf;
};

struct PanelComponent : public GUIComponent {
    
    PanelComponent(pd::Gui gui, Box* box);
    
    void paint(Graphics& g) override {
        g.fillAll(Colour::fromString(secondaryColour));
    }
    
    void resized() override {
        gui.setSize(getWidth(), getHeight());
    }
    
    void updateValue() override {};
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
    
    
};

// ELSE mousepad
struct MousePad : public GUIComponent {
    
    typedef struct _pad {
        t_object x_obj;
        t_glist* x_glist;
        void* x_proxy; // dont have this object and dont need it
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
    
    MousePad(pd::Gui gui, Box* box);
    ~MousePad();
    void paint(Graphics& g) override;
    
    void updateValue() override;
    
    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w, h };
    };
};

// Else "mouse" component
struct MouseComponent : public GUIComponent {
    
    typedef struct _mouse {
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
    
    MouseComponent(pd::Gui gui, Box* box);
    
    ~MouseComponent();
    
    std::pair<int, int> getBestSize() override { return { 0, 3 }; };
    
    void resized() override {};
    void updateValue() override;
    
    bool fakeGUI() override
    {
        return true;
    }
    
    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
};

// ELSE keyboard
struct KeyboardComponent : public GUIComponent, public MidiKeyboardStateListener {
    
    typedef struct _edit_proxy {
        t_object p_obj;
        t_symbol* p_sym;
        t_clock* p_clock;
        struct _keyboard* p_cnv;
    } t_edit_proxy;
    
    typedef struct _keyboard {
        t_object x_obj;
        t_glist* x_glist;
        t_edit_proxy* x_proxy;
        int* x_tgl_notes; // to store which notes should be played
        int x_velocity; // to store velocity
        int x_last_note; // to store last note
        float x_vel_in; // to store the second inlet values
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
    
    KeyboardComponent(pd::Gui gui, Box* box);
    
    void paint(Graphics& g) override {};
    
    void updateValue() override;
    
    void resized() override;
    
    void handleNoteOn(MidiKeyboardState* source,
                      int midiChannel, int midiNoteNumber, float velocity) override;
    
    void handleNoteOff(MidiKeyboardState* source,
                       int midiChannel, int midiNoteNumber, float velocity) override;
    
    std::pair<int, int> getBestSize() override
    {
        auto [x, y, w, h] = gui.getBounds();
        return { w - 28, h };
    };
    
    MidiKeyboardState state;
    MidiKeyboardComponent keyboard;
};

struct _fielddesc {
    char fd_type; /* LATER consider removing this? */
    char fd_var;
    union {
        t_float fd_float; /* the field is a constant float */
        t_symbol* fd_symbol; /* the field is a constant symbol */
        t_symbol* fd_varsym; /* the field is variable and this is the name */
    } fd_un;
    float fd_v1; /* min and max values */
    float fd_v2;
    float fd_screen1; /* min and max screen values */
    float fd_screen2;
    float fd_quantum; /* quantization in value */
};

// TODO: Pd template class for drawing (using "drawcurve", "drawpolygon", etc)
struct TemplateDraw {
    struct t_curve {
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
