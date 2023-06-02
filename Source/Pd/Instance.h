/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

extern "C" {
#include <z_libpd.h>
#include <s_inter.h>
}

#include <concurrentqueue.h>

#include "Utility/StringUtils.h"
#include "Patch.h"

class ObjectImplementationManager;

namespace pd {

class Atom {
public:
    // The default constructor.
    inline Atom()
        : type(FLOAT)
        , value(0)
        , symbol()
    {
    }

    static std::vector<pd::Atom> fromAtoms(int ac, t_atom* av)
    {
        auto array = std::vector<pd::Atom>();
        array.reserve(ac);

        for (int i = 0; i < ac; ++i) {
            if (av[i].a_type == A_FLOAT) {
                array.emplace_back(atom_getfloat(av + i));
            } else if (av[i].a_type == A_SYMBOL) {
                array.emplace_back(atom_getsymbol(av + i)->s_name);
            } else {
                array.emplace_back();
            }
        }

        return array;
    }

    // The const floatructor.
    inline Atom(float val)
        : type(FLOAT)
        , value(val)
        , symbol()
    {
    }

    // The string constructor.
    inline Atom(String sym)
        : type(SYMBOL)
        , value(0)
        , symbol(std::move(sym))
    {
    }

    // The pd hash constructor.
    inline Atom(t_symbol* sym)
        : type(SYMBOL)
        , value(0)
        , symbol(String::fromUTF8(sym->s_name))
    {
    }

    // The c-string constructor.
    inline Atom(char const* sym)
        : type(SYMBOL)
        , value(0)
        , symbol(String::fromUTF8(sym))
    {
    }

    // Check if the atom is a float.
    inline bool isFloat() const
    {
        return type == FLOAT;
    }

    // Check if the atom is a string.
    inline bool isSymbol() const
    {
        return type == SYMBOL;
    }

    // Get the float value.
    inline float getFloat() const
    {
        return value;
    }

    // Get the string.
    inline String const& getSymbol() const
    {
        return symbol;
    }

    // Compare two atoms.
    inline bool operator==(Atom const& other) const
    {
        if (type == SYMBOL) {
            return other.type == SYMBOL && symbol == other.symbol;
        } else {
            return other.type == FLOAT && value == other.value;
        }
    }

private:
    enum Type {
        FLOAT,
        SYMBOL
    };
    Type type = FLOAT;
    float value = 0;
    String symbol;
};

class MessageListener;
class Patch;
class Instance {
    struct Message {
        String selector;
        String destination;
        std::vector<pd::Atom> list;
    };

    struct dmessage {
        WeakReference object;
        String destination;
        String selector;
        std::vector<pd::Atom> list;
    };

    typedef struct midievent {
        enum {
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

public:
    Instance(String const& symbol);
    Instance(Instance const& other) = delete;
    virtual ~Instance();

    void initialisePd(String& pdlua_version);
    void prepareDSP(int nins, int nouts, double samplerate, int blockSize);
    void startDSP();
    void releaseDSP();
    void performDSP(float const* inputs, float* outputs);
    int getBlockSize() const;

    void sendNoteOn(int channel, int const pitch, int velocity) const;
    void sendControlChange(int channel, int const controller, int value) const;
    void sendProgramChange(int channel, int value) const;
    void sendPitchBend(int channel, int value) const;
    void sendAfterTouch(int channel, int value) const;
    void sendPolyAfterTouch(int channel, int const pitch, int value) const;
    void sendSysEx(int port, int byte) const;
    void sendSysRealTime(int port, int byte) const;
    void sendMidiByte(int port, int byte) const;

    virtual void receiveNoteOn(int channel, int pitch, int velocity)
    {
    }
    virtual void receiveControlChange(int channel, int controller, int value)
    {
    }
    virtual void receiveProgramChange(int channel, int value)
    {
    }
    virtual void receivePitchBend(int channel, int value)
    {
    }
    virtual void receiveAftertouch(int channel, int value)
    {
    }
    virtual void receivePolyAftertouch(int channel, int pitch, int value)
    {
    }
    virtual void receiveMidiByte(int port, int byte)
    {
    }

    virtual void updateDrawables() {};

    virtual void createPanel(int type, char const* snd, char const* location);

    void sendBang(char const* receiver) const;
    void sendFloat(char const* receiver, float value) const;
    void sendSymbol(char const* receiver, char const* symbol) const;
    void sendList(char const* receiver, std::vector<pd::Atom> const& list) const;
    void sendMessage(char const* receiver, char const* msg, std::vector<pd::Atom> const& list) const;
    void sendTypedMessage(void* object, char const* msg, std::vector<Atom> const& list) const;

    virtual void receivePrint(String const& message) {};

    virtual void receiveBang(String const& dest)
    {
    }
    virtual void receiveFloat(String const& dest, float num)
    {
    }
    virtual void receiveSymbol(String const& dest, String const& symbol)
    {
    }
    virtual void receiveList(String const& dest, std::vector<pd::Atom> const& list)
    {
    }
    virtual void receiveMessage(String const& dest, String const& msg, std::vector<pd::Atom> const& list)
    {
    }

    virtual void receiveSysMessage(String const& selector, std::vector<pd::Atom> const& list) {};

    void registerMessageListener(void* object, MessageListener* messageListener);
    void unregisterMessageListener(void* object, MessageListener* messageListener);
    
    void registerWeakReference(t_pd* ptr, pd_weak_reference* ref);
    void unregisterWeakReference(t_pd* ptr, pd_weak_reference* ref);
    void clearWeakReferences(t_pd* ptr);
    
    virtual void receiveDSPState(bool dsp) {};

    virtual void updateConsole() {};

    virtual void titleChanged() {};

    void enqueueFunctionAsync(std::function<void(void)> const& fn);

    void sendDirectMessage(void* object, String const& msg, std::vector<Atom>&& list);
    void sendDirectMessage(void* object, std::vector<pd::Atom>&& list);
    void sendDirectMessage(void* object, String const& msg);
    void sendDirectMessage(void* object, float msg);

    void updateObjectImplementations();
    void clearObjectImplementationsForPatch(pd::Patch* p);

    virtual void performParameterChange(int type, String const& name, float value) {};

    // JYG added this
    virtual void fillDataBuffer(std::vector<pd::Atom> const& list) {};
    virtual void parseDataBuffer(XmlElement const& xml) {};

    void logMessage(String const& message);
    void logError(String const& message);
    void logWarning(String const& message);

    std::deque<std::tuple<String, int, int>>& getConsoleMessages();
    std::deque<std::tuple<String, int, int>>& getConsoleHistory();

    virtual void messageEnqueued() {};

    void sendMessagesFromQueue();
    void processMessage(Message mess);
    void processMidiEvent(midievent event);
    void processSend(dmessage mess);

    String getExtraInfo(File const& toOpen);
    Patch::Ptr openPatch(File const& toOpen);

    virtual Colour getForegroundColour() = 0;
    virtual Colour getBackgroundColour() = 0;
    virtual Colour getTextColour() = 0;
    virtual Colour getOutlineColour() = 0;

    virtual void reloadAbstractions(File changedPatch, t_glist* except) = 0;

    void setThis() const;
    t_symbol* generateSymbol(String const& symbol) const;
    t_symbol* generateSymbol(char const* symbol) const;

    void lockAudioThread();
    bool tryLockAudioThread();
    void unlockAudioThread();

    bool loadLibrary(String const& library);

    void* m_instance = nullptr;
    void* m_patch = nullptr;
    void* m_atoms = nullptr;
    void* m_message_receiver = nullptr;
    void* m_parameter_receiver = nullptr;
    void* m_parameter_change_receiver = nullptr;
    void* m_midi_receiver = nullptr;
    void* m_print_receiver = nullptr;

    // JYG added this
    void* m_databuffer_receiver = nullptr;

    std::atomic<bool> canUndo = false;
    std::atomic<bool> canRedo = false;
    std::atomic<bool> waitingForStateUpdate = false;

    inline static const String defaultPatch = "#N canvas 827 239 527 327 12;";

    bool isPerformingGlobalSync = false;
    CriticalSection const audioLock;

private:
    
    std::unordered_map<t_pd*, std::vector<pd_weak_reference*>> pdWeakReferences;
    std::unordered_map<void*, std::vector<juce::WeakReference<MessageListener>>> messageListeners;

    std::unique_ptr<ObjectImplementationManager> objectImplementations;

    CriticalSection messageListenerLock;

    moodycamel::ConcurrentQueue<std::function<void(void)>> m_function_queue = moodycamel::ConcurrentQueue<std::function<void(void)>>(4096);

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
    
    std::atomic<int> numLocksHeld = 0;

    WaitableEvent updateWait;

protected:
    struct internal;

    struct ConsoleHandler : public Timer {
        Instance* instance;

        ConsoleHandler(Instance* parent)
            : instance(parent)
            , fastStringWidth(Font(14))
        {
        }

        void timerCallback() override
        {
            auto item = std::pair<String, bool>();
            bool receivedMessage = false;

            while (pendingMessages.try_dequeue(item)) {
                auto& [message, type] = item;
                consoleMessages.emplace_back(message, type, fastStringWidth.getStringWidth(message) + 8);

                if (consoleMessages.size() > 800)
                    consoleMessages.pop_front();

                receivedMessage = true;
            }

            // Check if any item got assigned
            if (receivedMessage) {
                instance->updateConsole();
            }

            stopTimer();
        }

        void logMessage(String const& message)
        {
            pendingMessages.enqueue({ message, false });
            startTimer(10);
        }

        void logWarning(String const& warning)
        {
            pendingMessages.enqueue({ warning, 1 });
            startTimer(10);
        }

        void logError(String const& error)
        {
            pendingMessages.enqueue({ error, 2 });
            startTimer(10);
        }

        void processPrint(char const* message)
        {
            std::function<void(const String)> forwardMessage =
                [this](String const& message) {
                    if (message.startsWith("error")) {
                        logError(message.substring(7));
                    } else if (message.startsWith("verbose(0):") || message.startsWith("verbose(1):")) {
                        logError(message.substring(12));
                    } else {
                        if (message.startsWith("verbose(")) {
                            logMessage(message.substring(12));
                        } else {
                            logMessage(message);
                        }
                    }
                };

            static int length = 0;
            printConcatBuffer[length] = '\0';

            int len = (int)strlen(message);
            while (length + len >= 2048) {
                int d = 2048 - 1 - length;
                strncat(printConcatBuffer, message, d);

                // Send concatenated line to plugdata!
                forwardMessage(String::fromUTF8(printConcatBuffer));

                message += d;
                len -= d;
                length = 0;
                printConcatBuffer[0] = '\0';
            }

            strncat(printConcatBuffer, message, len);
            length += len;

            if (length > 0 && printConcatBuffer[length - 1] == '\n') {
                printConcatBuffer[length - 1] = '\0';

                // Send concatenated line to plugdata!
                forwardMessage(String::fromUTF8(printConcatBuffer));

                length = 0;
            }
        }

        std::deque<std::tuple<String, int, int>> consoleMessages;
        std::deque<std::tuple<String, int, int>> consoleHistory;

        char printConcatBuffer[2048];

        moodycamel::ConcurrentQueue<std::pair<String, bool>> pendingMessages;

        StringUtils fastStringWidth; // For formatting console messages more quickly
    };

    ConsoleHandler consoleHandler;
};
} // namespace pd
