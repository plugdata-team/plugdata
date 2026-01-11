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
#include <readerwriterqueue.h>
#include "Patch.h"

class ObjectImplementationManager;

namespace pd {
class ConsoleMessageHandler;

class Atom {
public:
    // The default constructor.
    Atom()
        : type(FLOAT)
        , value(0)
    {
    }

    static SmallArray<pd::Atom, 8> atomsFromString(String const& str)
    {
        auto* binbuf = binbuf_new();
        binbuf_text(binbuf, str.toRawUTF8(), str.getNumBytesAsUTF8());
        auto const* argv = binbuf_getvec(binbuf);
        auto const argc = binbuf_getnatom(binbuf);

        auto atoms = fromAtoms(argc, argv);
        binbuf_free(binbuf);

        return atoms;
    }

    static SmallArray<pd::Atom, 8> fromAtoms(int const ac, t_atom const* av)
    {
        auto array = SmallArray<pd::Atom, 8>();
        array.reserve(ac);

        for (int i = 0; i < ac; ++i) {
            if (av[i].a_type == A_FLOAT) {
                array.emplace_back(atom_getfloat(av + i));
            } else if (av[i].a_type == A_SYMBOL) {
                array.emplace_back(atom_getsymbol(av + i));
            } else {
                array.emplace_back();
            }
        }

        return array;
    }

    // The float constructor.
    Atom(float const val)
        : type(FLOAT)
        , value(val)
    {
    }

    Atom(t_symbol* sym)
        : type(SYMBOL)
        , symbol(sym)
    {
    }

    Atom(t_atom const* atom)
    {
        if (atom->a_type == A_FLOAT) {
            type = FLOAT;
            value = atom->a_w.w_float;
        } else if (atom->a_type == A_SYMBOL) {
            type = SYMBOL;
            symbol = atom->a_w.w_symbol;
        }
    }

    // Check if the atom is a float.
    bool isFloat() const
    {
        return type == FLOAT;
    }

    // Check if the atom is a string.
    bool isSymbol() const
    {
        return type == SYMBOL;
    }

    // Get the float value.
    float getFloat() const
    {
        // jassert(isFloat());
        return value;
    }

    // Get the string.
    t_symbol* getSymbol() const
    {
        jassert(isSymbol());

        return symbol;
    }

    // Get the string.
    String toString() const
    {
        if (type == FLOAT) {
            return String(value);
        }
        return String::fromUTF8(symbol->s_name);
    }

    SmallString toSmallString() const
    {
        if (type == FLOAT) {
            return SmallString(value);
        }
        return SmallString(symbol->s_name);
    }

    // Compare two atoms.
    bool operator==(Atom const& other) const
    {
        if (type == SYMBOL) {
            return other.type == SYMBOL && symbol == other.symbol;
        }
        return other.type == FLOAT && value == other.value;
    }

private:
    enum Type {
        FLOAT,
        SYMBOL
    };

    Type type;
    union {
        float value;
        t_symbol* symbol;
    };
};

class MessageListener;
class MessageDispatcher;
class Patch;
class Instance : public AsyncUpdater {
    struct Message {
        SmallString selector;
        SmallString destination;
        SmallArray<pd::Atom> list;
    };
    struct dmessage;

public:
    explicit Instance();
    Instance(Instance const& other) = delete;
    ~Instance() override;

    void initialisePd(String& pdlua_version);
    void prepareDSP(int nins, int nouts, double samplerate);
    void startDSP();
    void releaseDSP();
    void performDSP(float const* inputs, float* outputs);
    static int getBlockSize();

    void handleAsyncUpdate() override;

    void sendNoteOn(int channel, int pitch, int velocity) const;
    void sendControlChange(int channel, int controller, int value) const;
    void sendProgramChange(int channel, int value) const;
    void sendPitchBend(int channel, int value) const;
    void sendAfterTouch(int channel, int value) const;
    void sendPolyAfterTouch(int channel, int pitch, int value) const;
    void sendSysEx(int port, int byte) const;
    void sendSysRealTime(int port, int byte) const;
    void sendMidiByte(int port, int byte) const;

    virtual void receiveNoteOn(int channel, int pitch, int velocity) = 0;
    virtual void receiveControlChange(int channel, int controller, int value) = 0;
    virtual void receiveProgramChange(int channel, int value) = 0;
    virtual void receivePitchBend(int channel, int value) = 0;
    virtual void receiveAftertouch(int channel, int value) = 0;
    virtual void receivePolyAftertouch(int channel, int pitch, int value) = 0;
    virtual void receiveMidiByte(int port, int byte) = 0;

    virtual void createPanel(int type, char const* snd, char const* location, char const* callbackName, int openMode = -1);

    void sendBang(char const* receiver) const;
    void sendFloat(char const* receiver, float value) const;
    void sendSymbol(char const* receiver, char const* symbol) const;
    void sendList(char const* receiver, SmallArray<pd::Atom> const& list) const;
    void sendMessage(char const* receiver, char const* msg, SmallArray<pd::Atom> const& list) const;
    void sendTypedMessage(void* object, char const* msg, SmallArray<Atom> const& list) const;

    virtual void addTextToTextEditor(uint64_t ptr, SmallString const& text) = 0;
    virtual void hideTextEditorDialog(uint64_t ptr) = 0;
    virtual void raiseTextEditorDialog(uint64_t ptr) = 0;
    virtual void showTextEditorDialog(uint64_t ptr, SmallString const& title, std::function<void(String, uint64_t)> save, std::function<void(uint64_t)> close) = 0;
    virtual void clearTextEditor(uint64_t ptr) = 0;
    virtual bool isTextEditorDialogShown(uint64_t ptr) = 0;

    virtual void receiveSysMessage(SmallString const& selector, SmallArray<pd::Atom> const& list) = 0;

    void registerMessageListener(void* object, MessageListener* messageListener);
    void unregisterMessageListener(MessageListener* messageListener);

    void registerWeakReference(void* ptr, pd_weak_reference* ref);
    void unregisterWeakReference(void* ptr, pd_weak_reference const* ref);
    void clearWeakReferences(void* ptr);

    static void registerLuaClass(char const* object);
    static bool isLuaClass(hash32 objectNameHash);

    virtual void updateConsole(int numMessages, bool newWarning) = 0;

    virtual void titleChanged() = 0;

    void enqueueFunctionAsync(std::function<void()> const& fn);

    void enqueueGuiMessage(Message const& fn);

    // Enqueue a message to an pd::WeakReference
    // This will first check if the weakreference is valid before triggering the callback
    template<typename T>
    void enqueueFunctionAsync(WeakReference& ref, std::function<void(T*)> const& fn)
    {
        functionQueue.enqueue([ref, fn] {
            if (auto obj = ref.get<T>()) {
                fn(obj.get());
            }
        });
    }

    void sendDirectMessage(void* object, SmallString const& msg, SmallArray<Atom>&& list);
    void sendDirectMessage(void* object, SmallArray<pd::Atom>&& list);
    void sendDirectMessage(void* object, SmallString const& msg);
    void sendDirectMessage(void* object, float msg);

    void updateObjectImplementations();
    void clearObjectImplementationsForPatch(pd::Patch const* p);

    virtual void handleParameterMessage(SmallArray<pd::Atom> const& atoms) = 0;
    virtual void performLatencyCompensationChange(float value) = 0;

    virtual void fillDataBuffer(SmallArray<pd::Atom> const& list) = 0;
    virtual void parseDataBuffer(XmlElement const& xml) = 0;

    void logMessage(String const& message);
    void logError(String const& message);
    void logWarning(String const& message);

    std::deque<std::tuple<void*, String, int, int, int>>& getConsoleMessages() const;
    std::deque<std::tuple<void*, String, int, int, int>>& getConsoleHistory() const;

    void sendMessagesFromQueue();
    void processSend(dmessage const& mess);

    Patch::Ptr openPatch(File const& toOpen);

    virtual void reloadAbstractions(File changedPatch, t_glist* except) = 0;

    void setThis() const;
    t_symbol* generateSymbol(String const& symbol) const;
    t_symbol* generateSymbol(SmallString const& symbol) const;
    t_symbol* generateSymbol(char const* symbol) const;

    void lockAudioThread();
    void unlockAudioThread();

    static bool loadLibrary(String const& library);

    void* instance = nullptr;
    void* messageReceiver = nullptr;
    void* parameterReceiver = nullptr;
    void* pluginLatencyReceiver = nullptr;
    void* midiReceiver = nullptr;
    void* printReceiver = nullptr;
    void* dataBufferReceiver = nullptr;

    inline static String const defaultPatch = "#N canvas 827 239 734 565 12;";

    bool initialiseIntoPluginmode = false;
    bool isPerformingGlobalSync = false;
    CriticalSection const audioLock;
    CriticalSection const weakReferenceLock;
    std::unique_ptr<pd::MessageDispatcher> messageDispatcher;

    // All opened patches
    SmallArray<pd::Patch::Ptr, 16> patches;

private:
    UnorderedMap<void*, SmallArray<pd_weak_reference*>> pdWeakReferences;

    moodycamel::ConcurrentQueue<std::function<void()>> functionQueue = moodycamel::ConcurrentQueue<std::function<void()>>(4096);
    moodycamel::ConcurrentQueue<Message> guiMessageQueue = moodycamel::ConcurrentQueue<Message>(64);

    std::unique_ptr<FileChooser> openChooser;
    static inline auto luaClasses = UnorderedSet<hash32>(); // Keep track of class names that correspond to pdlua objects

protected:
    struct internal;

    std::unique_ptr<ObjectImplementationManager> objectImplementations;
    std::unique_ptr<ConsoleMessageHandler> consoleMessageHandler;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Instance)
};
} // namespace pd
