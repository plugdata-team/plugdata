#pragma once

#include <type_traits>
#include <utility>
#include <JuceHeader.h>
#include <m_pd.h>
#include "Pd/x_libpd_extra_utils.h"
#include "Pd/PdGui.hpp"
#include "PlugData.h"
#include "LookAndFeel.h"

class Canvas;
class Box;
struct GUIComponent : public Component
{
    
    
    
    std::unique_ptr<ResizableBorderComponent> resizer;
    
    Box* box;
    
    Array<int> num_registered;
    
    GUIComponent(pd::Gui gui, Box* parent);
    
    static bool is_gui(String name);
    
    virtual ~GUIComponent();
    
    virtual std::pair<int, int> getBestSize() = 0;
    
    virtual std::tuple<int, int, int, int> getSizeLimits() = 0;
    
    void paint(Graphics& g) override {
        g.setColour(findColour(TextButton::buttonColourId));
        g.fillRect(getLocalBounds().reduced(1));
    }
    
    void paintOverChildren(Graphics& g) override {
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawLine(0, 0, getWidth(), 0);
    }
    
    static GUIComponent* create_gui(String name, Box* parent);
    
    virtual void updateValue();
    
    virtual void update() {};
    
    virtual Canvas* getCanvas() {
        return nullptr;
    }
    
    std::unique_ptr<Label> getLabel();
    pd::Gui getGUI();
    
    float getValueOriginal() const noexcept;
    void setValueOriginal(float v, bool sendNotification = true);
    float getValueScaled() const noexcept;
    void setValueScaled(float v);
    
    void startEdition() noexcept;
    void stopEdition() noexcept;
    
protected:
    const std::string string_gui = std::string("gui");
    const std::string string_mouse = std::string("mouse");
    
    
    PlugData& m_processor;
    pd::Gui     gui;
    std::atomic<bool> edited;
    float       value   = 0;
    float       min     = 0;
    float       max     = 1;
    
    
    PdGuiLook banglook;
};

struct BangComponent : public GUIComponent
{
 
    // TODO: clean this up
    struct BangTimer : public Timer {
        
        BangTimer(TextButton* button) {
            bang_button = button;
        }
        
        void timerCallback() override {
            bang_button->setToggleState(false, dontSendNotification);
            stopTimer();
        };
        
        TextButton* bang_button;
    };
    
    TextButton bang_button;
    
    BangComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override {return {24, 46}; };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        return {40, 60, 200, 200};
    };
    
    void update() override;
    
    void resized() override;
    
    BangTimer bang_timer = BangTimer(&bang_button);
    
};

struct ToggleComponent : public GUIComponent
{
    
    TextButton toggle_button;
    
    ToggleComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override {return {24, 46}; };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        return {40, 60, 200, 200};
    };
    
    void resized() override;
    
    void update() override;
};


struct MessageComponent : public GUIComponent
{
    
    TextEditor input;
    TextButton bang_button;
    
    std::string last_message = "";
    
    MessageComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override {return {120, 28}; };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        return {100, 50, 500, 600};
    };
    
    void updateValue() override;
    
    void resized() override;

    void update() override;
};


struct NumboxComponent : public GUIComponent
{

    TextEditor input;
    
    NumboxComponent(pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override {return {70, 28}; };
    
    std::tuple<int, int, int, int> getSizeLimits() override {
        return {100, 50, 500, 600};
    };
    
    void mouseDrag(const MouseEvent & e) override {
        startEdition();
        
        input.mouseDrag(e);
        int dist = -e.getDistanceFromDragStartY();
        if(abs(dist) > 2) {
            float newval = input.getText().getFloatValue() + ((float)dist / 100.);
            input.setText(String(newval));
        }
        //onMouseDrag();
    }
    

    
    void resized() override;
    
    void update() override;

};

struct SliderComponent : public GUIComponent
{

    bool v_slider;
    
    Slider slider;
    
    SliderComponent(bool vertical, pd::Gui gui, Box* parent);
    
    std::pair<int, int> getBestSize() override {
        if(v_slider) return {35, 130};
        
        return {130, 35}; };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        return {100, 60, 500, 600};
    };

    void resized() override;

    void update() override;

};


struct RadioComponent : public GUIComponent
{
    
    int last_state = 0;
    
    bool v_radio;
    RadioComponent(bool vertical, pd::Gui gui, Box* parent);
    
    std::array<TextButton, 8> radio_buttons;
    
    std::pair<int, int> getBestSize() override {
        if(v_radio) return {24, 164};
        
        return {161, 24};
    };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        return {100, 40, 500, 600};
    };
    
    
    void resized() override;

    void update() override;
    
};

struct GraphicalArray : public Component, public Timer
{
public:
    GraphicalArray(PlugData* pd, pd::Array& graph);
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;
    size_t getArraySize() const noexcept;
private:
    
    void timerCallback() override;
    
    template <typename T> T clip(const T& n, const T& lower, const T& upper) {
        return std::max(std::min(n, upper), lower);
    }
    
    pd::Array               m_array;
    std::vector<float>      m_vector;
    std::vector<float>      m_temp;
    std::atomic<bool>       m_edited;
    bool                    m_error = false;
    const std::string string_array = std::string("array");
    
    PlugData* m_instance;
};


struct ArrayComponent : public GUIComponent
{
public:
    ArrayComponent(pd::Gui gui, Box* box);
    void paint(Graphics& ) override {}
    void resized() override;
    void updateValue() override {}
    
    
    std::pair<int, int> getBestSize() override {return {200, 140}; };
    
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        
        
        return {100, 40, 500, 600};
    };
    
private:
    pd::Array      m_graph;
    GraphicalArray m_array;
};

struct GraphOnParent : public GUIComponent
{
public:
    GraphOnParent(pd::Gui gui, Box* box);
    ~GraphOnParent();
    
    void paint(Graphics& g) override {
        g.fillAll(findColour(TextButton::buttonColourId));
        g.setColour(findColour(ComboBox::outlineColourId));
        g.drawRect(getLocalBounds(), 1);
    }
    void resized() override;
    void updateValue() override {}
    
    std::pair<int, int> getBestSize() override {return {200, 140}; };
    
    
    Canvas* getCanvas() override {
        return canvas;
    }
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        
        
        return {100, 80, 500, 600};
    };
    
private:
    pd::Patch subpatch;
    Canvas* canvas;
    
};



struct Subpatch : public GUIComponent
{

    Subpatch(pd::Gui gui, Box* box);
    
    ~Subpatch();
    
    std::pair<int, int> getBestSize() override {return {0, 5}; };
    
    void resized() override {};
    void updateValue() override {};
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        
    };
    
    
    Canvas* getCanvas() override {
        return canvas;
    }
    
    
private:
    pd::Patch subpatch;
    Canvas* canvas = nullptr;
    
};


class CommentComponent : public GUIComponent
{
public:
    CommentComponent(pd::Gui gui, Box* box);
    void paint(Graphics& g) override;
    
    void updateValue() override {};
    
    std::pair<int, int> getBestSize() override {return {120, 28}; };
    
    std::tuple<int, int, int, int> getSizeLimits()  override {
        
    };
};
