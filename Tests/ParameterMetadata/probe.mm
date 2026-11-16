// Minimal native VST3 metadata host, adapted from the local lifecycle probe.
// Offline silent buffers only: no editor or audio device is created.
// Usage: probe BUNDLE STATE OUTPUT_PREFIX controller-first|state-first
//              [--pending-automation]
// Run only through run.py's filesystem sandbox and test-only path interposer.
#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include <sstream>
#include <iomanip>
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <cerrno>
using namespace Steinberg;
using namespace Steinberg::Vst;

class Host final : public IHostApplication {
    uint32 refs = 1;

public:
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        *obj = nullptr;
        if (FUnknownPrivate::iidEqual(iid, IHostApplication::iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
            *obj = static_cast<IHostApplication*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { return --refs; }
    tresult PLUGIN_API getName(String128 name) override
    {
        const char* s = "plugdata metadata lifecycle probe";
        std::fill(name, name + 128, 0);
        for (int i = 0; s[i]; ++i)
            name[i] = (TChar)s[i];
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID, TUID, void** obj) override
    {
        *obj = nullptr;
        return kNoInterface;
    }
};
class Stream final : public IBStream {
    uint32 refs = 1;
    int64 position = 0;

public:
    std::vector<char> bytes;
    Stream() = default;
    explicit Stream(const char* path)
    {
        std::ifstream f(path, std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(f), {});
    }
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        *obj = nullptr;
        if (FUnknownPrivate::iidEqual(iid, IBStream::iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
            *obj = static_cast<IBStream*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { return --refs; }
    tresult PLUGIN_API read(void* dst, int32 n, int32* done) override
    {
        if (n < 0 || position < 0 || position > (int64)bytes.size())
            return kInvalidArgument;
        int32 available = (int32)std::min<int64>(n, (int64)bytes.size() - position);
        if (available > 0)
            std::memcpy(dst, bytes.data() + position, available);
        position += available;
        if (done)
            *done = available;
        return available > 0 ? kResultOk : kResultFalse;
    }
    tresult PLUGIN_API write(void* src, int32 n, int32* done) override
    {
        if (n < 0 || position < 0)
            return kInvalidArgument;
        if (position + n > (int64)bytes.size())
            bytes.resize((size_t)(position + n));
        if (n)
            std::memcpy(bytes.data() + position, src, n);
        position += n;
        if (done)
            *done = n;
        return kResultOk;
    }
    tresult PLUGIN_API seek(int64 offset, int32 mode, int64* result) override
    {
        int64 next = offset
            + (mode == kIBSeekCur        ? position
                    : mode == kIBSeekEnd ? bytes.size()
                                         : 0);
        if (next < 0 || next > (int64)bytes.size())
            return kInvalidArgument;
        position = next;
        if (result)
            *result = position;
        return kResultOk;
    }
    tresult PLUGIN_API tell(int64* pos) override
    {
        *pos = position;
        return kResultOk;
    }
};
static void check(tresult r, const char* action)
{
    if (r != kResultOk) {
        std::fprintf(stderr, "%s failed: %d\n", action, r);
        std::exit(1);
    }
}
class Handler final : public IComponentHandler {
    uint32 refs = 1;

public:
    std::vector<int32> restarts;
    int edits = 0, begins = 0, ends = 0;
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override
    {
        *obj = nullptr;
        if (FUnknownPrivate::iidEqual(iid, IComponentHandler::iid)
            || FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
            *obj = static_cast<IComponentHandler*>(this);
            addRef();
            return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { return --refs; }
    tresult PLUGIN_API beginEdit(ParamID) override
    {
        ++begins;
        return kResultOk;
    }
    tresult PLUGIN_API performEdit(ParamID, ParamValue) override
    {
        ++edits;
        return kResultOk;
    }
    tresult PLUGIN_API endEdit(ParamID) override
    {
        ++ends;
        return kResultOk;
    }
    tresult PLUGIN_API restartComponent(int32 flags) override
    {
        restarts.push_back(flags);
        return kResultOk;
    }
};
static std::string json(const TChar* text)
{
    NSString* s = [NSString stringWithCharacters:(const unichar*)text
                                          length:std::char_traits<TChar>::length(text)];
    NSData* data = [NSJSONSerialization dataWithJSONObject:@[ s ] options:0 error:nil];
    std::string result((const char*)[data bytes], [data length]);
    return result.substr(1, result.size() - 2);
}
// Usage: probe BUNDLE STATE OUTPUT_PREFIX controller-first|state-first
// State-first deliberately processes64 samples*64 vectors before controller
// creation. The final controller recreation is a diagnostic reading of the same
// live processor properties; it does not reload the patch or process more audio.
static void snapshot(
    std::ostream& out, const char* stage, IEditController* controller, Handler& handler)
{
    out << "{\"stage\":\"" << stage << "\",\"parameter_count\":" << controller->getParameterCount()
        << ",\"parameters\":[";
    for (int i = 0; i < std::min(32, controller->getParameterCount()); ++i) {
        ParameterInfo info {};
        check(controller->getParameterInfo(i, info), "parameter info");
        double current = controller->getParamNormalized(info.id);
        String128 currentText {}, defaultText {}, halfText {};
        check(controller->getParamStringByValue(info.id, current, currentText), "current text");
        check(controller->getParamStringByValue(info.id, info.defaultNormalizedValue, defaultText),
            "default text");
        check(controller->getParamStringByValue(info.id, .5, halfText), "half text");
        if (i)
            out << ",";
        out << "{\"index\":" << i << ",\"id\":" << info.id << ",\"title\":" << json(info.title)
            << ",\"units\":" << json(info.units) << ",\"step_count\":" << info.stepCount
            << ",\"default\":" << info.defaultNormalizedValue << ",\"flags\":" << info.flags
            << ",\"current\":" << current << ",\"current_text\":" << json(currentText)
            << ",\"default_text\":" << json(defaultText) << ",\"half_text\":" << json(halfText)
            << "}";
    }
    out << "],\"restarts\":[";
    for (size_t i = 0; i < handler.restarts.size(); ++i) {
        if (i)
            out << ",";
        out << handler.restarts[i];
    }
    out << "],\"begins\":" << handler.begins << ",\"edits\":" << handler.edits
        << ",\"ends\":" << handler.ends << "}";
}
struct Controller {
    IEditController* edit = nullptr;
    IConnectionPoint* connection = nullptr;
};
static bool isolatedPaths()
{
    const char* expectedHome = std::getenv("PLUGDATA_PARAMETER_TEST_HOME");
    const char* expectedTemp = std::getenv("PLUGDATA_PARAMETER_TEST_TMPDIR");
    return expectedHome && expectedTemp
        && std::strcmp([NSHomeDirectory() fileSystemRepresentation], expectedHome) == 0
        && std::strcmp([NSTemporaryDirectory() fileSystemRepresentation], expectedTemp) == 0
        && std::getenv("TMPDIR") && std::strcmp(std::getenv("TMPDIR"), expectedTemp) == 0;
}
int main(int argc, char** argv)
{
    @autoreleasepool {
        if (!isolatedPaths()) {
            std::fprintf(stderr,
                "Required test-only home/temp interposition is absent; refusing to load a "
                "plug-in.\n");
            return 70;
        }
        if (argc == 3 && std::strcmp(argv[1], "--verify-isolation") == 0) {
            const std::string allowed
                = std::string(std::getenv("PLUGDATA_PARAMETER_TEST_TMPDIR")) + "/allowed-write";
            FILE* f = std::fopen(allowed.c_str(), "w");
            if (!f)
                return 71;
            std::fputs("isolated\n", f);
            std::fclose(f);
            errno = 0;
            f = std::fopen(argv[2], "w");
            if (f) {
                std::fclose(f);
                std::remove(argv[2]);
                return 72;
            }
            const int deniedError = errno;
            std::printf("{\"home\":\"%s\",\"temp\":\"%s\",\"denied_errno\":%d,\"pass\":%s}\n",
                [NSHomeDirectory() fileSystemRepresentation], std::getenv("TMPDIR"), deniedError,
                (deniedError == EPERM || deniedError == EACCES) ? "true" : "false");
            return deniedError == EPERM || deniedError == EACCES ? 0 : 73;
        }
    }
    if (argc != 5 && argc != 6)
        return 2;
    bool stateFirst = std::strcmp(argv[4], "state-first") == 0;
    bool pendingAutomation = argc == 6 && std::strcmp(argv[5], "--pending-automation") == 0;
    @autoreleasepool {
        [NSApplication sharedApplication];
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(
            nullptr, (UInt8*)argv[1], std::strlen(argv[1]), true);
        CFBundleRef bundle = CFBundleCreate(nullptr, url);
        CFRelease(url);
        if (!bundle || !CFBundleLoadExecutable(bundle))
            return 1;
        auto entry = reinterpret_cast<bool (*)(CFBundleRef)>(
            CFBundleGetFunctionPointerForName(bundle, CFSTR("bundleEntry")));
        auto factoryFn = reinterpret_cast<IPluginFactory* (*)()>(
            CFBundleGetFunctionPointerForName(bundle, CFSTR("GetPluginFactory")));
        auto exitFn = reinterpret_cast<bool (*)()>(
            CFBundleGetFunctionPointerForName(bundle, CFSTR("bundleExit")));
        if (!entry || !factoryFn || !entry(bundle))
            return 1;
        auto* factory = factoryFn();
        PClassInfo selected {};
        for (int i = 0; i < factory->countClasses(); ++i) {
            PClassInfo p {};
            check(factory->getClassInfo(i, &p), "class info");
            if (!std::strcmp(p.category, kVstAudioEffectClass)) {
                selected = p;
                break;
            }
        }
        IComponent* component = nullptr;
        check(factory->createInstance(selected.cid, IComponent::iid, (void**)&component),
            "component");
        Host host;
        Handler handler;
        check(component->initialize(&host), "initialize component");
        IConnectionPoint* cp = nullptr;
        check(
            component->queryInterface(IConnectionPoint::iid, (void**)&cp), "component connection");
        std::vector<tresult> syncResults;
        auto newController = [&]() {
            Controller c;
            TUID id {};
            check(component->getControllerClassId(id), "controller class");
            check(factory->createInstance(id, IEditController::iid, (void**)&c.edit), "controller");
            check(c.edit->initialize(&host), "initialize controller");
            check(c.edit->setComponentHandler(&handler), "handler");
            check(c.edit->queryInterface(IConnectionPoint::iid, (void**)&c.connection),
                "controller connection");
            check(cp->connect(c.connection), "connect component");
            check(c.connection->connect(cp), "connect controller");
            return c;
        };
        auto closeController = [&](Controller& c) {
            check(cp->disconnect(c.connection), "disconnect component");
            check(c.connection->disconnect(cp), "disconnect controller");
            c.connection->release();
            c.edit->setComponentHandler(nullptr);
            c.edit->terminate();
            c.edit->release();
            c = {};
        };
        auto syncController = [&](Controller& c) {
            Stream s;
            check(component->getState(&s), "state for controller");
            s.seek(0, IBStream::kIBSeekSet, nullptr);
            syncResults.push_back(c.edit->setComponentState(&s));
        };
        auto save = [&](const char* label) {
            Stream s;
            check(component->getState(&s), "save state");
            std::ofstream f(std::string(argv[3]) + "-" + label + ".state", std::ios::binary);
            f.write(s.bytes.data(), s.bytes.size());
        };
        Controller controller;
        if (!stateFirst)
            controller = newController();
        IAudioProcessor* processor = nullptr;
        check(component->queryInterface(IAudioProcessor::iid, (void**)&processor), "processor");
        int nin = component->getBusCount(kAudio, kInput),
            nout = component->getBusCount(kAudio, kOutput);
        if (nin < 1 || nout < 1) {
            std::fprintf(stderr, "probe requires a generic effect with an input bus\n");
            return 2;
        }
        std::vector<SpeakerArrangement> ins(nin, SpeakerArr::kEmpty),
            outs(nout, SpeakerArr::kEmpty);
        ins[0] = outs[0] = SpeakerArr::kStereo;
        check(processor->setBusArrangements(ins.data(), nin, outs.data(), nout), "buses");
        for (int i = 0; i < nin; ++i)
            check(component->activateBus(kAudio, kInput, i, i == 0), "input");
        for (int i = 0; i < nout; ++i)
            check(component->activateBus(kAudio, kOutput, i, i == 0), "output");
        ProcessSetup setup {};
        setup.processMode = kOffline;
        setup.symbolicSampleSize = kSample32;
        setup.maxSamplesPerBlock = 64;
        setup.sampleRate = 48000;
        check(processor->setupProcessing(setup), "setup");
        Stream state(argv[2]);
        check(component->setState(&state), "load state");
        if (!stateFirst)
            syncController(controller);
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.1]];
        check(component->setActive(true), "active");
        check(processor->setProcessing(true), "processing");
        float zero[64] = {}, left[64] = {}, right[64] = {};
        float* ip[2] = { zero, zero };
        float* op[2] = { left, right };
        std::vector<AudioBusBuffers> ib(nin), ob(nout);
        ib[0].numChannels = ob[0].numChannels = 2;
        ib[0].channelBuffers32 = ip;
        ob[0].channelBuffers32 = op;
        ProcessData data {};
        data.processMode = kOffline;
        data.symbolicSampleSize = kSample32;
        data.numSamples = 64;
        data.numInputs = nin;
        data.numOutputs = nout;
        data.inputs = ib.data();
        data.outputs = ob.data();
        ProcessContext context {};
        context.sampleRate = 48000;
        context.tempo = 120;
        data.processContext = &context;
        auto render = [&](int blocks) {
            for (int b = 0; b < blocks; ++b) {
                check(processor->process(data), "process");
                context.projectTimeSamples += 64;
            }
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.1]];
        };
        if (stateFirst) {
            render(64);
            controller = newController();
            syncController(controller);
        }
        std::ofstream report(std::string(argv[3]) + ".json");
        report << std::setprecision(17) << "{\"order\":\"" << argv[4] << "\",\"snapshots\":[";
        const char* labels[] = { "initial", "late-default-only", "late-range-only",
            "late-curve-only", "noop-range", "late-create-no-default" };
        for (int stage = 0; stage < 6; ++stage) {
            render(stage ? 256 : (stateFirst ? 0 : 64));
            if (stage)
                report << ",";
            snapshot(report, labels[stage], controller.edit, handler);
            save(labels[stage]);
            if (stage == 0 && pendingAutomation) {
                // The host has updated its controller but has not yet forwarded
                // that automation event in the process parameter queue. A later
                // unrelated mapping update must not erase this pending value.
                context.state = ProcessContext::kPlaying | ProcessContext::kTempoValid;
                render(1);
                ParameterInfo info {};
                bool found = false;
                for (int i = 0; i < controller.edit->getParameterCount(); ++i) {
                    check(controller.edit->getParameterInfo(i, info), "pending parameter");
                    if (json(info.title) == "\"held_float\"") {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::fprintf(stderr, "held_float missing\n");
                    return 3;
                }
                check(controller.edit->setParamNormalized(info.id, .9), "pending automation");
                report << ",";
                snapshot(report, "pending-controller-only", controller.edit, handler);
                save("pending-controller-only");
            }
        }
        check(processor->setProcessing(false), "stop processing");
        closeController(controller);
        controller = newController();
        syncController(controller);
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:.1]];
        report << ",";
        snapshot(report, "controller-recreated", controller.edit, handler);
        save("final");
        report << "],\"set_component_state_results\":[";
        for (size_t i = 0; i < syncResults.size(); ++i) {
            if (i)
                report << ",";
            report << syncResults[i];
        }
        report << "]}\n";
        report.close();
        check(component->setActive(false), "inactive");
        closeController(controller);
        cp->release();
        processor->release();
        component->terminate();
        component->release();
        factory->release();
        if (exitFn)
            exitFn();
        CFRelease(bundle);
    }
    return 0;
}
