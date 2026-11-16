// Test builds of the standalone host the plugdata VST3 that is built next to it (PLUGDATA_TEST_VST3), the way a DAW
// does. These tests run on the test runner thread, not the message thread: the plugin runs on the same message thread
// as we do, so it has to be free while we wait for the plugin to do something.

#ifdef PLUGDATA_TEST_VST3

#include <juce_audio_processors/juce_audio_processors.h>

namespace PluginHostPatches {

// [param cutoff] (range 0-10) drives [param echo] (range 0-100) with twice its value
inline String const parameters = "#N canvas 0 50 450 300 12;\n"
                                 "#X obj 20 20 loadbang;\n"
                                 "#X obj 20 50 t b b;\n"
                                 "#X msg 20 80 create 2.5 0 10;\n"
                                 "#X msg 180 80 create 0 0 100;\n"
                                 "#X obj 20 120 param cutoff;\n"
                                 "#X obj 180 120 param echo;\n"
                                 "#X obj 20 160 * 2;\n"
                                 "#X connect 0 0 1 0;\n"
                                 "#X connect 1 0 2 0;\n"
                                 "#X connect 1 1 3 0;\n"
                                 "#X connect 2 0 4 0;\n"
                                 "#X connect 3 0 5 0;\n"
                                 "#X connect 4 0 6 0;\n"
                                 "#X connect 6 0 5 0;\n";

// [param in] reaches [param out] (both range 0-100) through a global send name
inline String const sharedSend = "#N canvas 0 50 450 300 12;\n"
                                 "#X obj 20 20 loadbang;\n"
                                 "#X obj 20 50 t b b;\n"
                                 "#X msg 20 80 create 0 0 100;\n"
                                 "#X msg 180 80 create 0 0 100;\n"
                                 "#X obj 20 120 param in;\n"
                                 "#X obj 180 120 param out;\n"
                                 "#X obj 20 160 s plugdata-test-shared;\n"
                                 "#X obj 180 20 r plugdata-test-shared;\n"
                                 "#X connect 0 0 1 0;\n"
                                 "#X connect 1 0 2 0;\n"
                                 "#X connect 1 1 3 0;\n"
                                 "#X connect 2 0 4 0;\n"
                                 "#X connect 3 0 5 0;\n"
                                 "#X connect 4 0 6 0;\n"
                                 "#X connect 7 0 5 0;\n";

// [param step] at 1 re-creates [param recreated] with a new default, at 2 changes the range of [param ranged]
inline String const parameterInfo = "#N canvas 0 50 450 300 12;\n"
                                    "#X obj 20 20 loadbang;\n"
                                    "#X obj 20 50 t b b b;\n"
                                    "#X msg 20 80 create 0 0 2;\n"
                                    "#X msg 120 80 create 1 0 4 \\, mode 2;\n"
                                    "#X msg 280 80 create 2 0 4 \\, mode 2;\n"
                                    "#X obj 20 120 param step;\n"
                                    "#X obj 120 120 param recreated;\n"
                                    "#X obj 280 120 param ranged;\n"
                                    "#X obj 20 160 sel 1 2;\n"
                                    "#X msg 20 200 create 3 0 4;\n"
                                    "#X msg 120 200 range 0 8;\n"
                                    "#X connect 0 0 1 0;\n"
                                    "#X connect 1 0 2 0;\n"
                                    "#X connect 1 1 3 0;\n"
                                    "#X connect 1 2 4 0;\n"
                                    "#X connect 2 0 5 0;\n"
                                    "#X connect 3 0 6 0;\n"
                                    "#X connect 4 0 7 0;\n"
                                    "#X connect 5 0 8 0;\n"
                                    "#X connect 8 0 9 0;\n"
                                    "#X connect 8 1 10 0;\n"
                                    "#X connect 9 0 6 0;\n"
                                    "#X connect 10 0 7 0;\n";

} // namespace PluginHostPatches

class HostedPlugin final : Thread {
public:
    static constexpr double sampleRate = 48000.0;
    static constexpr int blockSize = 256;

    static std::unique_ptr<HostedPlugin> load(String& error)
    {
        auto instance = MessageManager::callSync([&error]() -> std::unique_ptr<AudioPluginInstance> {
            VST3PluginFormat format;
            OwnedArray<PluginDescription> types;
            format.findAllTypesForFile(types, PLUGDATA_TEST_VST3);
            if (types.isEmpty()) {
                error = "no plugin found in " PLUGDATA_TEST_VST3;
                return nullptr;
            }
            return format.createInstanceFromDescription(*types[0], sampleRate, blockSize, error);
        });

        if (!instance || !*instance)
            return nullptr;
        return std::unique_ptr<HostedPlugin>(new HostedPlugin(std::move(*instance)));
    }

    ~HostedPlugin() override
    {
        stopAudio();
        MessageManager::callSync([this] {
            window.reset(); // The editor has to go before the plugin
            instance.reset();
        });
    }

    void startAudio()
    {
        MessageManager::callSync([this] { instance->prepareToPlay(sampleRate, blockSize); });
        startThread(Priority::high);
    }

    void stopAudio()
    {
        if (!isThreadRunning())
            return;
        stopThread(5000);
        MessageManager::callSync([this] { instance->releaseResources(); });
    }

    void showEditor()
    {
        MessageManager::callSync([this] { window = std::make_unique<EditorWindow>(instance->createEditorIfNeeded()); });
    }

    // Holds the blob PluginProcessor::setStateInformation reads: an empty legacy header, then the XML state
    static MemoryBlock makeState(String const& patch)
    {
        XmlElement xml("plugdata_save");
        xml.setAttribute("Version", PLUGDATA_VERSION);
        xml.setAttribute("Legacy", false);
        auto* patchXml = xml.createNewChildElement("Patches")->createNewChildElement("Patch");
        patchXml->setAttribute("Content", patch);
        patchXml->setAttribute("Location", "");

        MemoryBlock xmlBlock;
        AudioProcessor::copyXmlToBinary(xml, xmlBlock);

        MemoryBlock pluginState;
        MemoryOutputStream stream(pluginState, false);
        stream.writeInt(0); // Legacy patch count
        stream.writeInt(0); // Legacy latency
        stream.writeInt(0); // Legacy oversampling
        stream.writeFloat(0.0f); // Legacy tail length
        stream.writeInt(static_cast<int>(xmlBlock.getSize()));
        stream.write(xmlBlock.getData(), xmlBlock.getSize());
        stream.flush();

        // JUCE's VST3 host wraps the plugin's state like this
        XmlElement hostXml("VST3PluginState");
        hostXml.createNewChildElement("IComponent")->addTextElement(pluginState.toBase64Encoding());
        MemoryBlock hostState;
        AudioProcessor::copyXmlToBinary(hostXml, hostState);
        return hostState;
    }

    void loadPatch(String const& patch)
    {
        setState(makeState(patch));
    }

    void setState(MemoryBlock const& state)
    {
        MessageManager::callSync([this, &state] { instance->setStateInformation(state.getData(), static_cast<int>(state.getSize())); });
    }

    MemoryBlock getState()
    {
        return MessageManager::callSync([this] {
            MemoryBlock state;
            instance->getStateInformation(state);
            return state;
        }).value_or(MemoryBlock());
    }

    // Parameter names change while the plugin runs ([param] renames them), so look them up on the message thread
    AudioProcessorParameter* waitForParameter(String const& name, int const timeoutMs = 5000)
    {
        AudioProcessorParameter* found = nullptr;
        waitUntil([this, &name, &found] {
            found = MessageManager::callSync([this, &name]() -> AudioProcessorParameter* {
                for (auto* parameter : instance->getParameters()) {
                    if (parameter->getName(128) == name)
                        return parameter;
                }
                return nullptr;
            }).value_or(nullptr);
            return found != nullptr;
        }, timeoutMs);
        return found;
    }

    static String getText(AudioProcessorParameter* parameter)
    {
        return MessageManager::callSync([parameter] { return parameter->getCurrentValueAsText(); }).value_or(String());
    }

    static bool waitUntil(std::function<bool()> const& condition, int const timeoutMs = 5000)
    {
        auto const end = Time::getMillisecondCounter() + timeoutMs;
        while (!condition()) {
            if (Time::getMillisecondCounter() > end)
                return false;
            Thread::sleep(10);
        }
        return true;
    }

    static bool waitForValue(AudioProcessorParameter* parameter, float const normalisedValue, int const timeoutMs = 5000)
    {
        return waitUntil([parameter, normalisedValue] { return std::abs(parameter->getValue() - normalisedValue) < 0.001f; }, timeoutMs);
    }

private:
    explicit HostedPlugin(std::unique_ptr<AudioPluginInstance> plugin)
        : Thread("VST3 Host Audio")
        , instance(std::move(plugin))
    {
    }

    // Renders in real time, like an audio device would
    void run() override
    {
        AudioBuffer<float> buffer(jmax(1, instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels()), blockSize);
        MidiBuffer midi;
        auto const blockDurationMs = 1000.0 * blockSize / sampleRate;
        auto nextBlock = Time::getMillisecondCounterHiRes();
        while (!threadShouldExit()) {
            buffer.clear();
            midi.clear();
            instance->processBlock(buffer, midi);

            nextBlock += blockDurationMs;
            auto const waitMs = nextBlock - Time::getMillisecondCounterHiRes();
            if (waitMs >= 1.0)
                wait(static_cast<int>(waitMs));
        }
    }

    struct EditorWindow final : DocumentWindow {
        explicit EditorWindow(AudioProcessorEditor* editor)
            : DocumentWindow("plugdata VST3", Colours::black, 0)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(editor, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
            toFront(true);
        }
    };

    std::unique_ptr<AudioPluginInstance> instance;
    std::unique_ptr<EditorWindow> window;
};

// [param] creates, renames and moves host parameters at runtime, which reaches the host through
// PluginProcessor::sendParameterInfoChangeMessage and the VST3 wrapper's restartComponent
class VST3ParameterTest final : public UnitTest {
public:
    VST3ParameterTest()
        : UnitTest("VST3 Parameter Test")
    {
    }

    void runTest() override
    {
        beginTest("A [param] in the patch becomes a host parameter");
        String error;
        auto plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->startAudio();
        plugin->loadPatch(PluginHostPatches::parameters);
        auto* cutoff = plugin->waitForParameter("cutoff");
        auto* echo = plugin->waitForParameter("echo");
        expect(cutoff && echo, "The host doesn't see the parameters the patch created");
        if (!cutoff || !echo)
            return;

        expectWithinAbsoluteError(cutoff->getValue(), 0.25f, 0.001f, "The default value should come from the patch");
        expectEquals(HostedPlugin::getText(cutoff), String("2.5"));

        beginTest("Host automation reaches the patch, and the patch's output reaches the host");
        cutoff->setValueNotifyingHost(0.8f);
        expect(HostedPlugin::waitForValue(echo, 0.16f), "Expected echo at 0.16 (16 of 100), got " + String(echo->getValue()));

        // The host caches each parameter's default and step count until the plugin reports a parameter info change
        beginTest("Repeated create and range messages update the host's parameter info");
        plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->startAudio();
        plugin->loadPatch(PluginHostPatches::parameterInfo);
        auto* step = plugin->waitForParameter("step");
        auto* recreated = plugin->waitForParameter("recreated");
        auto* ranged = plugin->waitForParameter("ranged");
        expect(step && recreated && ranged, "The host doesn't see the parameters the patch created");
        if (!step || !recreated || !ranged)
            return;

        auto defaultValue = [](AudioProcessorParameter* p) { return MessageManager::callSync([p] { return p->getDefaultValue(); }).value_or(-1.0f); };
        auto numSteps = [](AudioProcessorParameter* p) { return MessageManager::callSync([p] { return p->getNumSteps(); }).value_or(-1); };
        auto settled = HostedPlugin::waitUntil([&] { return numSteps(recreated) == 5 && numSteps(ranged) == 5; });
        expect(settled, "The host didn't see the initial integer ranges");
        if (!settled)
            return;
        expectWithinAbsoluteError(defaultValue(recreated), 0.25f, 0.001f);

        step->setValueNotifyingHost(0.5f);
        HostedPlugin::waitUntil([&] { return std::abs(defaultValue(recreated) - 0.75f) < 0.001f; }, 2000);
        expectWithinAbsoluteError(defaultValue(recreated), 0.75f, 0.001f, "The host's default didn't follow a repeated create");

        step->setValueNotifyingHost(1.0f);
        HostedPlugin::waitUntil([&] { return numSteps(ranged) == 9; }, 2000);
        expectEquals(numSteps(ranged), 9, "The host's step count didn't follow a range change");
    }
};

class VST3StateTest final : public UnitTest {
public:
    VST3StateTest()
        : UnitTest("VST3 State Test")
    {
    }

    void runTest() override
    {
        beginTest("Saved state restores the patch and its parameters in a new instance");
        String error;
        auto original = HostedPlugin::load(error);
        auto restored = HostedPlugin::load(error);
        expect(original && restored, "Could not load the VST3: " + error);
        if (!original || !restored)
            return;

        original->startAudio();
        original->loadPatch(PluginHostPatches::parameters);
        auto* cutoff = original->waitForParameter("cutoff");
        auto* echo = original->waitForParameter("echo");
        expect(cutoff && echo, "The host doesn't see the parameters the patch created");
        if (!cutoff || !echo)
            return;

        cutoff->setValueNotifyingHost(0.8f);
        expect(HostedPlugin::waitForValue(echo, 0.16f), "The patch didn't respond to automation");
        auto const state = original->getState();

        restored->startAudio();
        restored->setState(state);
        auto* restoredCutoff = restored->waitForParameter("cutoff");
        auto* restoredEcho = restored->waitForParameter("echo");
        expect(restoredCutoff && restoredEcho, "The restored instance doesn't have the saved parameters");
        if (!restoredCutoff || !restoredEcho)
            return;

        // The patch's [loadbang] sets the default (0.25) after the parameters are restored
        expectWithinAbsoluteError(restoredCutoff->getValue(), 0.8f, 0.001f, "The saved value should win over the patch's default");
        restoredCutoff->setValueNotifyingHost(0.3f);
        expect(HostedPlugin::waitForValue(restoredEcho, 0.06f), "The restored patch doesn't respond to automation");

        beginTest("Restoring state while the instance is processing");
        auto const otherPatch = HostedPlugin::makeState(PluginHostPatches::sharedSend);
        for (int i = 0; i < 4; i++) {
            original->setState(state);
            original->setState(otherPatch);
        }

        auto* in = original->waitForParameter("in");
        auto* out = original->waitForParameter("out");
        expect(in && out, "The last restored patch didn't create its parameters");
        if (!in || !out)
            return;

        in->setValueNotifyingHost(0.42f);
        expect(HostedPlugin::waitForValue(out, 0.42f), "The last restored patch doesn't respond to automation");
    }
};

// Every instance has its own Pd instance: a global send name must not reach another plugin instance
class VST3MultiInstanceTest final : public UnitTest {
public:
    VST3MultiInstanceTest()
        : UnitTest("VST3 Multi-Instance Test")
    {
    }

    void runTest() override
    {
        beginTest("Instances don't share Pd state");
        String error;
        auto first = HostedPlugin::load(error);
        auto second = HostedPlugin::load(error);
        expect(first && second, "Could not load the VST3: " + error);
        if (!first || !second)
            return;

        for (auto* plugin : { first.get(), second.get() }) {
            plugin->startAudio();
            plugin->loadPatch(PluginHostPatches::sharedSend);
        }

        auto* firstIn = first->waitForParameter("in");
        auto* firstOut = first->waitForParameter("out");
        auto* secondIn = second->waitForParameter("in");
        auto* secondOut = second->waitForParameter("out");
        expect(firstIn && firstOut && secondIn && secondOut, "The host doesn't see the parameters the patch created");
        if (!firstIn || !firstOut || !secondIn || !secondOut)
            return;

        firstIn->setValueNotifyingHost(0.42f);
        expect(HostedPlugin::waitForValue(firstOut, 0.42f), "The first instance didn't respond to automation");
        Thread::sleep(250);
        expectWithinAbsoluteError(secondOut->getValue(), 0.0f, 0.001f, "The first instance's [send] reached the second instance");

        secondIn->setValueNotifyingHost(0.17f);
        expect(HostedPlugin::waitForValue(secondOut, 0.17f), "The second instance didn't respond to automation");
        Thread::sleep(250);
        expectWithinAbsoluteError(firstOut->getValue(), 0.42f, 0.001f, "The second instance's [send] reached the first instance");

        beginTest("An instance can go away while another one keeps processing");
        for (int i = 0; i < 50; i++) {
            firstIn->setValueNotifyingHost(static_cast<float>(i % 10) / 10.0f);
            secondIn->setValueNotifyingHost(static_cast<float>((i + 5) % 10) / 10.0f);
            Thread::sleep(5);
        }
        first.reset();

        secondIn->setValueNotifyingHost(0.66f);
        expect(HostedPlugin::waitForValue(secondOut, 0.66f), "The remaining instance stopped responding");

        // Removing an instance frees its Pd symbols. Pd's class loader kept one of them on a global stack,
        // and the next instance's setup crashed reading it
        beginTest("A new instance after one was removed");
        auto third = HostedPlugin::load(error);
        expect(third != nullptr, "Could not load the VST3: " + error);
        if (!third)
            return;

        third->startAudio();
        third->loadPatch(PluginHostPatches::sharedSend);
        auto* thirdIn = third->waitForParameter("in");
        auto* thirdOut = third->waitForParameter("out");
        expect(thirdIn && thirdOut, "The host doesn't see the parameters the patch created");
        if (!thirdIn || !thirdOut)
            return;

        thirdIn->setValueNotifyingHost(0.33f);
        expect(HostedPlugin::waitForValue(thirdOut, 0.33f), "The new instance doesn't respond to automation");
    }
};

// Opens the VST3's editor, which starts the test suite inside the plugin (see runTests in Tests.cpp),
// and reports the plugin's results here. PLUGDATA_TEST_FILTER_VST3 picks the tests it runs.
class VST3SuiteTest final : public UnitTest {
public:
    VST3SuiteTest()
        : UnitTest("VST3 Suite Test")
    {
    }

    void runTest() override
    {
        beginTest("Test suite inside the VST3");
        auto const resultsDir = ProjectInfo::appDataDir.getChildFile("TestResults");
        auto const logFile = resultsDir.getChildFile("VST3.log");
        auto const summaryFile = resultsDir.getChildFile("VST3.json");
        logFile.deleteFile();
        summaryFile.deleteFile();

        String error;
        auto plugin = HostedPlugin::load(error);
        expect(plugin != nullptr, "Could not load the VST3: " + error);
        if (!plugin)
            return;

        plugin->startAudio();
        plugin->showEditor();

        if (!HostedPlugin::waitUntil([&logFile] { return logFile.existsAsFile(); }, 60000)) {
            expect(false, "The suite didn't start inside the VST3");
            return;
        }

        // No timeout: the suite takes as long inside the plugin as it does here
        while (!summaryFile.existsAsFile())
            Thread::sleep(100);

        auto const summary = JSON::parse(summaryFile);
        auto const passes = static_cast<int>(summary["passes"]);
        auto const failures = static_cast<int>(summary["failures"]);
        logMessage("VST3: " + String(passes) + " passed, " + String(failures) + " failed");
        expect(passes + failures > 0, "No tests ran inside the VST3. Check PLUGDATA_TEST_FILTER_VST3 or PLUGDATA_TEST_FILTER");

        if (auto const* failed = summary["failed"].getArray()) {
            for (auto const& result : *failed)
                expect(false, "[VST3] " + result["test"].toString() + " / " + result["case"].toString() + ": " + result["messages"].toString());
        }
    }
};

#endif
