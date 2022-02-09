/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>
#include <z_libpd.h>

#include <map>
#include <utility>

extern "C"
{
#include "s_libpd_inter.h"
}

#include "PdAtom.h"
#include "PdPatch.h"
#include "concurrentqueue.h"

namespace pd
{
class Patch;
// ==================================================================================== //
//                                      INSTANCE                                        //
// ==================================================================================== //

class Instance
{
   public:
    Instance(std::string const& symbol);
    Instance(Instance const& other) = delete;
    virtual ~Instance();

    void prepareDSP(const int nins, const int nouts, const double samplerate);
    void startDSP();
    void releaseDSP();
    void performDSP(float const* inputs, float* outputs);
    int getBlockSize() const noexcept;

    void sendNoteOn(const int channel, const int pitch, const int velocity) const;
    void sendControlChange(const int channel, const int controller, const int value) const;
    void sendProgramChange(const int channel, const int value) const;
    void sendPitchBend(const int channel, const int value) const;
    void sendAfterTouch(const int channel, const int value) const;
    void sendPolyAfterTouch(const int channel, const int pitch, const int value) const;
    void sendSysEx(const int port, const int byte) const;
    void sendSysRealTime(const int port, const int byte) const;
    void sendMidiByte(const int port, const int byte) const;

    virtual void receiveNoteOn(const int channel, const int pitch, const int velocity)
    {
    }
    virtual void receiveControlChange(const int channel, const int controller, const int value)
    {
    }
    virtual void receiveProgramChange(const int channel, const int value)
    {
    }
    virtual void receivePitchBend(const int channel, const int value)
    {
    }
    virtual void receiveAftertouch(const int channel, const int value)
    {
    }
    virtual void receivePolyAftertouch(const int channel, const int pitch, const int value)
    {
    }
    virtual void receiveMidiByte(const int port, const int byte)
    {
    }

    virtual void receiveGuiUpdate(int type){};
    virtual void createPanel(int type, const char* snd, const char* location);

    void sendBang(const char* receiver) const;
    void sendFloat(const char* receiver, float const value) const;
    void sendSymbol(const char* receiver, const char* symbol) const;
    void sendList(const char* receiver, const std::vector<Atom>& list) const;
    void sendMessage(const char* receiver, const char* msg, const std::vector<Atom>& list) const;

    virtual void receivePrint(const std::string& message){};

    virtual void receiveBang(const std::string& dest)
    {
    }
    virtual void receiveFloat(const std::string& dest, float num)
    {
    }
    virtual void receiveSymbol(const std::string& dest, const std::string& symbol)
    {
    }
    virtual void receiveList(const std::string& dest, const std::vector<Atom>& list)
    {
    }
    virtual void receiveMessage(const std::string& dest, const std::string& msg, const std::vector<Atom>& list)
    {
    }

    virtual void titleChanged(){};

    void enqueueFunction(const std::function<void(void)>& fn);
    void enqueueMessages(const std::string& dest, const std::string& msg, std::vector<Atom>&& list);

    void enqueueDirectMessages(void* object, std::vector<Atom> const& list);
    void enqueueDirectMessages(void* object, const std::string& msg);
    void enqueueDirectMessages(void* object, const float msg);

    void addListener(const char* sym);

    virtual void messageEnqueued(){};

    void sendMessagesFromQueue();
    void processMessages();
    void processPrints();
    void processMidi();
    
    std::atomic<uint64> lastCallbackTime;

    Patch openPatch(const File& toOpen);

    void savePatch(const File& location);
    void savePatch();

    File getCurrentFile()
    {
        return currentFile;
    }
    void setCurrentFile(File newFile)
    {
        currentFile = newFile;
    }

    bool isDirty();

    void closePatch();
    Patch getPatch();

    void setThis();
    Array getArray(std::string const& name);

    bool checkState(String pdstate);

    static Instance* getCurrent();

    void waitForStateUpdate();

    virtual const CriticalSection* getCallbackLock()
    {
        return nullptr;
    };

    void* m_instance = nullptr;
    void* m_patch = nullptr;
    void* m_atoms = nullptr;
    void* m_midi_receiver = nullptr;
    void* m_print_receiver = nullptr;
    std::vector<void*> m_message_receiver = std::vector<void*>(1, nullptr);

    moodycamel::ConcurrentQueue<std::function<void(void)>> m_function_queue = moodycamel::ConcurrentQueue<std::function<void(void)>>(4096);

    std::atomic<bool> audioStarted = false;
    std::atomic<bool> canUndo = false;
    std::atomic<bool> canRedo = false;
    std::recursive_mutex canvasLock;

    inline static const String defaultPatch = "#N canvas 827 239 527 327 12;";

   private:
    struct Message
    {
        std::string selector;
        std::string destination;
        std::vector<Atom> list;
    };

    struct dmessage
    {
        void* object;
        std::string destination;
        std::string selector;
        std::vector<Atom> list;
    };

    typedef struct midievent
    {
        enum
        {
            NOTEON,
            CONTROLCHANGE,
            PROGRAMCHANGE,
            PITCHBEND,
            AFTERTOUCH,
            POLYAFTERTOUCH,
            MIDIBYTE
        } type;
        int midi1;
        int midi2;
        int midi3;
    } midievent;

    typedef moodycamel::ConcurrentQueue<dmessage> message_queue;

    message_queue m_send_queue = message_queue(4096);

    moodycamel::ConcurrentQueue<Message> m_message_queue = moodycamel::ConcurrentQueue<Message>(4096);
    moodycamel::ConcurrentQueue<midievent> m_midi_queue = moodycamel::ConcurrentQueue<midievent>(4096);
    moodycamel::ConcurrentQueue<std::string> m_print_queue = moodycamel::ConcurrentQueue<std::string>(4096);

    File currentFile;

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    WaitableEvent updateWait;

    struct internal;
};
}  // namespace pd
