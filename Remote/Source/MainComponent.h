#pragma once

#include <JuceHeader.h>

#include <z_libpd.h>
#include <x_libpd_mod_utils.h>
#include <x_libpd_extra_utils.h>
#include <m_imp.h>

#include "MessageHandler.h"

// False GATOM
typedef struct _fake_gatom {
    t_text a_text;
    int a_flavor;          /* A_FLOAT, A_SYMBOL, or A_LIST */
    t_glist* a_glist;      /* owning glist */
    t_float a_toggle;      /* value to toggle to */
    t_float a_draghi;      /* high end of drag range */
    t_float a_draglo;      /* low end of drag range */
    t_symbol* a_label;     /* symbol to show as label next to object */
    t_symbol* a_symfrom;   /* "receive" name -- bind ourselves to this */
    t_symbol* a_symto;     /* "send" name -- send to this on output */
    t_binbuf* a_revertbuf; /* binbuf to revert to if typing canceled */
    int a_dragindex;       /* index of atom being dragged */
    int a_fontsize;
    unsigned int a_shift : 1;         /* was shift key down when drag started? */
    unsigned int a_wherelabel : 2;    /* 0-3 for left, right, above, below */
    unsigned int a_grabbed : 1;       /* 1 if we've grabbed keyboard */
    unsigned int a_doubleclicked : 1; /* 1 if dragging from a double click */
    t_symbol* a_expanded_to;
} t_fake_gatom;

struct PointerWithId
{
    PointerWithId(void* destination) : ptr(destination)
    {
    }
    
    String getID() {
        return String(reinterpret_cast<intptr_t>(ptr));
    }
    
    template<typename T>
    T getPointer() {
        return static_cast<T>(ptr);
    }
    
private:
    
    void* ptr;
};

struct Object : public PointerWithId
{
    Object(void* ptr) : PointerWithId(ptr)
    {
    }
    
    String getName() {
        const String name = libpd_get_object_class_name(getPointer<void*>());
        
        if (name == "gatom") {
            auto* gatom = getPointer<t_fake_gatom*>();
            if (gatom->a_flavor == A_FLOAT)
                return "floatatom";
            else if (gatom->a_flavor == A_SYMBOL)
                return "symbolatom";
            else if (gatom->a_flavor == A_NULL)
                return "listatom";
        }
        
        else if (name == "canvas" || name == "graph") {
            auto* cnv = getPointer<t_canvas*>();
            if (cnv->gl_list) {
                t_class* c = cnv->gl_list->g_pd;
                if (c && c->c_name && (String::fromUTF8(c->c_name->s_name) == "array")) {
                    return "array";
                } else if (cnv->gl_isgraph) {
                    return "graph";
                } else { // abstraction or subpatch
                    return "subpatch";
                }
            } else if (cnv->gl_isgraph) {
                return "graph";
            } else {
                return "subpatch";
            }
        }
    }
};


struct Connection
{
    int in_idx;
    PointerWithId inlet;
    
    int out_idx;
    PointerWithId outlet;
    
    Connection(int output_idx, PointerWithId start, int input_idx, PointerWithId end) :
    out_idx(output_idx),
    outlet(start),
    in_idx(input_idx),
    inlet(end)
    {
        
    }
};

struct Patch : public PointerWithId
{
    Patch(void* ptr) : PointerWithId(ptr)
    {
    }
    
    void synchronise() {
        
        MemoryOutputStream sync_message;
        
        for(auto& object : objects) {
            sync_message.writeBool(false);
            sync_message.writeString(object.getID());
            sync_message.writeString(object.getName());
        }
        
        for(auto& connection : connections)
        {
            sync_message.writeBool(true);
            sync_message.writeInt(connection.out_idx);
            sync_message.writeString(connection.outlet.getID());
            sync_message.writeInt(connection.in_idx);
            sync_message.writeString(connection.inlet.getID());
        }
            
    }
    
    void update() {
        
        objects.clear();
        auto* cnv = getPointer<t_canvas*>();
        
        for (t_gobj* y = cnv->gl_list; y; y = y->g_next) {

            
            //if ((onlyGui && y->g_pd->c_gobj) || !onlyGui) {
                
            //}
            
            objects.push_back(Object(static_cast<void*>(y)));
        }
        
        
        connections.clear();

        t_linetraverser t;
        // Get connections from pd
        linetraverser_start(&t, cnv);

        // TODO: fix data race
        while (linetraverser_next(&t)) {
            connections.push_back(Connection(t.tr_inno, PointerWithId(t.tr_ob), t.tr_outno, PointerWithId(t.tr_ob2)));
        }
    }
    
    std::vector<Object> objects;
    std::vector<Connection> connections;
};

class PureData  : public juce::AudioSource
{
public:
    //==============================================================================
    PureData(String ID);
    ~PureData() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    
    void processInternal();
    
    
    void createObject(String cnv_id, String initialiser);

    void synchronise();

    void shutdownAudio();
    void setAudioChannels(int numInputChannels, int numOutputChannels, const XmlElement* const xml = nullptr);
    
private:
    
    AudioDeviceManager deviceManager;
    AudioSourcePlayer audioSourcePlayer;
    
    int audioAdvancement = 0;
    std::vector<float> audioBufferIn;
    std::vector<float> audioBufferOut;
    
    std::vector<float*> channelPointers;

    MidiBuffer midiBufferIn;
    MidiBuffer midiBufferOut;
    MidiBuffer midiBufferTemp;
    MidiBuffer midiBufferCopy;

    bool midiByteIsSysex = false;
    uint8 midiByteBuffer[512] = {0};
    size_t midiByteIndex = 0;

    int minIn = 2;
    int minOut = 2;
    
    MessageHandler message_handler;
    
    OwnedArray<Patch> patches;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PureData)
};
