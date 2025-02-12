class ObjectFuzzTest : public PlugDataUnitTest
{
public:
    ObjectFuzzTest(PluginEditor* editor) : PlugDataUnitTest(editor, "Object Fuzz Test")
    {
    }

private:
    void perform() override
    {
        editor->pd->objectLibrary->waitForInitialisationToFinish();

        auto* cnv = editor->getTabComponent().newPatch();
        auto allObjects = editor->pd->objectLibrary->getAllObjects();

        allObjects.removeString("All_objects");
        allObjects.removeString("all_objects");
        //allObjects.removeRange(allObjects.size() - 250, 250);
        createAndFuzzObject(cnv, allObjects, true);
    }

    void createAndFuzzObject(Canvas* cnv, StringArray allObjects, bool fuzzIncorrectly)
    {
        if(!allObjects.size())  {
            signalDone(true);
            return;
        }

        auto* pd = cnv->pd;
        auto objectName = allObjects.getReference(allObjects.size() - 1);
        allObjects.remove(allObjects.size() - 1);

        beginTest(String(allObjects.size()) + " -> " + objectName);

        editor->pd->volume->store(0.0f);
        editor->pd->lockAudioThread();
        editor->pd->sendMessage("pd", "dsp", { 1.0f });
        editor->pd->unlockAudioThread();
        
        auto info = pd->objectLibrary->getObjectInfo(objectName);
        auto methods = info.getChildWithName("methods");
        auto arguments = info.getChildWithName("arguments");

        if(fuzzIncorrectly)
        {
            for(int i = 0; i < generateRandomFloat(); i++)
            {
                objectName += " " + generateIncorrectArg().toString();
            }
        }
        else {
            for(auto arg : arguments)
            {
                auto type = arg.getProperty("type").toString();
                if(type == "float")
                {
                    objectName += " " + String(generateRandomFloat());
                }
                else if(type == "symbol")
                {
                    objectName += " " + String::fromUTF8(generateRandomString(8)->s_name);
                }
                else {
                    objectName += " " + String(generateRandomFloat());
                }
            }
        }

        static int lastY = 0;

        auto* gobj = cnv->patch.createObject(0, lastY, objectName);
        cnv->synchronise();

        lastY += 20;
        for (auto method : methods)
        {
            auto methodName = method.getProperty("type").toString().toStdString();
            auto description = method.getProperty("description").toString();
            auto nameWithoutArgs = String(methodName).upToFirstOccurrenceOf("<", false, false).trim();

            SmallArray<pd::Atom> args;
            if(fuzzIncorrectly)
            {
                for(int i = 0; i < generateRandomFloat() + 2; i++)
                {
                    args.add(generateIncorrectArg());
                }
            }
            else {
                // Extract expected argument types from the method notation
                HeapArray<std::string> expectedArgTypes;
                size_t pos = 0;
                while ((pos = methodName.find('<', pos)) != std::string::npos)
                {
                    size_t end = methodName.find('>', pos + 1);
                    if (end != std::string::npos)
                    {
                        expectedArgTypes.add(methodName.substr(pos, end - pos + 1));
                        pos = end + 1;
                    }
                }
                for (const auto& argType : expectedArgTypes)
                {
                    args.add_array(generateAtomForType(argType));
                }
            }

            if(gobj->g_pd == canvas_class)
            {
                auto* glist = reinterpret_cast<t_glist*>(gobj);
                auto* x = glist->gl_list;
                if(x) {
                    SmallArray<pd::WeakReference> allInlets;
                    do {
                        // Just send to all inlets, good to test anyway
                        if(x->g_pd->c_name == gensym("inlet"))
                        {
                            allInlets.add({x, pd});
                        }
                    } while ((x = x->g_next));

                    for(auto inletPtr : allInlets) {
                        if(auto inlet = inletPtr.get<void>()) {
                            cnv->editor->pd->sendDirectMessage(inlet.get(), SmallString(nameWithoutArgs), std::move(args));
                        }
                    }
                }
            }
            else {
                // Send the fuzzed arguments to the object
                cnv->editor->pd->sendDirectMessage(gobj, SmallString(nameWithoutArgs), std::move(args));
            }
        }

        // Send a float and symbol to each
        if(gobj->g_pd == canvas_class)
        {
            auto* glist = reinterpret_cast<t_glist*>(gobj);
            auto* x = glist->gl_list;
            if(x) {
                SmallArray<pd::WeakReference> allInlets;
                do {
                    if(x->g_pd->c_name == gensym("inlet"))
                    {
                        allInlets.add({x, pd});
                    }
                } while ((x = x->g_next));

                for(auto inletPtr : allInlets) {
                    if(auto inlet = inletPtr.get<void>()) {
                        cnv->editor->pd->sendDirectMessage(inlet.get(), SmallString("float"), {generateAtomForType("<float>")});
                        cnv->editor->pd->sendDirectMessage(inlet.get(), SmallString("symbol"), {generateAtomForType("<symbol>")});
                    }
                }
            }
        }
        else {
            cnv->editor->pd->sendDirectMessage(gobj, SmallString("float"), {generateAtomForType("<float>")});
            cnv->editor->pd->sendDirectMessage(gobj, SmallString("symbol"), {generateAtomForType("<symbol>")});
        }

        if(cnv->objects.size() > 10)
        {
            cnv->editor->sidebar->clearConsole();
            cnv->editor->pd->sendDirectMessage(cnv->patch.getRawPointer(), SmallString("clear"), {});
            lastY = 0;
        }

        Timer::callAfterDelay(40, [this, cnv, allObjects, fuzzIncorrectly](){
            createAndFuzzObject(cnv, allObjects, fuzzIncorrectly);
        });
    }
};
