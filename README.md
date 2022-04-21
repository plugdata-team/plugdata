# PlugData
Plugin wrapper around PureData to allow patching in a wide selection of DAWs.

<img width="1054" alt="Screenshot 2022-04-21 at 03 30 17" src="https://user-images.githubusercontent.com/44585538/164353656-c087ebe3-4325-4f02-89d9-ac57c5ca332f.png">

<img width="1054" alt="Screenshot 2022-04-21 at 03 35 01" src="https://user-images.githubusercontent.com/44585538/164354121-bb196ebc-599e-4d4a-9083-ee580ad4317a.png">

PlugData is a plugin wrapper for PureData, featuring a new GUI made with JUCE. This is still a WIP, and there are probably still some bugs. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

For Arch users, you can install [the AUR package](https://aur.archlinux.org/packages/plugdata-git) manually or via an AUR helper.

## Build

```
git clone --recursive https://github.com/timothyschoen/PlugData.git
cd PlugData
mkdir build && cd build
cmake .. (the generator can be specified using -G"Unix Makefiles", -G"XCode" or -G"Visual Studio 16 2019" -A x64)
cmake --build .
```

**Important:**
- Please ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the PlugData repository .
- On Linux, Juce framework requires to install dependencies, please refer to [Linux Dependencies.md](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) and use the full command.
- The CMake build system has been tested with *Unix Makefiles*, *XCode* and *Visual Studio 16 2019*.

## Credits
Supported by [Deskew Technologies](https://gigperformer.com)

- [Camomile](https://github.com/pierreguillot/Camomile) by Pierre Guillot
- [ELSE](https://github.com/porres/pd-else) by Alexandre Torres Porres
- [cyclone](https://github.com/porres/pd-cyclone) by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Torres Porres, Derek Kwan, Matt Barber and others (note: Cyclone is included to offer an easy entry point for Max users but ELSE contains several alternatives to objects in Cyclone and Pure Data Vanilla itself also has some alternatives. Not only that, but objects that weren't cloned into Cyclone also have alternatives in ELSE, see: [this](https://github.com/porres/pd-else/wiki/Cyclone-alternatives))
- [Pure Data](https://puredata.info) by Miller Puckette and others
- [libpd](https://github.com/libpd/libpd) by the Peter Brinkmann, Dan Wilcox and others
- [Juce](https://github.com/WeAreROLI/JUCE) by ROLI Ltd.
- [MoodyCamel](https://github.com/cameron314/concurrentqueue) by Cameron Desrochers
- [Kiwi](https://github.com/Musicoll/Kiwi) by Eliott Paris, Pierre Guillot and Jean Millot
- [LV2 PlugIn Technology](http://lv2plug.in) by Steve Harris, David Robillard and others
- [VST PlugIn Technology](https://www.steinberg.net/en/company/technologies/vst3.html) by Steinberg Media Technologies
- [Audio Unit PlugIn Technology](https://developer.apple.com/documentation/audiounit) by Apple
- [Juce LV2 interface](http://www.falktx.com) by Filipe Coelho

## Status
What works:
- Very close to full support for pd (including all GUI objects, undo/redo, copy/paste, saving, loading, console, object properties, drawing functions, audio and MIDI I/O, help files)
- Most ELSE and cyclone library objects work
- LV2, AU and VST3 formats available, tested on Windows (x64), Mac (ARM/x64) and Linux (ARM/x64), also works as AU MIDI processor for Logic
- Receive 512 DAW parameters using [receive param1], [receive param2], etc. 
- Receive DAW playhead position, tempo and more using the [playhead] abstraction

Known issues:
- Tabs sometimes close when closing patch editor
- Broken objects:
  - text define
- There may still be some more bugs

## Roadmap

v0.5:
- Grid
- Tidy up feature
- Command line arguments
- Ensure all object properties are supported
- Expand pddocs with more descriptions


Future plans:
- Support for GEM or Ofelia

Please contact me if you wish to contribute, I could use some help! Bug reports are also appricated, they help me to get to a stable version much faster.
