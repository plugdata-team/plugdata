#!/usr/bin/make -f

.DEFAULT_GOAL := config

# If this is set to true (via the environment) then CRC checking will be
# disabled in libogg giving fuzzers a better chance at finding something.
disable_ogg_crc ?= false

# Build libsndfile as a dynamic/shared library, but statically link to
# libFLAC, libogg, libopus and libvorbis

ogg_version = libogg-1.3.5
vorbis_version = libvorbis-1.3.7
flac_version = flac-1.3.4
opus_version = opus-1.3.1

sndfile_version = 1.1.0
sndfile_name = libsndfile-$(sndfile_version)

root_dir = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
fluidsynth_dir = $(root_dir)/fluidsynth
#-------------------------------------------------------------------------------
# Code follows.

working_dir = fluidsynth_deps

stamp_dir = $(working_dir)/Stamp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	FLAGS="-arch arm64 -arch x86_64 -O3 -mmacosx-version-min=10.11"
else
	FLAGS="-fPIC -O3"
endif

build_dir = $(shell pwd)/fluidsynth
config_options = --prefix=$(build_dir) --disable-option-checking --disable-shared --disable-dependency-tracking --enable-option-checking CPP="gcc -E"  CXXCPP="g++ -E"
sndfile_options = $(config_options) --disable-sqlite --disable-alsa --disable-mpeg --disable-full-suite
fluidsynth_options = -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_OSX_DEPLOYMENT_TARGET="10.11" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -Denable-libsndfile=1 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-network=0 -Denable-pulseaudio=0 -Denable-dsound=0 -Denable-sdl2=0 -Denable-coreaudio=0 -Denable-coremidi=0 -Denable-framework=0 -Denable-threads=1 -Denable-openmp=0 -Denable-alsa=0 -Denable-pkgconfig=0 -DBUILD_SHARED_LIBS=0

pwd = $(shell pwd)

help :
	@echo
	@echo "This script will build libsndfile as a dynamic/shared library but statically linked"
	@echo "to libFLAC, libogg and libvorbis. It should work on Linux and Mac OS X. It might"
	@echo "work on Windows with a correctly set up MinGW."
	@echo
	@echo "It requires all the normal build tools require to build libsndfile."
	@echo

config : $(working_dir)/Stamp/install-libs

clean :
	rm -rf $(working_dir)/flac-* $(working_dir)/libogg-* $(working_dir)/libvorbis-* $(working_dir)/opus-*
	rm -rf $(working_dir)/bin $(working_dir)/include $(working_dir)/lib $(working_dir)/share
	rm -f $(working_dir)/Stamp/install $(working_dir)/Stamp/extract $(working_dir)/Stamp/build-ogg

$(working_dir)/Stamp/install-libs:
	mkdir -p $(stamp_dir)
	(cd $(working_dir)/$(ogg_version) && CFLAGS=$(FLAGS) ./configure $(config_options) || 1 && make all install)
	(cd $(working_dir)/$(vorbis_version) && CFLAGS=$(FLAGS) ./configure --disable-oggtest $(config_options) && make all install)
	(cd $(working_dir)/$(flac_version) && CFLAGS=$(FLAGS) ./configure $(config_options) --disable-thorough-tests --disable-cpplibs  --disable-examples  --disable-oggtest --disable-doxygen-docs --disable-xmms-plugin && make all install)
	(cd $(working_dir)/$(opus_version) && CFLAGS=$(FLAGS) ./configure $(config_options) --disable-rtcd --disable-extra-programs --disable-doc && make all install)
	(cd $(working_dir)/$(sndfile_name) && CFLAGS=$(FLAGS) ./configure $(sndfile_options) && make all install)
	(cd $(working_dir)/fluidsynth && mkdir -p Build && cd Build && cmake ${fluidsynth_options} .. && cmake --build . --target libfluidsynth)
	(cd $(working_dir) && cp ./fluidsynth/Build/src/libfluidsynth.a $(build_dir)/lib/libfluidsynth.a && cp -rf ./fluidsynth/Build/include/* $(build_dir)/include && cp -rf ./fluidsynth/include/* $(build_dir)/include)

	touch $@
