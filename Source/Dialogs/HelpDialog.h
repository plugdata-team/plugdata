/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <Utility/BarelyML.h>


const String example = "# Compiling and Compiled Mode\n"
"\n"
"Not only can you run plugdata as an audio plugin inside your DAW, plugdata provides the ability to compile your plugdata code for multiple targets, including audio plugins. The currently supported targets are:\n"
"\n"
"- C++ code\n"
"  - C/C++ source code for use in other DSP projects\n"
"- Electro-Smith Daisy\n"
"  - Compilation for and flashing of a Daisy microcontroller board.\n"
"- DPF Audio Plugin\n"
"  - An audio plugin that can be an audio effect, instrument, or custom MIDI plugin. To be used by itself in the majority of DAWs.\n"
"- Pd External\n"
"  - Compile your patch into a Pd external object that can be used in PureData or plugdata like a built-in object.\n"
"\n"
">[!WARNING]\n"
">When exporting code or plugin binaries, as of writing, the destination path cannot contain *any* spaces.\n"
"\n"
"## Compiled Mode\n"
"\n"
"In the main plugdata menu, you will find a toggle box labelled ***Compiled Mode***.\n"
"\n"
"![plugdata Compilated Mode-screenShot](images/heavy_compiled_mode.png)\n"
"\n"
"Compiled mode checks your patch for compliance with the plugdata compilation tools.\n"
"Included with plugdata is the [Heavy hvcc compiler](https://wasted-audio.github.io/hvcc/docs/01.introduction.html#what-is-heavy) maintained by [Wasted Audio](https://wasted.audio/). hvcc can only compile plugins using a portion of the objects included with plugdata, which are a subset of the Pure Data Vanilla set of objects.\n"
"\n"
"***Compiled Mode*** indicates if there are objects in your patch that cannot be used in a compiled patch by posting a message to the Console, and outlining the object in question. The auto-completion in plugdata will also only provide compatible objects when this mode is activated.\n"
"\n"
"<img width=\"145\" alt=\"PlugData Compilation Mode - unsupported object\" src=\"images/heavy_unsupported.png\">\n"
"\n"
"<sub>Object error indication when using an unsupported object in Compilation Mode</sub>\n"
"\n"
"<img width=\"251\" alt=\"PlugData Compilation Mode - unsupported object console warning\" src=\"images/heavy_unsupported_warning.png\">\n"
"\n"
"<sub>Console error warning when using an unsupported object in Compilation Mode</sub>\n"
"\n"
"## Compiling in plugdata\n"
"\n"
"When you select `Compile...` a window opens with several compilation options along the left.\n"
"Each option has a general section displaying the common options for all modes and then specific options for the mode you currently have selected.\n"
"\n"
"### General\n"
"\n"
"![Plugdata Compilation Mode - General](images/heavy_general_config.png)\n"
"\n"
"This section is found in every compilation mode, with the following fields:\n"
"\n"
"- Patch to export\n"
"  - If a patch is currently open, it will read \"Currently opened patch\" otherwise it will read \"Choose a patch to export...\". Select the patch you wish to export here.\n"
"- Project Name (Optional)\n"
"  - The name of the project (potentially different than the patch name). This will autofill to your patch's name, or the last name you typed in this field.\n"
"  - Project Copyright (optional). If you want to share your code you can put it under one of the [common source licenses](https://spdx.org/licenses/) available.\n"
"\n"
"## C++ Code\n"
"\n"
"<img width=\"634\" alt=\"Plugdata Compiler C++ Code\" src=\"images/heavy_cpp_menu.png\">\n"
"\n"
"In the C++ Code mode, your plugdata patch isn't actually compiled, instead it is [transpiled/transcompiled](https://en.wikipedia.org/wiki/Source-to-source_compiler) from one coding language (Pd) to another (C/C++). The plugdata patch is transpiled to a generic C/C++ output and the raw code will need to be adapted for a specific application. Read more about the C/C++ API in the [official documentation](https://wasted-audio.github.io/hvcc/docs/05.c.html).\n"
"\n"
"## Electro-Smith Daisy\n"
"\n"
"<img width=\"635\" alt=\"Plugdata Compiler Daisy Export\" src=\"images/heavy_daisy_menu.png\">\n"
"\n"
"Daisy is an embedded platform for music produced by [Electro-Smith](https://electro-smith.com). It features everything you need for creating high fidelity audio hardware devices. This option allows to run your patch on an embedded hardware device based on an STM32 microcontroller. Read more about the Daisy exporter in the [official documentation](https://wasted-audio.github.io/hvcc/docs/03.gen.daisy.html).\n"
"\n"
"- Target board\n"
"  - Choose from a selection of predefined Daisy boards, or supply your own custom json file.\n"
"- Export type\n"
"  - **Source Code** exports the plugdata patch into C/C++ code that can be externally compiled for Daisy.\n"
"  - **Binary** produces a binary file that can be flashed onto a Daisy board manually.\n"
"  - **Flash** Compile and flash the program directly onto a connected Daisy board (this is the default).\n"
"- USB MIDI\n"
"  - This will enable MIDI i/o on the onboard microusb connection.\n"
"- Debug printing\n"
"  - This will make the `[print]` object output over serial. It wil automatically disable the `USB MIDI` option.\n"
"- Patch size\n"
"  - By default the Daisy is programmed on the internal Flash memory (small). It is also possible to flash a bootloader and then program to either `SRAM` (big) or `QSPI` (huge) memory regions. These are bigger than internal Flash, but are also potentially slower, which may impact performance. If the bootloader is not detected plugdata will try to first flash it to your Daisy board. Using the `Custom Linker` option you can provide your own advanced linker script for a custom memory layout.\n"
"\n"
"## DPF Audio Plugin\n"
"\n"
"<img width=\"636\" alt=\"Plugdata Compiler Plugin Export\" src=\"images/heavy_dpf_menu.png\">\n"
"\n"
"While plugdata itself can run as a plugin, you can also export self contained versions of your patch in the VST2, VST3, LV2, CLAP, and JACK standalone formats. HVCC uses the [DISTRHO Plugin Framework](https://github.com/DISTRHO/DPF) to generate these plugins.\n"
"\n"
"### DPF\n"
"\n"
"- Export type\n"
"  - **Source code** exports the plugdata patch into C/C++ source code that can be externally compiled into an audio plugin\n"
"  - **Binary** produces an executable plugin that can be loaded into any plugin host.\n"
"\n"
"- Plugin type. This automatically defines the MIDI input/output parameters, which is used by some hosts to categorize plugins and their functionality.\n"
"  - **Effect** is a plug in that accepts input audio, processes the audio, and outputs audio. It doesn't accept MIDI nor does it output MIDI.\n"
"  - **Instrument** is a plugin that accepts MIDI input to control the software instrument contained in the plugin, and outputs audio. Will not accept audio nor output MIDI.\n"
"  - **Custom** allows you to break with the effect or instrument conventions. This would be the type to choose if you wanted to create a plugin that modifies MIDI, for instance, as you can set the plugin to accept MIDI input but set it to also output MIDI data (also known as a MIDI effect). Not all hosts will allow such plugins.\n"
"\n"
"The Heavy Compiler supports many more options for the DPF target, however these are not included in the plugdata export menu yet. You can read more about the DPF exporter in the [official documentation](https://wasted-audio.github.io/hvcc/docs/03.gen.dpf.html).\n"
"  \n"
"### Plugin formats\n"
"\n"
"<img width=\"425\" alt=\"Plugdata Compiler Plugin Formats\" src=\"images/heavy_dpf_formats.png\">\n"
"\n"
"Here you select the formats you wish to output your patch as. You can select as many or as few formats as you like. JACK standalone is deselected by default, as many users do not have a JACK server available on their system.\n"
"\n"
"## Pd External\n"
"\n"
"<img width=\"638\" alt=\"Plugdata Compiler External\" src=\"images/heavy_pdexternal_menu.png\">\n"
"\n"
"plugdata allows you to write abstractions that look and operate like built in objects. Abstractions however can't run as efficiently as an object that is a compiled binary. By exporting your patch into a Pd external, which can be used in plugdata or regular Pd, you can optimize its performance and reuse it in your regular patches. Read more about the Pd External option in the [official documentation](https://wasted-audio.github.io/hvcc/docs/03.gen.pdext.html).\n"
"\n"
"- **Export Type**\n"
"  - Export your compiled external as a Binary (an object ready to be used in Pd) or as source code for further modification and compilation.\n"
"  \n"
"- **Copy to externals path**\n"
"  - This will create a copy of the external object in the plugdata/Externals folder  \n";

class HelpDialog : public Component {
    ResizableBorderComponent resizer;
    std::unique_ptr<Button> closeButton;
    ComponentDragger windowDragger;
    ComponentBoundsConstrainer constrainer;
    
public:
    std::function<void()> onClose;
    PluginProcessor* pd;
    BarelyMLDisplay markupDisplay;

    HelpDialog(PluginProcessor* instance)
        : resizer(this, &constrainer)
        , pd(instance)
    {
        markupDisplay.setMarkdownString(example);
        addAndMakeVisible(markupDisplay);
        
        closeButton.reset(LookAndFeel::getDefaultLookAndFeel().createDocumentWindowButton(-1));
        addAndMakeVisible(closeButton.get());

        constrainer.setMinimumSize(500, 300);

        closeButton->onClick = [this]() {
            MessageManager::callAsync([this]() {
                onClose();
            });
        };

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowHasDropShadow);
        setVisible(true);

        // Position in centre of screen
        setBounds(Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.withSizeKeepingCentre(700, 500));

        addAndMakeVisible(resizer);
    }
    

    void resized() override
    {
        resizer.setBounds(getLocalBounds());
        markupDisplay.setBounds(getLocalBounds().withTrimmedTop(40).withTrimmedLeft(250));
        
        auto closeButtonBounds = getLocalBounds().removeFromTop(30).removeFromRight(30).translated(-5, 5);
        closeButton->setBounds(closeButtonBounds);
    }
    
    void mouseDown(MouseEvent const& e) override
    {
        windowDragger.startDraggingComponent(this, e);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        windowDragger.dragComponent(this, e, nullptr);
    }

    void paint(Graphics& g) override
    {
        auto toolbarHeight = 38;
        auto b = getLocalBounds();
        auto titlebarBounds = b.removeFromTop(toolbarHeight).toFloat();
        auto arrayBounds = b.toFloat();

        Path toolbarPath;
        toolbarPath.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(toolbarPath);

        Path arrayPath;
        arrayPath.addRoundedRectangle(arrayBounds.getX(), arrayBounds.getY(), arrayBounds.getWidth(), arrayBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, false, false, true, true);
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillPath(arrayPath);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(toolbarHeight, 0, getWidth());
        
        Fonts::drawStyledText(g, "Help", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
    }
};
