#if JUCE_WINDOWS
#    include <process.h>
extern "C" __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void*, unsigned long);
extern "C" __declspec(dllimport) int __stdcall CloseHandle(void*);
#else
#    include <pthread.h>
#endif

// m_stack_guard.h must stop deep message recursion on small stacks (macOS audio threads, Windows plugin
// hosts) with an error instead of a crash, without cutting recursion short on large stacks.
class StackGuardTest : public PlugDataUnitTest {
public:
    StackGuardTest(PluginEditor* editor)
        : PlugDataUnitTest(editor, "Stack Guard Test")
    {
    }

private:
    void perform() override
    {
        // Each loop is 4 hops and puts 90 atoms on the stack twice: Pd's 1000-hop limit allows about
        // 250 loops, which takes about 900 KB in release builds and more in debug builds
        String atoms;
        for (int i = 1; i <= 90; i++)
            atoms << i << " ";

        auto& tabs = editor->getTabComponent();
        auto* cnv = tabs.openPatch(String("#N canvas 0 50 450 300 12;\n"
                                          "#X obj 30 20 r stack-guard-test;\n"
                                          "#X obj 30 50 t b b;\n"
                                          "#X obj 120 80 v stack-guard-loops;\n"
                                          "#X obj 120 110 + 1;\n"
                                          "#X obj 120 140 v stack-guard-loops;\n"
                                          "#X msg 30 170 ")
            + atoms.trim()
            + ";\n"
              "#X obj 30 200 list append;\n"
              "#X obj 30 230 list length;\n"
              "#X connect 0 0 1 0;\n"
              "#X connect 1 0 5 0;\n"
              "#X connect 1 1 2 0;\n"
              "#X connect 2 0 3 0;\n"
              "#X connect 3 0 4 0;\n"
              "#X connect 5 0 6 0;\n"
              "#X connect 6 0 7 0;\n"
              "#X connect 7 0 1 0;\n");

#if JUCE_MAC || JUCE_IOS
        bool const guarded = true;
#elif JUCE_WINDOWS
        bool const guarded = !ProjectInfo::isStandalone; // The Windows standalone has an 8 MB stack instead
#else
        bool const guarded = false;
#endif
        if (guarded) {
            beginTest("Recursion on a 512 KB stack stops with an error");
            auto const loops = recurse(512 * 1024);
            expect(loops > 0 && loops < 240, "recursion must stop before Pd's hop limit, ran " + String(loops) + " loops");
            expect(reportedOverflow(), "the overflow must be reported");
        }

        // Also shows that Pd recovered from the overflow above
        beginTest("Recursion that fits the stack reaches Pd's hop limit");
        auto const loops = recurse(8 * 1024 * 1024);
        expect(loops >= 240, "the guard must not stop recursion early, ran " + String(loops) + " loops");
        expect(reportedOverflow(), "the hop limit must be reported");

        if (cnv)
            tabs.closeTab(cnv);
        signalDone(true);
    }

    // Bangs the patch's loop on a thread with the given stack size, returns how many loops ran
    int recurse(size_t const stackSize)
    {
        auto* pd = editor->pd;
        pd->flushConsoleMessages();
        editor->getSidebarForPanel(Sidebar::ConsolePanel)->clearConsole();
        runOnThread(stackSize, [pd] {
            pd->lockAudioThread();
            value_setfloat(gensym("stack-guard-loops"), 0);
            if (auto* receiver = gensym("stack-guard-test")->s_thing)
                pd_bang(receiver);
            pd->unlockAudioThread();
        });

        t_float loops = 0;
        pd->lockAudioThread();
        value_getfloat(gensym("stack-guard-loops"), &loops);
        pd->unlockAudioThread();
        return static_cast<int>(loops);
    }

    bool reportedOverflow() const
    {
        editor->pd->flushConsoleMessages();
        for (auto& [object, message, type, length, repeats] : editor->pd->getConsoleMessages()) {
            if (type == 1 && message.contains("stack overflow"))
                return true;
        }
        return false;
    }

    // std::thread can't set a stack size, and juce::Thread only sets the committed size on Windows
    static void runOnThread(size_t const stackSize, std::function<void()> function)
    {
#if JUCE_WINDOWS
        constexpr unsigned stackSizeIsReservation = 0x10000; // STACK_SIZE_PARAM_IS_A_RESERVATION
        auto const entry = [](void* f) -> unsigned {
            (*static_cast<std::function<void()>*>(f))();
            return 0;
        };
        auto* thread = reinterpret_cast<void*>(_beginthreadex(nullptr, static_cast<unsigned>(stackSize), entry, &function, stackSizeIsReservation, nullptr));
        WaitForSingleObject(thread, 0xFFFFFFFF);
        CloseHandle(thread);
#else
        auto const entry = [](void* f) -> void* {
            (*static_cast<std::function<void()>*>(f))();
            return nullptr;
        };
        pthread_attr_t attributes;
        pthread_attr_init(&attributes);
        pthread_attr_setstacksize(&attributes, stackSize);
        pthread_t thread;
        pthread_create(&thread, &attributes, entry, &function);
        pthread_join(thread, nullptr);
        pthread_attr_destroy(&attributes);
#endif
    }
};
