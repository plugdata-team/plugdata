# FluidLite

[![License: LGPL-2.1](https://img.shields.io/badge/License-LGPL--2.1-brightgreen.svg)](https://opensource.org/licenses/LGPL-2.1)
[![Travis-CI Status](https://travis-ci.com/katyo/fluidlite.svg?branch=master)](https://travis-ci.com/katyo/fluidlite)

FluidLite (c) 2016 Robin Lobel

FluidLite is a very light version of FluidSynth
designed to be hardware, platform and external dependency independent.
It only uses standard C libraries.

It also adds support for SF3 files (SF2 files compressed with ogg vorbis)
and an additional setting to remove the constraint of channel 9 (drums):
fluid_settings_setstr(settings, "synth.drums-channel.active", "no");
you can still select bank 128 on any channel to use drum kits.

FluidLite keeps very minimal functionalities (settings and synth),
therefore MIDI file reading, realtime MIDI events and audio output must be
implemented externally.

## Configuration

By default SF3 support is disabled. To enable it use `-DENABLE_SF3=YES` with cmake.

Alternatively it can be configured to use [stb_vorbis](https://github.com/nothings/stb)
to decompress SF3 instead of Xiph's [libogg](https://github.com/xiph/ogg)/[libvorbis](https://github.com/xiph/vorbis).
You can pass `-DSTB_VORBIS=YES` to cmake to do it.

You can run cmake to configure and build the sources, and also to install the
compiled products. Here are some examples

To configure the sources with debug symbols:

~~~
$ cmake -S <source-directory> -B <build-directory> -DCMAKE_BUILD_TYPE=Debug
~~~

To configure the sources with an install prefix:

~~~
$ cmake -S <source-directory> -B <build-directory> -DCMAKE_INSTALL_PREFIX=/usr/local
---
$ cmake -S <source-directory> -B <build-directory> -DCMAKE_INSTALL_PREFIX=${HOME}/FluidLite
~~~

To configure the sources with additional dependency search paths:

~~~
$ cmake -S <source-directory> -B <build-directory> -DCMAKE_PREFIX_PATH=${HOME}/tests
~~~

To build (compile) the sources:

~~~
$ cmake --build <build-directory>
~~~

To install the compiled products (needs cmake 3.15 or newer):

~~~
$ cmake --install <build-directory>
~~~

Here is a bash script that you can customize/use on Linux:

~~~shell
#!/bin/bash
CMAKE="${HOME}/Qt/Tools/CMake/bin/cmake"
SRC="${HOME}/Projects/FluidLite"
BLD="${SRC}-build"
${CMAKE} -S ${SRC} -B ${BLD} \
    -DFLUIDLITE_BUILD_STATIC:BOOL="1" \
    -DFLUIDLITE_BUILD_SHARED:BOOL="1" \
    -DENABLE_SF3:BOOL="1" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_SKIP_RPATH:BOOL="0" \
    -DCMAKE_INSTALL_LIBDIR="lib" \
    -DCMAKE_INSTALL_PREFIX=$HOME/FluidLite \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL="1" \
    $*
read -p "Cancel now or pulse <Enter> to build"
${CMAKE} --build $BLD
read -p "Cancel now or pulse <Enter> to install"
${CMAKE} --install $BLD
~~~

See also the [complete cmake documentation](https://cmake.org/cmake/help/latest/manual/cmake.1.html) for more information.

## Usage

Here is the source code of the example that resides in example/src/main.c

```c
#include <stdlib.h>
#include <stdio.h>

#include "fluidlite.h"

#define SAMPLE_RATE 44100
#define SAMPLE_SIZE sizeof(float)
#define NUM_FRAMES SAMPLE_RATE
#define NUM_CHANNELS 2
#define NUM_SAMPLES (NUM_FRAMES * NUM_CHANNELS)

int main() {
    fluid_settings_t* settings = new_fluid_settings();
    fluid_synth_t* synth = new_fluid_synth(settings);
    fluid_synth_sfload(synth, "soundfont.sf2", 1);

    float* buffer = calloc(SAMPLE_SIZE, NUM_SAMPLES);

    FILE* file = fopen("float32output.pcm", "wb");

    fluid_synth_noteon(synth, 0, 60, 127);
    fluid_synth_write_float(synth, NUM_FRAMES, buffer, 0, NUM_CHANNELS, buffer, 1, NUM_CHANNELS);
    fwrite(buffer, SAMPLE_SIZE, NUM_SAMPLES, file);

    fluid_synth_noteoff(synth, 0, 60);
    fluid_synth_write_float(synth, NUM_FRAMES, buffer, 0, NUM_CHANNELS, buffer, 1, NUM_CHANNELS);
    fwrite(buffer, SAMPLE_SIZE, NUM_SAMPLES, file);

    fclose(file);

    free(buffer);

    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
}
```

There is also a cmake project file that you may use to build the example, or to
model your own projects:

~~~cmake
cmake_minimum_required(VERSION 3.1)
project(fluidlite-test LANGUAGES C)

#1. To find an installed fluidlite with pkg-config:
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(fluidlite REQUIRED fluidlite)

#2. To find an installed fluidlite with cmake only:
#find_package(fluidlite REQUIRED)

#3. using a subdirectory (for instance, a git submodule):
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite)
include(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite/aliases.cmake)

add_executable(${PROJECT_NAME}
    src/main.c
)

if(UNIX AND NOT APPLE)
    find_library(MATH_LIB m)
endif()

# if you include 'aliases.cmake' or after find_package(fluidlite),
# you get two targets defined, that you may link directly here:
# 1. 'fluidlite::fluidlite-static' is the static library
# 2. 'fluidlite::fluidlite' is the shared dynamic library
target_link_libraries(${PROJECT_NAME} PRIVATE
    fluidlite::fluidlite-static
    ${MATH_LIB}
)
~~~

Warning! this repository contains symlinks. If you are a Windows user and this is new for you, please [learn about this feature in Windows](https://blogs.windows.com/windowsdeveloper/2016/12/02/symlinks-windows-10/) and [git for windows](https://github.com/orgs/community/discussions/23591).
