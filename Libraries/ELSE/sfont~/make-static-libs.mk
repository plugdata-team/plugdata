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

ogg_tarball = $(ogg_version).tar.xz
vorbis_tarball = $(vorbis_version).tar.xz
flac_tarball = $(flac_version).tar.xz
opus_tarball = $(opus_version).tar.gz
sndfile_tarball = $(sndfile_name).tar.xz
download_url = http://downloads.xiph.org/releases/
working_dir = fluidsynth_deps
tarball_dir = $(working_dir)/Tarballs
stamp_dir = $(working_dir)/Stamp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	FLAGS="-arch arm64 -arch x86_64 -O3"
endif
ifeq ($(UNAME_S),Linux)
	FLAGS="-fPIC -O3"
endif

build_dir = $(shell pwd)/fluidsynth
config_options = --prefix=$(build_dir) --disable-shared --disable-doc --disable-extra-programs --enable-option-checking CPP="gcc -E"  CXXCPP="g++ -E"
sndfile_options = $(config_options) --disable-sqlite --disable-alsa --disable-mpeg --disable-full-suite
fluidsynth_options = -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_OSX_DEPLOYMENT_TARGET="10.11" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -Denable-libsndfile=1 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-network=0 -Denable-pulseaudio=0 -Denable-dsound=0 -Denable-sdl2=0 -Denable-coreaudio=0 -Denable-coremidi=0 -Denable-framework=0 -Denable-threads=1 -Denable-openmp=0 -Denable-alsa=0 -Denable-pkgconfig=0 -DBUILD_SHARED_LIBS=0

pwd = $(shell pwd)

help :
	@echo
	@echo "This script will build libsndfile as a dynamic/shared library but statically linked"
	@echo "to libFLAC, libogg and libvorbis. It should work on Linux and Mac OS X. It might"
	@echo "work on Windows with a correctly set up MinGW."
	@echo
	@echo "It requires all the normal build tools require to build libsndfile plus wget."
	@echo

config : $(working_dir)/Stamp/install-libs

clean :
	rm -rf $(working_dir)/flac-* $(working_dir)/libogg-* $(working_dir)/libvorbis-* $(working_dir)/opus-*
	rm -rf $(working_dir)/bin $(working_dir)/include $(working_dir)/lib $(working_dir)/share
	rm -f $(working_dir)/Stamp/install $(working_dir)/Stamp/extract $(working_dir)/Stamp/build-ogg

$(working_dir)/Stamp/init :
	mkdir -p $(stamp_dir) $(tarball_dir)
	touch $@

$(working_dir)/Tarballs/$(flac_tarball) : $(working_dir)/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)flac/$(flac_tarball) -O $(flac_tarball))
	touch $@

$(working_dir)/Tarballs/$(ogg_tarball) : $(working_dir)/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)ogg/$(ogg_tarball) -O $(ogg_tarball))
	touch $@

$(working_dir)/Tarballs/$(vorbis_tarball) : $(working_dir)/Stamp/init
	(cd $(tarball_dir) && wget $(download_url)vorbis/$(vorbis_tarball) -O $(vorbis_tarball))
	touch $@

$(working_dir)/Tarballs/$(opus_tarball) : $(working_dir)/Stamp/init
	(cd $(tarball_dir) && wget https://archive.mozilla.org/pub/opus/$(opus_tarball) -O $(opus_tarball))
	touch $@

$(working_dir)/Tarballs/$(sndfile_tarball) : $(working_dir)/Stamp/init
	(cd $(tarball_dir) && wget https://github.com/libsndfile/libsndfile/releases/download/$(sndfile_version)/$(sndfile_name).tar.xz -O $(sndfile_tarball))
	touch $@

$(working_dir)/Stamp/tarballs : $(working_dir)/Tarballs/$(flac_tarball) $(working_dir)/Tarballs/$(ogg_tarball) $(working_dir)/Tarballs/$(vorbis_tarball) $(working_dir)/Tarballs/$(opus_tarball) $(working_dir)/Tarballs/$(sndfile_tarball)
	touch $@


$(working_dir)/Stamp/extract : $(working_dir)/Stamp/tarballs
	(cd $(working_dir) && tar xf Tarballs/$(ogg_tarball))
	(cd $(working_dir) && tar xf Tarballs/$(flac_tarball))
	(cd $(working_dir) && tar xf Tarballs/$(vorbis_tarball))
	(cd $(working_dir) && tar xf Tarballs/$(opus_tarball))
	(cd $(working_dir) && tar xf Tarballs/$(sndfile_tarball))
	(cd $(working_dir) && cp -r $(fluidsynth_dir) ./fluidsynth)
	touch $@

$(working_dir)/Stamp/build-ogg : $(working_dir)/Stamp/extract
	(cd $(working_dir) && tar xf Tarballs/$(ogg_tarball))
	(cd $(working_dir)/$(ogg_version) && CFLAGS=$(FLAGS) ./configure $(config_options) && make all install)
	touch $@

$(working_dir)/Stamp/install-libs : $(working_dir)/Stamp/extract $(working_dir)/Stamp/build-ogg
	(cd $(working_dir)/$(vorbis_version) && CFLAGS=$(FLAGS) ./configure $(config_options) && make all install)
	(cd $(working_dir)/$(flac_version) && CFLAGS=$(FLAGS) ./configure $(config_options) && make all install)
	(cd $(working_dir)/$(opus_version) && CFLAGS=$(FLAGS) ./configure $(config_options) && make all install)
	(cd $(working_dir)/$(sndfile_name) && CFLAGS=$(FLAGS) ./configure $(sndfile_options) && make all install)
	(cd $(working_dir)/fluidsynth && mkdir -p Build && cd Build && cmake ${fluidsynth_options} .. && cmake --build . --target libfluidsynth)
	(cd $(working_dir) && cp ./fluidsynth/Build/src/libfluidsynth.a ../fluidsynth/lib/libfluidsynth.a && cp -rf ./fluidsynth/Build/include/* ../fluidsynth/include/ && cp -rf ./fluidsynth/include/* ../fluidsynth/include/)

	touch $@

