<p align="center">
  <a href="https://github.com/plugdata-team/plugdata/wiki">
    <img src="https://github.com/plugdata-team/plugdata/assets/44585538/17062f59-bb1b-442e-a0ba-a96a74327460" alt="Logo" width=130 height=130>
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
  <p align="center">
    <a href="https://discord.gg/eT2RxdF9Nq"><img src="https://img.shields.io/discord/993531159956955256?label=discord&logo=discord" alt="Discord"></a>
  </p>
</p>

<p align="middle">
<img width="1093" alt="app" src="https://github.com/plugdata-team/plugdata/assets/44585538/78c1018d-9949-4f95-9041-f912597c921d">
<img width="1093" alt="darkmode" src="https://github.com/plugdata-team/plugdata/assets/44585538/8411f664-563f-449d-91a6-ca0221439307">
</p>

Introducing PlugData: a dynamic plugin wrapper for Pure Data, currently in development with an innovative GUI built using JUCE. While still a work in progress, expect some bugs as we refine the experience. PlugData comes equipped with the ELSE collection of externals and abstractions out of the box. Our goal? To enhance the patching experience across a variety of DAWs, offering seamless integration. Plus, it doubles as a standalone alternative to pure-data. Join our Discord community for patch sharing, issue reporting, and feature requests: https://discord.gg/eT2RxdF9Nq

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

You can also download a recent experimental build from our [website](https://plugdata.org/download.html)

## Build

```
git clone --recursive https://github.com/plugdata-team/plugdata.git
cd plugdata
mkdir build && cd build
cmake .. (the generator can be specified using -G"Unix Makefiles", -G"XCode" or -G"Visual Studio 16 2019" -A x64)
cmake --build .
```

**Important:**
- plugdata requires cmake 3.21 or later to build CLAP plugins. If you use an older version of cmake, CLAP plugin builds will be disabled.
- Ensure that the git submodules are initialized and updated! You can use the `--recursive` option while cloning or `git submodule update --init --recursive` in the plugdata repository .
- On Linux, Juce framework requires to install dependencies, please refer to [Linux Dependencies.md](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md) and use the full command.
- The CMake build system has been tested with *Unix Makefiles*, *XCode*, *Visual Studio 17 2022* and *Visual Studio 16 2019*

## Adding your own externals

To integrate externals into PlugData's plugin version, follow these steps:

Add External Sources:

Include your external's source files in the "externals" target within Libraries/CMakeLists.txt. Alternatively, you can place the source files in Libraries/ELSE/Source, as all .c files in this folder will be compiled automatically.
Modify Setup.cpp:

Navigate to Source/Pd/Setup.cpp.
Add the setup function for your external. The recommended location to call your setup function is inside libpd_init_pdlua. Alternatively, initialiseELSE and initialiseCyclone can also be used, but note that this might result in the externals being accessible under the else/* and cyclone/* prefix as well.
By following these steps, you can recompile PlugData along with your externals, enabling their usage within the plugin version. This customization expands the capabilities of PlugData, allowing for a more tailored and versatile patching experience.

## Corporate sponsors
<p align="center" style="display: flex; flex-direction: column; align-items: center; justify-content: center;">
  <a href="https://gigperformer.com/">
    <img src="https://github.com/plugdata-team/plugdata/assets/44585538/e0c37c87-05cc-4766-9dc4-a42ddbb8784f">
  </a>
  <a href="https://jb.gg/OpenSourceSupport">
    <img src="https://github.com/plugdata-team/plugdata/assets/44585538/fd9ae433-4913-411f-8ae0-c1a7c5f28c72">
  </a>
</p>

## Credits
- Logo by [Bas de Bruin](https://www.bdebruin.nl/), based on concept by [Joshua A.C. Newman](https://glyphpress.com/talk/) 
- [Camomile](https://github.com/pierreguillot/Camomile) by Pierre Guillot
- [ELSE](https://github.com/porres/pd-else) by Alexandre Torres Porres
- [cyclone](https://github.com/porres/pd-cyclone) by Krzysztof Czaja, Hans-Christoph Steiner, Fred Jan Kraan, Alexandre Torres Porres, Derek Kwan, Matt Barber and others (note: Cyclone is included to offer an easy entry point for Max users but ELSE contains several alternatives to objects in Cyclone and Pure Data Vanilla itself also has some alternatives. Not only that, but objects that weren't cloned into Cyclone also have alternatives in ELSE, see: [this](https://github.com/porres/pd-else/wiki/Cyclone-alternatives))
- [pd-lua](https://github.com/agraef/pd-lua) by Claude Heiland-Allen, Albert Graef, and others
- [Pure Data](https://puredata.info) by Miller Puckette and others
- [libpd](https://github.com/libpd/libpd) by the Peter Brinkmann, Dan Wilcox and others
- [Heavy/hvcc](https://github.com/Wasted-Audio/hvcc) originally by Enzien Audio, maintained and modernised by Wasted Audio
- [Juce](https://github.com/WeAreROLI/JUCE) by ROLI Ltd.
- [MoodyCamel](https://github.com/cameron314/concurrentqueue) by Cameron Desrochers
- [Inter](https://rsms.me/inter/) font by Rasmus Andersson
- [Kiwi](https://github.com/Musicoll/Kiwi) by Eliott Paris, Pierre Guillot and Jean Millot
- [FluidLite](https://github.com/divideconcept/FluidLite) by divideconcept, based on [Fluidsynth](https://github.com/FluidSynth/fluidsynth)

## Status

What Works:

- Extensive support for Pure Data (pd), with nearly complete functionality.
- Compatibility with most ELSE and Cyclone library objects.
- Availability in VST3, LV2, CLAP, and AU formats, extensively tested across various platforms including Windows (x86/x64), Mac (arm64/x64), and Linux (arm64/armhf/x64).
- Seamless integration with DAW parameters, enabling reception of up to 512 parameters using the versatile [param] abstraction.
- Access to DAW playhead position, tempo, and additional parameters facilitated by the [playhead] abstraction.

Known Issues:

- Some ELSE objects may currently be non-functional. Refer to #174 for details.
- Possible existence of additional bugs.
- Despite these known issues, PlugData offers a robust platform for patching and audio manipulation, with comprehensive support across multiple plugin formats - and platforms. Ongoing development aims to address any outstanding issues, ensuring an optimal user experience.