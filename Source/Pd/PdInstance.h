/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

#include <map>
#include <utility>

extern "C"
{
#include <z_libpd.h>
#include "s_libpd_inter.h"
}

#include "PdPatch.h"
#include "concurrentqueue.h"

extern int libpd_process_nodsp(void);

namespace pd
{

class Atom
{
   public:
    // The default constructor.
    inline Atom() : type(FLOAT), value(0), symbol()
    {
    }

    // The float constructor.
    inline Atom(const float val) : type(FLOAT), value(val), symbol()
    {
    }

    // The string constructor.
    inline Atom(String sym) : type(SYMBOL), value(0), symbol(std::move(sym))
    {
    }

    // The c-string constructor.
    inline Atom(const char* sym) : type(SYMBOL), value(0), symbol(sym)
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
        if (type == SYMBOL)
        {
            return other.type == SYMBOL && symbol == other.symbol;
        }
        else
        {
            return other.type == FLOAT && value == other.value;
        }
    }

   private:
    enum Type
    {
        FLOAT,
        SYMBOL
    };
    Type type = FLOAT;
    float value = 0;
    String symbol;
};

class Patch;

struct ContinuityChecker : public Timer
{
    
    struct BackupTimer : public HighResolutionTimer
    {
        t_pdinstance* pd;
        
        std::atomic<bool>& hasTicked;
        
        std::vector<t_float> emptyInBuffer;
        std::vector<t_float> emptyOutBuffer;
        
        std::function<void(t_float*, t_float*)> callback;
        
        int numBlocksPerCallback;
        int intervalMs;
        
        BackupTimer(std::atomic<bool>& ticked) : hasTicked(ticked) {
        }
        
        void prepare(int samplesPerBlock, int schedulerInterval, int numChannels) {
            pd = pd_this;
            
            numBlocksPerCallback = samplesPerBlock / libpd_blocksize();
            intervalMs = schedulerInterval;
            
            emptyInBuffer.resize(numChannels * samplesPerBlock);
            emptyOutBuffer.resize(numChannels * samplesPerBlock);
        }
        
        void startScheduler() {
            if(isTimerRunning()) return;

            
            startTimer(intervalMs);
#if JUCE_DEBUG
            std::cout <<  "backup scheduler started" << std::endl;
#endif
        }
        
        void stopScheduler() {
            if(!isTimerRunning()) return;
            
            stopTimer();
            
#if JUCE_DEBUG
            std::cout <<  "backup scheduler stopped" << std::endl;
#endif
        }
        
        void hiResTimerCallback() override {
            if(hasTicked) {
                stopScheduler();
                return;
            }
            
            for(int i = 0; i < numBlocksPerCallback; i++) {
                std::fill(emptyInBuffer.begin(), emptyInBuffer.end(), 0.0f);
                
                callback(emptyInBuffer.data(), emptyOutBuffer.data());
                
                std::fill(emptyOutBuffer.begin(), emptyOutBuffer.end(), 0.0f);
            }
        }
    };
    
    ContinuityChecker() : backupTimer(hasTicked) {};
    
    void setCallback(std::function<void(t_float*, t_float*)> cb) {
        backupTimer.callback = std::move(cb);
    }
    
    void prepare(double sampleRate, int samplesPerBlock, int numChannels) {
        timePerBlock = std::round((samplesPerBlock / sampleRate) * 1000.0);
        
        backupTimer.prepare(samplesPerBlock, timePerBlock, numChannels);
        
        startTimer(timePerBlock);
    }
    
    void setTimer()
    {
        lastTime = Time::getCurrentTime().getMillisecondCounterHiRes();
        hasTicked = true;
    }
    
    void setNonRealtime(bool nonRealtime){
        isNonRealtime = nonRealtime;
        if(isNonRealtime)  backupTimer.stopScheduler();
    }

    void timerCallback() override
    {
        int timePassed = Time::getCurrentTime().getMillisecondCounterHiRes() - lastTime;
        
        // Scheduler
        if(timePassed > 2 * timePerBlock && !hasTicked && !isNonRealtime) {
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

class Instance
{
    struct Message
    {
        String selector;
        String destination;
        std::vector<pd::Atom> list;
    };

    struct dmessage
    {
        void* object;
        String destination;
        String selector;
        std::vector<pd::Atom> list;
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

   public:
    Instance(String const& symbol);
    Instance(Instance const& other) = delete;
    virtual ~Instance();

    void prepareDSP(const int nins, const int nouts, const double samplerate, const int blockSize);
    void startDSP();
    void releaseDSP();
    void performDSP(float const* inputs, float* outputs);
    int getBlockSize() const;

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

    virtual void receiveGuiUpdate(int type) {};
    virtual void synchroniseCanvas(void* cnv) {};

    virtual void createPanel(int type, const char* snd, const char* location);

    void sendBang(const char* receiver) const;
    void sendFloat(const char* receiver, float const value) const;
    void sendSymbol(const char* receiver, const char* symbol) const;
    void sendList(const char* receiver, const std::vector<pd::Atom>& list) const;
    void sendMessage(const char* receiver, const char* msg, const std::vector<pd::Atom>& list) const;

    virtual void receivePrint(const String& message){};

    virtual void receiveBang(const String& dest)
    {
    }
    virtual void receiveFloat(const String& dest, float num)
    {
    }
    virtual void receiveSymbol(const String& dest, const String& symbol)
    {
    }
    virtual void receiveList(const String& dest, const std::vector<pd::Atom>& list)
    {
    }
    virtual void receiveMessage(const String& dest, const String& msg, const std::vector<pd::Atom>& list)
    {
    }
    virtual void receiveParameter(int idx, float value)
    {
    }

    virtual void receiveDSPState(bool dsp){};

    virtual void updateConsole(){};

    virtual void titleChanged(){};

    void enqueueFunction(const std::function<void(void)>& fn);
    void enqueueFunctionAsync(const std::function<void(void)>& fn);

    void enqueueMessages(const String& dest, const String& msg, std::vector<pd::Atom>&& list);

    void enqueueDirectMessages(void* object, std::vector<pd::Atom> const& list);
    void enqueueDirectMessages(void* object, const String& msg);
    void enqueueDirectMessages(void* object, const float msg);

    void logMessage(const String& message);
    void logError(const String& error);

    virtual void messageEnqueued(){};

    void sendMessagesFromQueue();
    void processMessage(Message mess);
    void processPrint(String print);
    void processMidiEvent(midievent event);
    void processSend(dmessage mess);

    Patch openPatch(const File& toOpen);

    virtual Colour getForegroundColour() = 0;
    virtual Colour getBackgroundColour() = 0;
    virtual Colour getTextColour() = 0;
    virtual Colour getOutlineColour() = 0;

    void setThis();

    void waitForStateUpdate();

    virtual const CriticalSection* getCallbackLock()
    {
        return nullptr;
    };

    void* m_instance = nullptr;
    void* m_patch = nullptr;
    void* m_atoms = nullptr;
    void* m_message_receiver = nullptr;
    void* m_parameter_receiver = nullptr;
    void* m_midi_receiver = nullptr;
    void* m_print_receiver = nullptr;

    std::atomic<bool> canUndo = false;
    std::atomic<bool> canRedo = false;

    inline static const String defaultPatch = "#N canvas 827 239 527 327 12;";

    std::deque<std::tuple<String, int, int>> consoleMessages;
    std::deque<std::tuple<String, int, int>> consoleHistory;

   private:
    moodycamel::ConcurrentQueue<std::function<void(void)>> m_function_queue = moodycamel::ConcurrentQueue<std::function<void(void)>>(4096);

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    WaitableEvent updateWait;
  
protected:
    ContinuityChecker continuityChecker;

    struct internal;
};
}  // namespace pd
