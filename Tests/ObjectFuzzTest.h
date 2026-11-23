// Creates every object (and documented sub-forms like [list split]) with fuzzed arguments and flags, sends
// random message sequences into each of its inlets while running DSP, then saves, duplicates, deletes and
// undoes it. Each object gets its own seed, so PLUGDATA_OBJECT_FILTER replays what a full run did.
class ObjectFuzzTest : public PlugDataUnitTest
{
public:
    ObjectFuzzTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Object Fuzz Test")
    {
    }

private:
    using Info = pd::Library::ObjectReferenceTable;

    Component::SafePointer<Canvas> cnv;
    File tempDir, previousWorkingDirectory;
    AudioDeviceManager* deviceManager = nullptr;
    int64 baseSeed = 0;
    StringArray symbolPool;
    int numFuzzed = 0;

    // Set PLUGDATA_FUZZ_SYMBOLS=1 to also fuzz with random and malformed symbols. These reach file operations
    // ([file mkdir/delete/move], [text write], ...). The fuzz patch and working directory are a temporary
    // directory, but only enable this in a sandbox (separate home and TMPDIR)
    static bool fuzzWithSymbols()
    {
        static bool const enabled = SystemStats::getEnvironmentVariable("PLUGDATA_FUZZ_SYMBOLS", {}) == "1";
        return enabled;
    }

    // Set PLUGDATA_FUZZ_EXTREME=1 to also use tiny, huge, infinite and NaN values in arguments and messages (signal inputs
    // always get them). Objects then loop, tick and allocate as much as they're told to, so expect hangs and out-of-memory aborts
    static bool fuzzWithExtremeValues()
    {
        static bool const enabled = SystemStats::getEnvironmentVariable("PLUGDATA_FUZZ_EXTREME", {}) == "1";
        return enabled;
    }

    void perform() override
    {
        auto& library = *editor->pd->objectLibrary;
        auto allObjects = library.getAllObjects();

        // Sub-forms like [list split] are only known from their documentation
        for (auto const& doc : library.getAllDocumentation()) {
            if (doc.title.containsChar(' ') && allObjects.contains(doc.title.upToFirstOccurrenceOf(" ", false, false)))
                allObjects.addIfNotAlreadyThere(doc.title);
        }

        // These all take a long time to process, so we skip them
        allObjects.removeString("All_else_objects");
        allObjects.removeString("All_about_else");
        allObjects.removeString("README.deken");
        allObjects.removeString("about.MERDA");
        allObjects.removeString("until"); // A bang loops forever by design

        // Set PLUGDATA_OBJECT_SKIP to comma-separated object names to leave out, such as known crashes
        for (auto const& name : StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_OBJECT_SKIP", {}), ",", ""))
            allObjects.removeString(name);

        // Set PLUGDATA_OBJECT_FILTER to comma-separated parts of the object names you want to test
        auto const filter = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_OBJECT_FILTER", {}), ",", "");
        if (!filter.isEmpty()) {
            for (int i = allObjects.size(); --i >= 0;)
                if (std::none_of(filter.begin(), filter.end(), [&](String const& token) { return allObjects[i].contains(token); }))
                    allObjects.remove(i);
        }

        beginTest(String(allObjects.size()) + " objects");

        // Relative paths in file operations end up in here
        tempDir = File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("plugdata-object-fuzz", "", false);
        tempDir.createDirectory();
        previousWorkingDirectory = File::getCurrentWorkingDirectory();
        tempDir.setAsCurrentWorkingDirectory();

        // Run DSP from here: CI runners often have no audio device, and it makes runs reproducible
        if ((deviceManager = ProjectInfo::getDeviceManager()))
            deviceManager->closeAudioDevice();
        editor->pd->volume->store(0.0f);

        baseSeed = rng.nextInt64();
        fuzzNext(allObjects);
    }

    void fuzzNext(StringArray objects)
    {
        if (!cnv) {
            auto const file = tempDir.getChildFile("fuzz.pd");
            file.replaceWithText("#N canvas 0 0 800 600 12;\n");
            editor->getTabComponent().openPatch(URL(file));
            cnv = editor->getTabComponent().getCurrentCanvas();
        }

        if (++numFuzzed % 16 == 0)
            editor->getSidebarForPanel(Sidebar::ConsolePanel)->clearConsole();

        editor->pd->lockAudioThread();
        pd_typedmess(reinterpret_cast<t_pd*>(cnv->patch.getRawPointer()), editor->pd->generateSymbol("clear"), 0, nullptr);
        editor->pd->unlockAudioThread();

        if (objects.isEmpty()) {
            if (deviceManager)
                deviceManager->restartLastAudioDevice();
            editor->getTabComponent().closeTab(cnv);
            previousWorkingDirectory.setAsCurrentWorkingDirectory();
            tempDir.deleteRecursively();
            signalDone(true);
            return;
        }

        auto const name = objects[objects.size() - 1];
        objects.remove(objects.size() - 1);

        rng.setSeed(baseSeed ^ name.hashCode64());
        auto const& info = editor->pd->objectLibrary->getObjectInfo(name);
        symbolPool = { "fuzz-array", "fuzz-tiny" };
        for (auto const& flag : info.flags)
            symbolPool.add(parse(flag.type).first);

        auto const box = makeBox(name, info);
        beginTest(String(objects.size()) + " -> " + box);
        fuzzObject(box, info);

        Timer::callAfterDelay(2, [this, objects] { fuzzNext(objects); });
    }

    void fuzzObject(String const& box, Info const& info)
    {
        auto* pd = editor->pd;
        auto& root = cnv->patch;

        // Host the object in the patch, or in a subpatch with its own block size and resampling
        std::unique_ptr<pd::Patch> subpatch;
        t_gobj* subpatchObject = nullptr;
        if (rng.nextBool()) {
            static StringArray const blocks { "", "block~ 1", "block~ 2", "block~ 4", "block~ 8", "block~ 128", "block~ 1024", "block~ 64 4", "block~ 256 2 0.5", "block~ 64 1 2", "block~ 16 1 0.25" };
            auto const block = blocks[rng.nextInt(blocks.size())];
            auto const text = "#N canvas 0 0 450 300 fuzz 0;\n" + (block.isEmpty() ? String() : "#X obj 10 10 " + block + ";\n") + "#X restore 10 10 pd fuzz;\n";
            pd->lockAudioThread();
            pd::Interface::paste(root.getRawPointer(), text.toRawUTF8());
            subpatchObject = pd::Interface::getNewest(root.getRawPointer());
            glist_noselect(root.getRawPointer());
            pd->unlockAudioThread();
            subpatch = std::make_unique<pd::Patch>(pd::WeakReference(subpatchObject, pd), pd, false);
        }
        auto& host = subpatch ? *subpatch : root;
        auto const create = [&host](int x, int y, String const& text) { return pd::Interface::checkObject(host.createObject(x, y, text)); };
        auto const connect = [&host](t_object* src, t_object* sink, int inlet) {
            if (src && sink)
                host.createConnection(src, 0, sink, inlet);
        };

        auto* array = create(10, 40, "array define fuzz-array 256");
        create(10, 70, "array define fuzz-tiny 1");
        auto* gobj = host.createObject(100, 160, box);
        if (!gobj)
            return;

        auto* obj = pd::Interface::checkObject(gobj);
        bool const isCanvas = pd_class(&gobj->g_pd) == canvas_class;
        int const numInlets = obj ? pd::Interface::numInlets(obj) : 0;

        // A [t a] into each inlet, and signal sources (plain, noise and multichannel) into some signal inlets
        HeapArray<std::pair<pd::WeakReference, int>> targets;
        SmallArray<t_object*> sources;
        pd::WeakReference sig(pd);
        for (int i = 0; i < numInlets; i++) {
            if (i >= 10 && i < numInlets - 2)
                continue;

            auto* feeder = create(100 + 30 * i, 120, "t a");
            connect(feeder, obj, i);
            if (feeder)
                targets.add({ pd::WeakReference(feeder, pd), i });

            if (!pd::Interface::isSignalInlet(obj, i))
                continue;
            if (sources.empty()) {
                auto* sigObject = create(10, 300, "sig~ " + boxToken(floatAtom(fuzzFloat(true))));
                auto* noise = create(80, 300, "noise~");
                auto* snake = create(10, 340, "snake~ in 3");
                connect(noise, snake, 0);
                connect(sigObject, snake, 1);
                sources = { sigObject, noise, snake };
                if (sigObject)
                    sig = pd::WeakReference(sigObject, pd);
            }
            if (auto const source = rng.nextInt(4))
                connect(sources[source - 1], obj, i);
        }
        if (targets.empty() && !isCanvas)
            targets.add({ pd::WeakReference(gobj, pd), 0 });

        pd::WeakReference const arrayRef(array, pd);
        pd::WeakReference const unitRef(subpatch ? subpatchObject : gobj, pd);

        setDSP(true);
        cnv->performSynchronise();
        tick(4);

        for (int i = targets.empty() ? 0 : 8 + rng.nextInt(24); --i >= 0;) {
            auto const& [target, inlet] = targets[rng.nextInt(static_cast<int>(targets.size()))];
            auto const [selector, args] = makeMessage(pickNotation(info, inlet, isCanvas), info);
            send(target, selector, args);

            switch (rng.nextInt(16)) {
            case 0: // Makes every object rebuild its DSP
                setDSP(false);
                setDSP(true);
                break;
            case 1:
                send(sig, "float", { floatAtom(fuzzFloat(true)) });
                break;
            case 2:
                send(arrayRef, "resize", { floatAtom(static_cast<float>(rng.nextInt(300))) });
                break;
            case 3: // For objects that hold on to the array
                if (auto g = arrayRef.get<t_gobj>(); g && rng.nextInt(4) == 0)
                    glist_delete(host.getRawPointer(), g.get());
                break;
            }
            tick(rng.nextInt(4));
        }
        tick(32);

        root.getCanvasContent(); // Runs every save function on the fuzzed state

        // A second instance made from the saved state, then delete the original and undo that
        if (auto* unit = unitRef.getRaw<t_gobj>()) {
            root.duplicate({ unit }, nullptr);
            root.deselectAll();
            cnv->performSynchronise();
            tick(8);
            if (unitRef.isValid()) {
                root.removeObjects({ unit });
                root.finishRemove();
                cnv->performSynchronise();
                tick(4);
                root.undo();
                cnv->performSynchronise();
                tick(8);
            }
        }
        cnv->synchronise();
    }

    String makeBox(String const& name, Info const& info)
    {
        StringArray tokens { name };
        auto const add = [&](std::vector<t_atom> const& atoms) {
            for (auto const& atom : atoms)
                tokens.add(boxToken(atom));
        };
        auto const addFlag = [&](bool withValue) {
            auto const [flag, specs] = parse(info.flags[rng.nextInt(static_cast<int>(info.flags.size()))].type);
            tokens.add(flag);
            std::vector<t_atom> atoms;
            if (withValue)
                for (auto const& spec : specs)
                    addAtoms(atoms, spec);
            add(atoms);
        };

        for (int i = info.flags.empty() ? 0 : rng.nextInt(4); --i >= 0;)
            addFlag(rng.nextInt(4) != 0);

        std::vector<t_atom> args;
        if (rng.nextInt(4)) {
            for (auto const& arg : info.arguments)
                addAtoms(args, arg.type);
        } else {
            addJunk(args);
        }
        add(args);

        // A trailing flag without its value
        if (!info.flags.empty() && rng.nextInt(4) == 0)
            addFlag(false);

        return tokens.joinIntoString(" ");
    }

    String pickNotation(Info const& info, int inlet, bool isCanvas)
    {
        // Methods go to the left inlet, but an abstraction can handle them in any inlet
        if ((inlet == 0 || isCanvas) && !info.methods.empty() && rng.nextInt(3) == 0)
            return info.methods[rng.nextInt(static_cast<int>(info.methods.size()))].type;

        auto const& inlets = info.inlets;
        auto const* doc = inlet < static_cast<int>(inlets.size()) ? &inlets[inlet] : (!inlets.empty() && inlets.back().repeating ? &inlets.back() : nullptr);
        if (doc && !doc->messages.empty() && rng.nextBool())
            return doc->messages[rng.nextInt(static_cast<int>(doc->messages.size()))].type;

        static StringArray const generic { "bang", "float", "symbol", "list", "anything" };
        return generic[rng.nextInt(generic.size())];
    }

    std::pair<String, std::vector<t_atom>> makeMessage(String const& notation, Info const& info)
    {
        auto [head, specs] = parse(notation);
        auto const alternatives = StringArray::fromTokens(head, "/", "");
        auto selector = alternatives.isEmpty() ? String("list") : alternatives[rng.nextInt(alternatives.size())].trim();
        if (selector.startsWith("signal"))
            selector = "float";

        if (selector == "anything") {
            selector = !info.methods.empty() && rng.nextBool() ? parse(info.methods[rng.nextInt(static_cast<int>(info.methods.size()))].type).first : fuzzSymbol();
            specs = { "list" };
        } else if (specs.isEmpty() && (selector == "float" || selector == "symbol" || selector == "list")) {
            specs.add(selector);
        }

        std::vector<t_atom> args;
        if (rng.nextInt(3) == 0) {
            addJunk(args); // Wrong types and counts on purpose
        } else {
            for (auto const& spec : specs)
                addAtoms(args, spec);
        }
        return { selector, args };
    }

    // "range <float, float>" -> { "range", { "float", "float" } }
    static std::pair<String, StringArray> parse(String const& notation)
    {
        StringArray specs;
        for (auto rest = notation.fromFirstOccurrenceOf("<", false, false); rest.isNotEmpty(); rest = rest.fromFirstOccurrenceOf("<", false, false))
            specs.addTokens(rest.upToFirstOccurrenceOf(">", false, false), ",", "");
        specs.trim();
        specs.removeEmptyStrings();
        return { notation.upToFirstOccurrenceOf("<", false, false).trim(), specs };
    }

    void addAtoms(std::vector<t_atom>& atoms, String const& spec)
    {
        auto const options = StringArray::fromTokens(spec, "/", "");
        auto const type = options[rng.nextInt(jmax(1, options.size()))].trim();
        if (type == "symbol" || type == "s")
            atoms.push_back(symbolAtom(fuzzSymbol()));
        else if (type == "list" || type == "anything")
            addJunk(atoms);
        else
            atoms.push_back(floatAtom(fuzzFloat()));
    }

    // Include empty and long lists: parsers often only get a handful of atoms right
    void addJunk(std::vector<t_atom>& atoms)
    {
        static constexpr int counts[] = { 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 9, 12, 20 };
        for (int i = counts[rng.nextInt(std::size(counts))]; --i >= 0;)
            atoms.push_back(rng.nextInt(3) ? floatAtom(fuzzFloat()) : symbolAtom(fuzzSymbol()));
    }

    float fuzzFloat(bool extreme = fuzzWithExtremeValues())
    {
        static constexpr float special[] = { 0.0f, -0.0f, 1.0f, -1.0f, 0.5f, -0.5f, 2.0f, 7.0f, 8.0f, 63.0f, 64.0f, 65.0f, 127.0f, 128.0f, 1024.0f, 4096.0f, -1000.0f };
        static constexpr float extremes[] = { 1e-40f, 1e-7f, 65536.0f, 1e9f, -1e9f, 2147483648.0f, -2147483648.0f, 1e37f, -1e37f,
            std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() };
        switch (rng.nextInt(extreme ? 5 : 4)) {
        case 4:
            return extremes[rng.nextInt(std::size(extremes))];
        case 0:
            return special[rng.nextInt(std::size(special))];
        case 1:
            return static_cast<float>(rng.nextInt(300) - 100);
        default:
            return static_cast<float>(rng.nextInt(200) - 100) / 10.0f;
        }
    }

    String fuzzSymbol()
    {
        if (!fuzzWithSymbols() || rng.nextBool())
            return symbolPool[rng.nextInt(symbolPool.size())];

        static String const malformed[] = { "", String::repeatedString("x", 2000), "$1", "$0-x", "a;b", "a,b", "1e5", "-", "a b", String::fromUTF8("\xc3\xbc\xe2\x82\xac") };
        if (rng.nextInt(3) == 0)
            return malformed[rng.nextInt(std::size(malformed))];

        String symbol;
        for (int i = rng.nextInt(12) + 1; --i >= 0;)
            symbol += String::charToString("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[rng.nextInt(62)]);
        return symbol;
    }

    static String boxToken(t_atom const& atom)
    {
        if (atom.a_type == A_SYMBOL) {
            auto const symbol = String::fromUTF8(atom.a_w.w_symbol->s_name);
            return symbol.isEmpty() || symbol.containsAnyOf(" ;,$\\") ? String("fuzz-array") : symbol;
        }
        auto const f = atom.a_w.w_float;
        if (std::isnan(f))
            return "0";
        if (std::isinf(f))
            return f > 0 ? "1e39" : "-1e39"; // Parsed as infinity
        return String(f);
    }

    static t_atom floatAtom(float f)
    {
        t_atom atom;
        SETFLOAT(&atom, f);
        return atom;
    }

    t_atom symbolAtom(String const& symbol) const
    {
        t_atom atom;
        SETSYMBOL(&atom, editor->pd->generateSymbol(symbol));
        return atom;
    }

    void send(pd::WeakReference const& target, String const& selector, std::vector<t_atom> const& args)
    {
        // An exactly sized copy, so ASan catches reads past the end of argv
        auto argv = std::make_unique<t_atom[]>(args.size());
        std::copy(args.begin(), args.end(), argv.get());
        if (auto ptr = target.get<t_pd>())
            pd_typedmess(ptr.get(), editor->pd->generateSymbol(selector), static_cast<int>(args.size()), argv.get());
    }

    void setDSP(bool on)
    {
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { on ? 1.0f : 0.0f });
        editor->pd->unlockAudioThread();
    }

    void tick(int blocks)
    {
        auto* pd = editor->pd;
        pd->setThis();
        std::vector<float> in(sys_get_inchannels() * pd::Instance::getBlockSize()), out(sys_get_outchannels() * pd::Instance::getBlockSize());
        while (--blocks >= 0) {
            pd->sendMessagesFromQueue();
            pd->performDSP(in.data(), out.data());
        }
    }
};
