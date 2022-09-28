<p align="center">
  <a href="https://github.com/timothyschoen/PlugData/wiki">
    <img src="https://user-images.githubusercontent.com/44585538/192803827-b88dcceb-4933-4a59-8a02-7800138a2bf7.png" alt="Logo" width=96 height=96>
  </a>
  <h1 align="center">PlugData</h1>
  <p align="center">
    Plugin wrapper around Pure Data to allow patching in a wide selection of DAWs.
  </p>
  <p align="center">
    <a href="https://github.com/timothyschoen/PlugData/actions/workflows/cmake.yml"><img src="https://github.com/timothyschoen/PlugData/actions/workflows/cmake.yml/badge.svg" alt="Workflows"></a>
  </p>
  <p align="center">
    <a href="https://github.com/timothyschoen/PlugData/releases/latest"><img src="https://img.shields.io/github/downloads/timothyschoen/PlugData/total.svg?colorB=007ec6" alt="Downloads"></a>
    <a href="https://github.com/timothyschoen/PlugData/releases/latest"><img src="https://img.shields.io/github/release/timothyschoen/PlugData.svg?include_prereleases" alt="Release"></a>
    <a href="https://github.com/timothyschoen/PlugData/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-GPL--v3-blue.svg" alt="License"></a>
  </p>
</p>

<img width="1054" alt="Screenshot 2022-06-29 at 00 14 03" src="https://user-images.githubusercontent.com/44585538/176311724-b6e2e92b-1a0b-4f8d-a328-fc1c9cf0666c.png">

<img width="1054" alt="Screenshot 2022-06-29 at 00 18 35" src="https://user-images.githubusercontent.com/44585538/176312569-dca8f9f1-c4c7-4176-82c1-17378daf17fd.png">

PlugData is a plugin wrapper for Pure Data, featuring a new GUI made with JUCE. This is still a WIP, and there are probably still some bugs. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

Join the Discord here, for sharing patches, reporting issues or requesting features: https://discord.gg/eT2RxdF9Nq

## Installation

Linux users are advised to install PlugData using the repositories [here](https://software.opensuse.org//download.html?project=home%3Aplugdata&package=plugdata). Arch users can also install [the AUR package](https://aur.archlinux.org/packages/plugdata-git).

For Mac and Windows, you can find the last stable-ish release in the releases section on the right, or download a recent build from the actions tab.

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
- [Inter](https://rsms.me/inter/) font by Rasmus Andersson
- [Kiwi](https://github.com/Musicoll/Kiwi) by Eliott Paris, Pierre Guillot and Jean Millot
- [LV2 PlugIn Technology](http://lv2plug.in) by Steve Harris, David Robillard and others
- [VST PlugIn Technology](https://www.steinberg.net/en/company/technologies/vst3.html) by Steinberg Media Technologies
- [Audio Unit PlugIn Technology](https://developer.apple.com/documentation/audiounit) by Apple

## Status
What works:
- Nearly complete support for pd
- Most ELSE and cyclone library objects work
- LV2, AU and VST3 formats available, tested on Windows (x64), Mac (ARM/x64) and Linux (ARM/x64)
- Receive 512 DAW parameters using the [param] abstraction
- Receive DAW playhead position, tempo and more using the [playhead] abstraction

Known issues:
- Broken ELSE objects: 
  - [oscope~]
  - [function]
  - [bicoeff]
  - [messbox]
  - [note]
  - [canvas.active]
  - [canvas.mouse]
  - [canvas.vis]
  - [canvas.zoom]
- There may still be some more bugs

## Roadmap

These are all the things I'm hoping to implement before version 0.7
I might release some smaller versions inbetween if any serious bugs are being found.

FEATURES:
- Object discovery panel: a place where all objects (with descriptions) are listed by category
- Search in patch
- Autosave

PD COMPATIBILITY:
- Fix broken objects
- Some properties are not updated in the panel when set with a message

OTHER:
- Expand documentation with more object descriptions and inlet/outlet hover messages
- Restructure build directory
- Clean up and document code
- Codesign executables for Windows
- Support for Gem and Ofelia


Please contact me if you wish to contribute, I could use some help! Bug reports are also appreciated, they help me to get to a stable version much faster.
