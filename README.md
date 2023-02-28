<p align="center">
  <a href="https://github.com/plugdata-team/plugdata/wiki">
    <img src="https://user-images.githubusercontent.com/44585538/199558692-df4fdd3f-b15b-4d88-82e8-d08ab7309c7c.png" alt="Logo" width=130 height=130>
  </a>
  <h1 align="center">plugdata</h1>
  <p align="center">
    Plugin wrapper around Pure Data to allow patching in a wide selection of DAWs.
  </p>
  <p align="center">
    <a href="https://github.com/plugdata-team/plugdata/actions/workflows/cmake.yml"><img src="https://github.com/plugdata-team/plugdata/actions/workflows/cmake.yml/badge.svg" alt="Workflows"></a>
  </p>
  <p align="center">
    <a href="https://github.com/plugdata-team/plugdata/releases/latest"><img src="https://img.shields.io/github/downloads/plugdata-team/plugdata/total.svg?colorB=007ec6" alt="Downloads"></a>
    <a href="https://github.com/plugdata-team/plugdata/releases/latest"><img src="https://img.shields.io/github/release/plugdata-team/plugdata.svg?include_prereleases" alt="Release"></a>
    <a href="https://github.com/plugdata-team/plugdata/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-GPL--v3-blue.svg" alt="License"></a>
  </p>
</p>

<img width="1046" alt="Screenshot 2023-01-16 at 15 00 08" src="https://user-images.githubusercontent.com/44585538/212699165-d5aa1bb8-f212-403c-9ea6-45114c6a42fa.png">


<img width="1046" alt="Screenshot 2023-01-10 at 15 45 01" src="https://user-images.githubusercontent.com/44585538/211581898-7f3b6079-369e-4ff7-930b-aaffd18a4c04.png">


plugdata is a plugin wrapper for Pure Data, featuring a new GUI made with JUCE. This is still a WIP, and there are probably still some bugs. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

Join the Discord here, for sharing patches, reporting issues or requesting features: https://discord.gg/eT2RxdF9Nq

## Installation

**-Windows:**
- Get the installer from the latest [official release](https://github.com/plugdata-team/plugdata/tags)

**-MacOS:**
- Option 1: Get the installer from the latest [official release](https://github.com/plugdata-team/plugdata/tags)
- Option 2: Install the [homebrew cask](https://formulae.brew.sh/cask/plugdata). I do not maintain this so it may be outdated. 

**-Linux:**
- Option 1: [OBS repository/packages](https://software.opensuse.org//download.html?project=home%3Aplugdata&package=plugdata)
  The repository contains the packages plugdata, plugdata-vst3 and plugdata-lv2 (lv2-plugdata and vst3-plugdata on some distros). If you get a package without the repo, it will only contain the standalone version.
- Option 2: Arch User Repository (Arch only).
  Has the [plugdata-bin](https://aur.archlinux.org/packages/plugdata-bin) package for the latest stable version, or the [plugdata-git](https://aur.archlinux.org/packages/plugdata-git) package for the latest experimental build.
- Option 3: Get the binaries from the latest [official release](https://github.com/plugdata-team/plugdata/tags)

If you have a GitHub account, you can also download a recent experimental build from the actions tab.

## Build

```
git clone --recursive https://github.com/plugdata-team/plugdata.git
cd plugdata
mkdir build && cd build
cmake .. (the generator can be specified using -G"Unix Makefiles", -G"XCode" or -G"Visual Studio 16 2019" -A x64)
cmake --build .
```

**Important:**
- Please ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the plugdata repository .
- On Linux, Juce framework requires to install dependencies, please refer to [Linux Dependencies.md](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) and use the full command.
- The CMake build system has been tested with *Unix Makefiles*, *XCode* and *Visual Studio 16 2019*.

## Credits
Supported by [Deskew Technologies](https://gigperformer.com)
- Logo by [Joshua A.C. Newman](https://glyphpress.com/talk/) 
- [Camomile](https://github.com/pierreguillot/Camomile) by Pierre Guillot
- [ELSE](https://github.com/porres/pd-else) by Alexandre Torres Porres
- [cyclone](https://github.com/porres/pd-cyclone) by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Torres Porres, Derek Kwan, Matt Barber and others (note: Cyclone is included to offer an easy entry point for Max users but ELSE contains several alternatives to objects in Cyclone and Pure Data Vanilla itself also has some alternatives. Not only that, but objects that weren't cloned into Cyclone also have alternatives in ELSE, see: [this](https://github.com/porres/pd-else/wiki/Cyclone-alternatives))
- [pd-lua](https://github.com/agraef/pd-lua) by Pierre Guillot and Albert Graef
- [Pure Data](https://puredata.info) by Miller Puckette and others
- [libpd](https://github.com/libpd/libpd) by the Peter Brinkmann, Dan Wilcox and others
- [Heavy/hvcc](https://github.com/Wasted-Audio/hvcc) originally by Enzien Audio, maintained and modernised by Wasted-Audio
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
  - [bicoeff]
  - [messbox]
  - [note]
- There may still be some more bugs
