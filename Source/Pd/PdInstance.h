/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

#include <map>
#include <utility>

extern "C" {
#include <z_libpd.h>
#include "s_libpd_inter.h"
}

#include "PdPatch.h"
#include "concurrentqueue.h"
#include "../Utility/FastStringWidth.h"


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

    // The float constructor.
    inline Atom(float const val)
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

struct ContinuityChecker : public Timer {

    struct BackupTimer : public HighResolutionTimer {
        t_pdinstance* pd;

        std::atomic<bool>& hasTicked;

        std::vector<t_float> emptyInBuffer;
        std::vector<t_float> emptyOutBuffer;

        std::function<void(t_float*, t_float*)> callback;

        int numBlocksPerCallback;
        int intervalMs;

        BackupTimer(std::atomic<bool>& ticked)
            : hasTicked(ticked)
        {
        }

        void prepare(int samplesPerBlock, int schedulerInterval, int numChannels)
        {
            pd = pd_this;

            numBlocksPerCallback = samplesPerBlock / libpd_blocksize();
            intervalMs = schedulerInterval;

            emptyInBuffer.resize(numChannels * samplesPerBlock);
            emptyOutBuffer.resize(numChannels * samplesPerBlock);
        }

        void startScheduler()
        {
            if (isTimerRunning())
                return;

                // startTimer(intervalMs);
#if JUCE_DEBUG
                // std::cout << "backup scheduler started" << std::endl;
#endif
        }

        void stopScheduler()
        {
            if (!isTimerRunning())
                return;

                // stopTimer();

#if JUCE_DEBUG
                // std::cout << "backup scheduler stopped" << std::endl;
#endif
        }

        void hiResTimerCallback() override
        {
            if (hasTicked) {
                stopScheduler();
                return;
            }

            for (int i = 0; i < numBlocksPerCallback; i++) {
                std::fill(emptyInBuffer.begin(), emptyInBuffer.end(), 0.0f);

                callback(emptyInBuffer.data(), emptyOutBuffer.data());

                std::fill(emptyOutBuffer.begin(), emptyOutBuffer.end(), 0.0f);
            }
        }
    };

    ContinuityChecker()
        : backupTimer(hasTicked) {};

    void setCallback(std::function<void(t_float*, t_float*)> cb)
    {
        backupTimer.callback = std::move(cb);
    }

    void prepare(double sampleRate, int samplesPerBlock, int numChannels)
    {
        timePerBlock = std::round((samplesPerBlock / sampleRate) * 1000.0);

        backupTimer.prepare(samplesPerBlock, timePerBlock, numChannels);

        startTimer(timePerBlock);
    }

    void setTimer()
    {
        lastTime = Time::getCurrentTime().getMillisecondCounterHiRes();
        hasTicked = true;
    }

    void setNonRealtime(bool nonRealtime)
    {
        isNonRealtime = nonRealtime;
        if (isNonRealtime)
            backupTimer.stopScheduler();
    }

    void timerCallback() override
    {
        int timePassed = Time::getCurrentTime().getMillisecondCounterHiRes() - lastTime;

        // Scheduler
        if (timePassed > 2 * timePerBlock && !hasTicked && !isNonRealtime) {
            backupTimer.startScheduler();
        }

        hasTicked = false;
    }

    t_pdinstance* pd;

    std::atomic<double> lastTime;
    std::atomic<bool> hasTicked;

    std::atomic<bool> isNonRealtime = false;
    int timePerBlock;

    BackupTimer backupTimer;
};

class Instance {
    struct Message {
        String selector;
        String destination;
        std::vector<pd::Atom> list;
    };

    struct dmessage {
        void* object;
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

    void prepareDSP(int const nins, int const nouts, double const samplerate, int const blockSize);
    void startDSP();
    void releaseDSP();
    void performDSP(float const* inputs, float* outputs);
    int getBlockSize() const;

    void sendNoteOn(int const channel, int const pitch, int const velocity) const;
    void sendControlChange(int const channel, int const controller, int const value) const;
    void sendProgramChange(int const channel, int const value) const;
    void sendPitchBend(int const channel, int const value) const;
    void sendAfterTouch(int const channel, int const value) const;
    void sendPolyAfterTouch(int const channel, int const pitch, int const value) const;
    void sendSysEx(int const port, int const byte) const;
    void sendSysRealTime(int const port, int const byte) const;
    void sendMidiByte(int const port, int const byte) const;

    virtual void receiveNoteOn(int const channel, int const pitch, int const velocity)
    {
    }
    virtual void receiveControlChange(int const channel, int const controller, int const value)
    {
    }
    virtual void receiveProgramChange(int const channel, int const value)
    {
    }
    virtual void receivePitchBend(int const channel, int const value)
    {
    }
    virtual void receiveAftertouch(int const channel, int const value)
    {
    }
    virtual void receivePolyAftertouch(int const channel, int const pitch, int const value)
    {
    }
    virtual void receiveMidiByte(int const port, int const byte)
    {
    }

    virtual void receiveGuiUpdate(int type) {};
    virtual void synchroniseCanvas(void* cnv) {};

    virtual void createPanel(int type, char const* snd, char const* location);

    void sendBang(char const* receiver) const;
    void sendFloat(char const* receiver, float const value) const;
    void sendSymbol(char const* receiver, char const* symbol) const;
    void sendList(char const* receiver, std::vector<pd::Atom> const& list) const;
    void sendMessage(char const* receiver, char const* msg, std::vector<pd::Atom> const& list) const;

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


    virtual void receiveDSPState(bool dsp) {};

    virtual void updateConsole() {};

    virtual void titleChanged() {};

    void enqueueFunction(std::function<void(void)> const& fn);
    void enqueueFunctionAsync(std::function<void(void)> const& fn);

    void enqueueMessages(String const& dest, String const& msg, std::vector<pd::Atom>&& list);

    void enqueueDirectMessages(void* object, std::vector<pd::Atom> const& list);
    void enqueueDirectMessages(void* object, String const& msg);
    void enqueueDirectMessages(void* object, float const msg);
    
    virtual void performParameterChange(int type, int idx, float value) {};

    void logMessage(String const& message);
    void logError(String const& message);
    
    std::deque<std::tuple<String, int, int>>& getConsoleMessages();
    std::deque<std::tuple<String, int, int>>& getConsoleHistory();
    
    virtual void messageEnqueued() {};

    void sendMessagesFromQueue();
    void processMessage(Message mess);
    void processMidiEvent(midievent event);
    void processSend(dmessage mess);
    
    String getExtraInfo(File const& toOpen);
    Patch openPatch(File const& toOpen);

    virtual Colour getForegroundColour() = 0;
    virtual Colour getBackgroundColour() = 0;
    virtual Colour getTextColour() = 0;
    virtual Colour getOutlineColour() = 0;

    void setThis();

    void waitForStateUpdate();

    virtual CriticalSection const* getCallbackLock()
    {
        return nullptr;
    };

    void* m_instance = nullptr;
    void* m_patch = nullptr;
    void* m_atoms = nullptr;
    void* m_message_receiver = nullptr;
    void* m_parameter_receiver = nullptr;
    void* m_parameter_change_receiver = nullptr;
    void* m_midi_receiver = nullptr;
    void* m_print_receiver = nullptr;

    std::atomic<bool> canUndo = false;
    std::atomic<bool> canRedo = false;

    inline static const String defaultPatch = "#N canvas 827 239 527 327 12;";

private:
    moodycamel::ConcurrentQueue<std::function<void(void)>> m_function_queue = moodycamel::ConcurrentQueue<std::function<void(void)>>(4096);
    

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    WaitableEvent updateWait;
    
protected:
    ContinuityChecker continuityChecker;
    
    moodycamel::ConcurrentQueue<std::tuple<int, int, float>> m_parameter_queue = moodycamel::ConcurrentQueue<std::tuple<int, int, float>>();
    
    struct internal;
    
    struct ConsoleHandler : public Timer
    {
        Instance* instance;
        
        ConsoleHandler(Instance* parent) : instance(parent), fastStringWidth(Font(14))
        {
            
        }
        
        void timerCallback() override
        {
            auto item = std::pair<String, bool>();
            while(pendingMessages.try_dequeue(item)) {
                auto& [message, type] = item;
                consoleMessages.emplace_back(message, type, fastStringWidth.getStringWidth(message) + 12);

                if (consoleMessages.size() > 800)
                    consoleMessages.pop_front();
            }
            
            // Check if any item got assigned
            if(std::get<0>(item).isNotEmpty()) {
                instance->updateConsole();
            }
            
            stopTimer();
        }
        
        void logMessage(String const& message)
        {
            pendingMessages.enqueue({message, false});
            startTimer(10);
        }

        void logError(String const& error)
        {
            pendingMessages.enqueue({error, 1});
            startTimer(10);
        }
        
        void processPrint(const char* message)
        {
            std::function<void(String)> forwardMessage =
            [this](String message) {
                if (message.startsWith("error:"))
                {
                    
                    logError(message.substring(7));
                }
                else if (message.startsWith("verbose(4):"))
                {
                    logError(message.substring(12));
                }
                else
                {
                    logMessage(message);
                }
            };
            
            static int length = 0;
            printConcatBuffer[length] = '\0';

            int len = (int)strlen(message);
            while (length + len >= 2048) {
                int d = 2048 - 1 - length;
                strncat(printConcatBuffer, message, d);

                // Send concatenated line to PlugData!
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

                // Send concatenated line to PlugData!
                forwardMessage(String::fromUTF8(printConcatBuffer));

                length = 0;
            }
        }

        
        std::deque<std::tuple<String, int, int>> consoleMessages;
        std::deque<std::tuple<String, int, int>> consoleHistory;

        char printConcatBuffer[2048];
        
        moodycamel::ConcurrentQueue<std::pair<String, bool>> pendingMessages;
        
        FastStringWidth fastStringWidth; // For formatting console messages more quickly
    };
    
    ConsoleHandler consoleHandler;
};
} // namespace pd
