// Some extra tests and helpers that are currently not used

void fuzzObjectsInCanvas(Component::SafePointer<Canvas> cnv, HeapArray<Object*>& objects)
{
    for (auto& object : objects)
    {
        auto info = cnv->pd->objectLibrary->getObjectInfo(object->getType());
        auto methods = info.getChildWithName("methods");

        for (auto method : methods)
        {
            auto methodName = method.getProperty("type").toString().toStdString();
            auto description = method.getProperty("description").toString();
            std::cout << "Fuzzing method: " << methodName << " (" << description << ")" << std::endl;

            auto nameWithoutArgs = String(methodName).upToFirstOccurrenceOf("<", false, false).trim();
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

            // Generate a vector of pd::Atom arguments based on expected types
            SmallArray<pd::Atom> args;
            for (const auto& argType : expectedArgTypes)
            {
                args.add_array(generateAtomForType(argType));
            }

            // Log the generated arguments for debugging
            std::cout << "Generated arguments: ";
            for (const auto& arg : args)
            {
                if (arg.isFloat())
                    std::cout << arg.getFloat() << " ";
                else if (arg.isSymbol())
                    std::cout << arg.getSymbol() << " ";
                else
                    std::cout << "<unknown> ";
            }
            std::cout << std::endl;

            // Send the fuzzed arguments to the object
            cnv->editor->pd->sendDirectMessage(object->getPointer(), SmallString(nameWithoutArgs), std::move(args));
        }
    }
}

void helpPatchToImage(TabComponent& tabbar, File file, String lib)
{
    auto outputFileDir = ProjectInfo::appDataDir.getChildFile("HelpFileImages");
    Image helpFileImage;

    auto* cnv = tabbar.openPatch(URL(file));
    tabbar.handleUpdateNowIfNeeded();

    cnv->jumpToOrigin();

    cnv->editor->nvgSurface.invalidateAll();
    cnv->editor->nvgSurface.render();
    cnv->editor->nvgSurface.renderFrameToImage(helpFileImage, cnv->patch.getBounds().withZeroOrigin());

    auto outputDir = outputFileDir.getChildFile(lib);
    outputDir.createDirectory();

    auto outputFile = outputDir.getChildFile(file.getFileNameWithoutExtension().upToLastOccurrenceOf("-help", false, false) + ".png");

    PNGImageFormat imageFormat;
    FileOutputStream ostream(outputFile);
    imageFormat.writeImageToStream(helpFileImage, ostream);

    tabbar.closeTab(cnv);
}

void exportHelpFileImages(TabComponent& tabbar, File outputFileDir)
{
    if(!outputFileDir.isDirectory()) outputFileDir.createDirectory();

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/9.else"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "else");
        }
    }

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/10.cyclone"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "cyclone");
        }
    }

    for(auto& file : OSUtils::iterateDirectory(ProjectInfo::appDataDir.getChildFile("Documentation/5.reference"), true, true))
    {
        if(file.hasFileExtension(".pd"))
        {
            helpPatchToImage(tabbar, file, "vanilla");
        }
    }
}
