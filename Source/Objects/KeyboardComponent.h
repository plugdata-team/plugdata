
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
        int x_semitones;
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
    
    KeyboardComponent(const pd::Gui& gui, Box* box, bool newObject) : GUIComponent(gui, box, newObject), keyboard(state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setAvailableRange(36, 83);
        keyboard.setScrollButtonsVisible(false);
        
        state.addListener(this);
        
        addAndMakeVisible(keyboard);

        auto* x = (t_keyboard*)gui.getPointer();
        x->x_width = width * 0.595f;
        
        rangeMin = x->x_low_c;
        rangeMax = x->x_low_c + (x->x_octaves * 12) + x->x_semitones;
        
        if(static_cast<int>(rangeMin.getValue()) == 0 || static_cast<int>(rangeMax.getValue()) == 0) {
            rangeMin = 36;
            rangeMax = 86;
        }
        
        initialise(newObject);
    }
    
    void checkBoxBounds() override {
        int numKeys = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
        float ratio = numKeys / 9.55f;
        
        auto* keyboardObject = static_cast<t_keyboard*>(gui.getPointer());
        
        if(box->getWidth() / ratio != box->getHeight()) {
            box->setSize(box->getHeight() * ratio, box->getHeight());
            
            if(getWidth() > 0) {
                keyboard.setKeyWidth(getWidth() / (numKeys * 0.597f));
                keyboardObject->x_width = getWidth();
            }
        }
        
    }
    
    
    void resized() override
    {
        keyboard.setBounds(getLocalBounds());
    }
    
    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)gui.getPointer();
        
        box->cnv->pd->enqueueFunction(
                                      [x, note, velocity]() mutable
                                      {
                                          int ac = 2;
                                          t_atom at[2];
                                          SETFLOAT(at, note);
                                          SETFLOAT(at + 1, velocity * 127);
                                          
                                          outlet_list(x->x_out, &s_list, ac, at);
                                          if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
                                      });
    }
    
    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int note, float velocity) override
    {
        auto* x = (t_keyboard*)gui.getPointer();
        
        box->cnv->pd->enqueueFunction(
                                      [x, note]() mutable
                                      {
                                          int ac = 2;
                                          t_atom at[2];
                                          SETFLOAT(at, note);
                                          SETFLOAT(at + 1, 0);
                                          
                                          outlet_list(x->x_out, &s_list, ac, at);
                                          if (x->x_send != &s_ && x->x_send->s_thing) pd_list(x->x_send->s_thing, &s_list, ac, at);
                                      });
    };
    
    ObjectParameters defineParameters() override
    {
        return {{"Lowest note", tInt, cGeneral, &rangeMin, {}}, {"Highest note", tInt, cGeneral, &rangeMax, {}}};
    };
    
    
    void valueChanged(Value& value) override
    {
        auto* keyboardObject = static_cast<t_keyboard*>(gui.getPointer());
        
        if (value.refersToSameSourceAs(rangeMin))
        {
            rangeMin = std::clamp<int>(rangeMin.getValue(), 0, 127);
            
            keyboardObject->x_low_c = static_cast<int>(rangeMin.getValue());
            
            int range = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
            keyboardObject->x_octaves = range / 12;
            keyboardObject->x_semitones = range % 12;
            
            keyboard.setAvailableRange(rangeMin.getValue(), rangeMax.getValue());
            checkBoxBounds();
        }
        else if (value.refersToSameSourceAs(rangeMax))
        {
            
            rangeMax = std::clamp<int>(rangeMax.getValue(), 0, 127);
            /*
             static_cast<t_keyboard*>(gui.getPointer())->x_octaves = static_cast<int>(value.getValue()); */
            keyboard.setAvailableRange(rangeMin.getValue(), rangeMax.getValue());
            
            int range = static_cast<int>(rangeMax.getValue()) - static_cast<int>(rangeMin.getValue());
            keyboardObject->x_octaves = range / 12;
            keyboardObject->x_semitones = range % 12;
            checkBoxBounds();
        }
    }
    
    Value rangeMin;
    Value rangeMax;
    
    MidiKeyboardState state;
    MidiKeyboardComponent keyboard;
};
