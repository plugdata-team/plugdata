# PlugData
Plugin wrapper around PureData to allow patching in a wide selection of DAWs.

<img width="1048" alt="Screenshot 2022-02-11 at 17 09 10" src="https://user-images.githubusercontent.com/44585538/153626710-933853e5-868b-4a4c-a47f-9e5ba1e0efe3.png">

<img width="1048" alt="Screenshot 2022-02-11 at 17 01 22" src="https://user-images.githubusercontent.com/44585538/153625492-dd83ac00-3da3-43a9-8fb5-882ac085acf5.png">


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
- [cyclone](https://github.com/porres/pd-cyclone) by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Torres Porres, Derek Kwan, Matt Barber and others (note: Cyclone is included to offer an easy entry point for Max users but ELSE contains several alternatives to objects in Cyclone and Pure Data Vanilla itself also has some alternatives. Not only that, but objects that weren't cloned into Cyclone also have alternatives in ELSE, see: [this](https://github.com/porres/pd-else/wiki/Cyclone-alternatives))



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
- Broken objects:
  - text define
- There may still be some more bugs

## Roadmap

v0.3.3:
- Fix all warnings
- Fix multi-instance problems for plugins (DONE, hopefully)
- Clean up:
    - Box class, reduce resize calls
    - Better patch management (opening/closing patches, tab behaviour, remembering tabs when closing plugin editor) (DONE)
    - Move sidebar classes to separate file
    - Consistent naming of inlets/outlets
    - LookAndFeel classes (DONE)
- MIDI Blinker
- Allow modifying label text, font, position and colour
- Ensure all object properties are supported, including height/width where applicable
- Create document and system for object descriptions and inlet/outlet hover messages (DONE)
- More flexible connection manipulation
- Increase number of automatable parameters
- Make sure most Pd and ELSE objects work properly, especially ones with GUI features
  - text define, pic, possibly more
- Add AtomList object
- Fix resize problems:
    - No resize for some pre-existing objects
    - Array resize
    - Graph resize
    - Keyboard resize
- Fix gaps when dragging over array
- Fix showing suggestions in comments
- Fix comment rectangle not clearing
- Implement zooming by ctrl+scrolling, maybe pinch on trackpads/touchscreens
- Always show console when interacting with GUI objects
- Fix mixed red/white outline bug
- Add light theme for pd compatibility (to make black text in original pd patches readable)
- Fix ctrl to temporarily enter lock mode on Linux (DONE)
- Make sure pd patches always look correct (especially for compact UI's designed in pd)

v0.3.4 - v0.4:
- Support grid
- Smart patching behaviour: shift to drop object between connection or create multiple connections at once
- Final GUI changes before stable release
- Finished document with object descriptions and inlet/outlet hover messages
- Ensure that all externals work
- Create manual
- Improve code documentation


Future plans:
- Support for GEM or Ofelia

Please contact me if you wish to contribute, I could use some help! Bug reports are also appricated, they help me to get to a stable version much faster.
