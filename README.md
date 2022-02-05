# PlugData
Plugin wrapper around PureData to allow patching in a wide selection of DAWs.

<img width="1048" alt="Screenshot 2022-01-27 at 11 51 33" src="https://user-images.githubusercontent.com/44585538/151344732-983df431-d355-474f-9652-0aeab5e3baae.png">

<img width="1048" alt="151343690-8e397bf2-adbe-4c7a-aafd-8bf170ebd9c5" src="https://user-images.githubusercontent.com/44585538/151345013-6d67abb6-ddb8-4b46-896e-642b40c58012.png">

<img width="1048" alt="151343023-b6f88c21-cb24-47c4-b94d-86c1102aa040" src="https://user-images.githubusercontent.com/44585538/151345026-7b8aec32-ce7e-4a70-a707-a0d025984f17.png">



PlugData a plugin wrapper for PureData, featuring a new GUI made with JUCE. This is still a WIP, and there are probably still some bugs. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

```
git clone --recursive https://github.com/timothyschoen/PlugData.git
cd PlugData
mkdir build && cd build
cmake .. (the generator can be specified using -G"Unix Makefiles", -G"XCode" or -G"Visual Studio 16 2019" -A x64)
cmake --build .
```

**Important:**
- Please ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the PlugData repository .
- On Linux OS, Juce framework requires to install dependencies, please refer to [Linux Dependencies.md](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) and use the full command.
- The CMake build system has been tested with *Unix Makefiles*, *XCode* and *Visual Studio 16 2019*.

## Credits
Supported by [Deskew Technologies](https://gigperformer.com)

- [Camomile](https://github.com/pierreguillot/Camomile) by Pierre Guillot
- [ELSE](https://github.com/porres/pd-else) by Alexandre Torres Porres
- [cyclone](https://github.com/porres/pd-cyclone) by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Torres Porres, Derek Kwan, Matt Barber and others
- [Pure Data](https://puredata.info) by Miller Puckette and others
- [libpd](https://github.com/libpd/libpd) by the Peter Brinkmann, Dan Wilcox and others
- [Juce](https://github.com/WeAreROLI/JUCE) by ROLI Ltd.
- [ff_meters](https://github.com/ffAudio/ff_meters) by Foleys Finest Audio
- [MoodyCamel](https://github.com/cameron314/concurrentqueue) by Cameron Desrochers
- [LV2 PlugIn Technology](http://lv2plug.in) by Steve Harris, David Robillard and others
- [VST PlugIn Technology](https://www.steinberg.net/en/company/technologies/vst3.html) by Steinberg Media Technologies
- [Audio Unit PlugIn Technology](https://developer.apple.com/documentation/audiounit) by Apple
- [Juce LV2 interface](http://www.falktx.com) by Filipe Coelho
- [Multi-Dragger](https://github.com/jcredland/juce-multi-component-dragger) by Jim Credland

## Status
What works:
- Very close to full support for pd (including all GUI objects, undo/redo, copy/paste, saving, loading, console, object properties, drawing functions, audio and MIDI I/O, help files)
- Most ELSE library objects work
- LV2, AU and VST3 formats available, tested on Windows (x64), Mac (ARM/x64) and Linux (ARM/x64), also works as AU MIDI processor for Logic
- Receive 8 DAW parameters using "receive param1"

Known issues:
- Tabs sometimes close when closing editor
- Needs more testing on different systems and DAWs
- Unstable/broken objects:
  - Openpanel/savepanel
  - play.file~
  - text define
- There may still be some more bugs

## Roadmap
v0.3.1:
- Fix array crash when name is empty
- Fix helpfiles and issues with multiple instances
- Last console line not visible
- Fix graph on parent resizing
- Keyboard GUI resize behaviour
- Fix broken objects: openpanel/savepanel, play.file~, text define
- Allow click on subpatch to open
- Make symbolatom and floatatom look less similar
- Fix levelmeter redrawing
- Bigger hitboxes and clearer hover colour for inlets/outlets
- Ditch the mouse-over idea in favour of regular Pd/Max patching behaviour
- Increase number of automatable parameters

v0.3.2:
- Fix all warnings
- Clean up:
    - Box class, reduce resize calls
    - Better patch management (opening/closing patches, tab behaviour, remembering tabs when closing plugin editor)
    - Move sidebar features to separate file
    - Consistent naming of inlets/outlets
    - LookAndFeel classes
- MIDI Blinker
- Allow modifying labels
- Ensure all object properties are supported, including height/width where applicable
- Consistent GUI size logic
- Create document and system for object descriptions and inlet/outlet hover messages
- More flexible connection manipulation
- Update to latest ELSE externals
- Enable more cyclone externals
- Do a lot more testing

v0.4
- Smart patching behaviour: shift to drop object between connection or create multiple connections at once
- Final GUI changes before stable release
- Finished document with object descriptions and inlet/outlet hover messages
- Ensure that all externals fully work
- Create manual
- Improve code documentation


Future plans:
- Support for GEM

Please contact me if you wish to contribute, I could use some help! Bug reports are also appricated, they help me to get to a stable version much faster.
