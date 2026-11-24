// Creates every object (and documented sub-forms like [list split]) with fuzzed arguments and flags, sends
// random message sequences into each of its inlets while running DSP, then saves, duplicates, deletes and
// undoes it. Objects that find each other by name, like [delwrite~] and [delread~], are then fuzzed together.
// Each case gets its own seed, so PLUGDATA_OBJECT_FILTER replays what a full run did.
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

    // Set PLUGDATA_FUZZ_SYMBOLS=1 to also fuzz with malformed symbols (empty, very long, or with $ ; , and spaces).
    // Symbols reach file operations ([file mkdir/delete/move], [text write], ...), which resolve relative paths
    // against the fuzz patch's directory or the working directory: both are a temporary directory
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

    // Objects that find each other by name, fuzzed together as "pairs:<first object>" with fuzz-pair for NAME
    static inline StringArray const pairGroups[] = {
        { "delwrite~ NAME", "delread~ NAME", "delread4~ NAME" },
        { "send~ NAME", "receive~ NAME" },
        { "throw~ NAME", "catch~ NAME" },
        { "value NAME", "value NAME" },
        { "send NAME", "receive NAME", "sender NAME", "receiver NAME", "forward NAME", "retrieve NAME", "grab 1 NAME" },
        { "text define NAME", "text get NAME", "text set NAME", "text insert NAME", "text delete NAME", "text size NAME", "text tolist NAME", "text fromlist NAME", "text search NAME", "text sequence NAME" },
        { "file define NAME", "file handle NAME" },
        { "struct NAME float x float y array a NAME-e", "struct NAME-e float z", "scalar define NAME", "pointer", "vpointer NAME", "vpointer NAME", "get NAME x y", "set NAME x y", "element NAME a", "getsize NAME a", "setsize NAME a", "append NAME x y" },
        { "coll NAME", "coll NAME" },
        { "funbuff", "funbuff" },
        { "pv NAME", "pv NAME" },
        { "cyclone/table NAME", "cyclone/table NAME" },
        { "buffer~ NAME", "play~ NAME", "record~ NAME", "wave~ NAME", "index~ NAME", "peek~ NAME", "poke~ NAME", "lookup~ NAME", "buffir~ NAME" },
        { "messcoll NAME", "messcoll NAME" },
        { "dict NAME", "dict NAME" },
        { "del.in~ NAME", "del.out~ NAME" },
        { "del~ in NAME", "del~ out NAME" },
    };

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

        // Single objects go first (from the back), then the pairs
        HeapArray<StringArray> jobs;
        for (auto const& group : pairGroups)
            jobs.add(group);
        for (auto const& name : allObjects)
            jobs.add(StringArray(name));

        // Set PLUGDATA_OBJECT_SKIP to comma-separated object names (or pairs:<first object>) to leave out, such as known crashes
        auto const skip = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_OBJECT_SKIP", {}), ",", "");
        jobs.remove_if([&](StringArray const& job) { return skip.contains(jobKey(job)); });

        // Set PLUGDATA_OBJECT_FILTER to comma-separated parts of the object names you want to test, or pairs: for all pairs
        auto const filter = StringArray::fromTokens(SystemStats::getEnvironmentVariable("PLUGDATA_OBJECT_FILTER", {}), ",", "");
        if (!filter.isEmpty()) {
            jobs.remove_if([&](StringArray const& job) {
                return std::none_of(filter.begin(), filter.end(), [&](String const& token) {
                    return jobKey(job).contains(token) || std::any_of(job.begin(), job.end(), [&](String const& part) { return part.contains(token); });
                });
            });
        }

        beginTest(String(jobs.size()) + " objects");

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
        fuzzNext(std::move(jobs));
    }

    String jobKey(StringArray const& job) const
    {
        return job.size() == 1 ? job[0] : "pairs:" + partObject(job[0]).first;
    }

    // "text define NAME" -> { "text define", 1 }: the object, and how many arguments the part gives it
    std::pair<String, int> partObject(String const& part) const
    {
        auto const tokens = StringArray::fromTokens(part, false);
        if (tokens.size() > 1 && editor->pd->objectLibrary->getObjectInfo(tokens[0] + " " + tokens[1]).title.isNotEmpty())
            return { tokens[0] + " " + tokens[1], tokens.size() - 2 };
        return { tokens[0], tokens.size() - 1 };
    }

    void fuzzNext(HeapArray<StringArray> jobs)
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

        if (jobs.empty()) {
            if (deviceManager)
                deviceManager->restartLastAudioDevice();
            editor->getTabComponent().closeTab(cnv);
            previousWorkingDirectory.setAsCurrentWorkingDirectory();
            tempDir.deleteRecursively();
            signalDone(true);
            return;
        }

        auto const job = jobs.back();
        jobs.remove_at(jobs.size() - 1);

        rng.setSeed(baseSeed ^ jobKey(job).hashCode64());
        symbolPool = { "fuzz-array", "fuzz-tiny" };
        StringArray boxes;
        SmallArray<Info const*> infos;
        for (auto const& part : job) {
            auto const [object, given] = job.size() == 1 ? std::pair { part, 0 } : partObject(part);
            auto const& info = editor->pd->objectLibrary->getObjectInfo(object);
            for (auto const& flag : info.flags)
                symbolPool.add(parse(flag.type).first);
            boxes.add(job.size() == 1 ? makeBox(part, info) : makePartBox(part, info, given));
            infos.add(&info);
        }
        if (job.size() > 1)
            symbolPool.add("fuzz-pair");

        beginTest(String(jobs.size()) + " -> " + (job.size() > 1 ? jobKey(job) + " -> " : String()) + boxes.joinIntoString(" + "));
        fuzzObjects(boxes, infos);

        Timer::callAfterDelay(2, [this, jobs = std::move(jobs)]() mutable { fuzzNext(std::move(jobs)); });
    }

    void fuzzObjects(StringArray const& boxes, SmallArray<Info const*> const& infos)
    {
        auto* pd = editor->pd;
        auto& root = cnv->patch;
        auto const create = [](pd::Patch& host, int x, int y, String const& text) { return pd::Interface::checkObject(host.createObject(x, y, text)); };
        auto const connect = [](pd::Patch& host, t_object* src, int outlet, t_object* sink, int inlet) {
            if (src && sink)
                host.createConnection(src, outlet, sink, inlet);
        };

        auto* array = create(root, 10, 40, "array define fuzz-array 256");
        create(root, 10, 70, "array define fuzz-tiny 1");

        struct Part {
            pd::Patch* host;
            t_object* object;
            pd::WeakReference ref;
            Info const* info;
            bool isCanvas;
        };
        struct Target {
            pd::WeakReference feeder;
            int inlet;
            int part;
        };
        HeapArray<Part> parts;
        HeapArray<Target> targets;
        HeapArray<pd::WeakReference> units; // What sits in the patch itself: an object, or the subpatch hosting it
        HeapArray<pd::WeakReference> sigs;
        std::vector<std::unique_ptr<pd::Patch>> subpatches;

        for (int p = 0; p < boxes.size(); p++) {
            // Host each object in the patch, or in a subpatch with its own block size and resampling. Parts can share one
            pd::Patch* host = &root;
            if (p > 0 && !parts.empty() && rng.nextBool()) {
                host = parts.back().host;
            } else if (rng.nextBool()) {
                static StringArray const blocks { "", "block~ 1", "block~ 2", "block~ 4", "block~ 8", "block~ 128", "block~ 1024", "block~ 64 4", "block~ 256 2 0.5", "block~ 64 1 2", "block~ 16 1 0.25" };
                auto const block = blocks[rng.nextInt(blocks.size())];
                auto const text = "#N canvas 0 0 450 300 fuzz 0;\n" + (block.isEmpty() ? String() : "#X obj 10 10 " + block + ";\n") + "#X restore 10 10 pd fuzz;\n";
                pd->lockAudioThread();
                pd::Interface::paste(root.getRawPointer(), text.toRawUTF8());
                auto* subpatchObject = pd::Interface::getNewest(root.getRawPointer());
                glist_noselect(root.getRawPointer());
                pd->unlockAudioThread();
                subpatches.push_back(std::make_unique<pd::Patch>(pd::WeakReference(subpatchObject, pd), pd, false));
                units.add(pd::WeakReference(subpatchObject, pd));
                host = subpatches.back().get();
            }

            auto const x = 100 + 300 * p;
            auto* gobj = host->createObject(x, 160, boxes[p]);
            if (!gobj)
                continue;
            if (host == &root)
                units.add(pd::WeakReference(gobj, pd));

            auto* obj = pd::Interface::checkObject(gobj);
            bool const isCanvas = pd_class(&gobj->g_pd) == canvas_class;
            int const numInlets = obj ? pd::Interface::numInlets(obj) : 0;

            // A [t a] into each inlet, and signal sources (plain, noise and multichannel) into some signal inlets
            SmallArray<t_object*> sources;
            bool hasTarget = false;
            for (int i = 0; i < numInlets; i++) {
                if (i >= 10 && i < numInlets - 2)
                    continue;

                auto* feeder = create(*host, x + 30 * i, 120, "t a");
                connect(*host, feeder, 0, obj, i);
                if (feeder) {
                    targets.add({ pd::WeakReference(feeder, pd), i, static_cast<int>(parts.size()) });
                    hasTarget = true;
                }

                if (!pd::Interface::isSignalInlet(obj, i))
                    continue;
                if (sources.empty()) {
                    auto* sigObject = create(*host, x - 90, 300, "sig~ " + boxToken(floatAtom(fuzzFloat(true))));
                    auto* noise = create(*host, x - 20, 300, "noise~");
                    auto* snake = create(*host, x - 90, 340, "snake~ in 3");
                    connect(*host, noise, 0, snake, 0);
                    connect(*host, sigObject, 0, snake, 1);
                    sources = { sigObject, noise, snake };
                    if (sigObject)
                        sigs.add(pd::WeakReference(sigObject, pd));
                }
                if (auto const source = rng.nextInt(4))
                    connect(*host, sources[source - 1], 0, obj, i);
            }
            if (!hasTarget && !isCanvas)
                targets.add({ pd::WeakReference(gobj, pd), 0, static_cast<int>(parts.size()) });
            parts.add({ host, obj, pd::WeakReference(gobj, pd), infos[p], isCanvas });
        }

        // Parts in the same patch also feed each other: pointers into [get], signals, a [receive] into a [send]
        for (int a = 0; a < static_cast<int>(parts.size()); a++) {
            auto const& part = parts[a];
            SmallArray<int> candidates;
            for (int t = 0; t < static_cast<int>(targets.size()); t++) {
                if (targets[t].part != a && parts[targets[t].part].host == part.host)
                    candidates.add(t);
            }
            for (int o = 0; part.object && !candidates.empty() && o < jmin(4, pd::Interface::numOutlets(part.object)); o++) {
                if (rng.nextInt(3))
                    continue;
                auto const& target = targets[candidates[rng.nextInt(static_cast<int>(candidates.size()))]];
                if (pd::Interface::isSignalOutlet(part.object, o))
                    connect(*part.host, part.object, o, parts[target.part].object, target.inlet);
                else
                    connect(*part.host, part.object, o, target.feeder.getRaw<t_object>(), 0);
            }
        }

        pd::WeakReference const arrayRef(array, pd);

        setDSP(true);
        cnv->performSynchronise();
        tick(4);

        for (int i = targets.empty() ? 0 : 8 + rng.nextInt(24 * static_cast<int>(parts.size())); --i >= 0;) {
            auto const& target = targets[rng.nextInt(static_cast<int>(targets.size()))];
            auto const& part = parts[target.part];
            auto const [selector, args] = makeMessage(pickNotation(*part.info, target.inlet, part.isCanvas), *part.info);
            send(target.feeder, selector, args);

            switch (rng.nextInt(16)) {
            case 0: // Makes every object rebuild its DSP
                setDSP(false);
                setDSP(true);
                break;
            case 1: {
                auto const value = floatAtom(fuzzFloat(true));
                if (!sigs.empty())
                    send(sigs[sigs.size() > 1 ? rng.nextInt(static_cast<int>(sigs.size())) : 0], "float", { value });
                break;
            }
            case 2:
                send(arrayRef, "resize", { floatAtom(static_cast<float>(rng.nextInt(300))) });
                break;
            case 3: // For objects that hold on to the array
                if (auto g = arrayRef.get<t_gobj>(); g && rng.nextInt(4) == 0)
                    glist_delete(root.getRawPointer(), g.get());
                break;
            case 4: // For the other parts of a pair, which may still hold on to it
                if (parts.size() > 1 && rng.nextInt(4) == 0) {
                    auto const& victim = parts[rng.nextInt(static_cast<int>(parts.size()))];
                    if (auto g = victim.ref.get<t_gobj>())
                        glist_delete(victim.host->getRawPointer(), g.get());
                }
                break;
            }
            tick(rng.nextInt(4));
        }
        tick(32);

        root.getCanvasContent(); // Runs every save function on the fuzzed state

        // A second instance made from the saved state, then delete the original and undo that
        for (auto const& unitRef : units) {
            auto* unit = unitRef.getRaw<t_gobj>();
            if (!unit)
                continue;
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

    // A part with the shared name, and fuzzed values for the arguments it doesn't give
    String makePartBox(String const& part, Info const& info, int given)
    {
        auto tokens = StringArray::fromTokens(part.replace("NAME", "fuzz-pair"), false);
        std::vector<t_atom> args;
        if (auto const mode = rng.nextInt(8); mode == 0) {
            addJunk(args);
        } else if (mode < 5) {
            for (int i = given; i < static_cast<int>(info.arguments.size()); i++)
                addAtoms(args, info.arguments[i].type);
        }
        for (auto const& atom : args)
            tokens.add(boxToken(atom));
        return tokens.joinIntoString(" ");
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
        if (rng.nextBool())
            return symbolPool[rng.nextInt(symbolPool.size())];

        static String const malformed[] = { "", String::repeatedString("x", 2000), "$1", "$0-x", "a;b", "a,b", "1e5", "-", "a b", String::fromUTF8("\xc3\xbc\xe2\x82\xac") };
        if (fuzzWithSymbols() && rng.nextInt(3) == 0)
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
