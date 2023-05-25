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

<p align="middle">
  <img width="1082" alt="cipher" src="https://github.com/plugdata-team/plugdata/assets/44585538/0eb0c97e-bf68-4252-a297-0f7c7a7ff747">

  <img width="1115" alt="amaranth" src="https://github.com/plugdata-team/plugdata/assets/44585538/33beb62b-1e84-4f0d-81b8-4c130c6c5cf1">
</p>

plugdata is a plugin wrapper for Pure Data, featuring a new GUI made with JUCE. This is still a WIP, and there are probably still some bugs. By default, it ships with the ELSE collection of externals and abstractions. The aim is to provide a more comfortable patching experience for a large selection of DAWs. It can also be used as a standalone replacement for pure-data.

Join the Discord here, for sharing patches, reporting issues or requesting features: https://discord.gg/eT2RxdF9Nq

<p align="middle">
<img width="431" alt="LIRA-8" src="https://github.com/plugdata-team/plugdata/assets/44585538/1eb2e36f-6552-4caa-8e7b-2cd6adf03917">
</p>

## Installation

**-Windows:**
- Get the installer from the latest [official release](https://github.com/plugdata-team/plugdata/tags)

**-MacOS:**
- Option 1: Get the installer from the latest [official release](https://github.com/plugdata-team/plugdata/tags)
- Option 2: Install the [homebrew cask](https://formulae.brew.sh/cask/plugdata). I do not maintain this so it may be outdated. 

**-Linux:**
- Option 1: [OBS repository/packages](https://software.opensuse.org//download.html?project=home%3Aplugdata&package=plugdata)
  The repository contains the packages plugdata, plugdata-vst3 and plugdata-lv2 (lv2-plugdata and vst3-plugdata on some distros). If you get a package without the repo, it will only contain the standalone version.
    - Plugin versions can be found here:
      - Deb/Ubuntu: https://software.opensuse.org/package/plugdata-vst3 and https://software.opensuse.org/package/plugdata-lv2
      - RPM based distros: https://software.opensuse.org/package/vst3-plugdata and https://software.opensuse.org/package/lv2-plugdata
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

## Adding your own externals
You can use externals inside plugdata's plugin version by recompiling the externals along with plugdata. You can doing that by making the following modification to plugdata:

-  Add your sources to the "externals" target inside Libraries/CMakeLists.txt. If you want to be lazy, you can also just put the source inside the Libraries/ELSE/Source folder, all .c files in that folder will be compiled automatically.
-  In Libraries/libpd/x_libpd_multi.c, add the setup function for your external. The best place to call your setup function is inside libpd_init_pdlua. libpd_init_else and libpd_init_cyclone will also work, but it has the side-effect that the externals will also be available under the else/* and cyclone/* prefix.

**Important:**
- plugdata requires cmake 3.21 or later
- Ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the plugdata repository .
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
- [FluidLite](https://github.com/divideconcept/FluidLite) by divideconcept, based on [Fluidsynth](https://github.com/FluidSynth/fluidsynth)

## Status
What works:
- Nearly complete support for pd
- Most ELSE and cyclone library objects work
- VST3, LV2, CLAP and AU format available, tested on Windows (x86/x64), Mac (arm64/x64) and Linux (arm64/armhf/x64)
- Receive 512 DAW parameters using the [param] abstraction
- Receive DAW playhead position, tempo and more using the [playhead] abstraction

Known issues:
- Broken ELSE objects: See [#174](https://github.com/plugdata-team/plugdata/issues/174)
- There may still be some more bugs
