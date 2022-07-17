#!/bin/bash

OGGVERSION=1.3.5
VORBISVERSION=1.3.7
FLACVERSION=1.3.3
OPUSVERSION=1.3.1
SNDFILE_VERSION=1.1.0

JOBS=8
SNDFILENAME=libsndfile-$SNDFILE_VERSION
OGG_INCDIR="$(pwd)/libogg-$OGGVERSION/include"
OGG_LIBDIR="$(pwd)/libogg-$OGGVERSION/src/.libs"

if [[ "$OSTYPE" == "darwin"* ]]; then
FLAGS="-arch arm64 -arch x86_64"
else
FLAGS="-fPIC"
fi

mkdir -p fluidsynth
mkdir -p fluidsynth/lib
mkdir -p fluidsynth/include

cd fluidsynth/lib
NUM_LIBS=$(ls | wc -l)
if [[ NUM_LIBS -eq 7 ]]; then
  # Fluidsynth was built already
  exit 0
fi

cd ../..

mkdir -p work
cd work

rm -p output.log 2> /dev/null
touch output.log
# libogg
echo "   -- Building libogg"
if curl --silent -LO https://downloads.xiph.org/releases/ogg/libogg-$OGGVERSION.tar.gz; then
    tar zxvf libogg-$OGGVERSION.tar.gz
    cd libogg-$OGGVERSION
    ./configure --disable-shared CC="gcc ${FLAGS}" CXX="g++ ${FLAGS}" CPP="gcc -E"  CXXCPP="g++ -E"
    make -j$JOBS
    cd ..
fi

# libvorbis
echo "   -- Building libvorbis"
if curl --silent -LO https://downloads.xiph.org/releases/vorbis/libvorbis-$VORBISVERSION.tar.gz; then
    tar zxvf libvorbis-$VORBISVERSION.tar.gz
    cd libvorbis-$VORBISVERSION
    ./configure --disable-shared --with-ogg-includes=$OGG_INCDIR --with-ogg-libraries=$OGG_LIBDIR CC="gcc ${FLAGS}" CXX="g++ ${FLAGS}" CPP="gcc -E"  CXXCPP="g++ -E"
    make -j$JOBS
cd ..
fi

# libFLAC
echo "   -- Building libflac"
if curl --silent -LO https://downloads.xiph.org/releases/flac/flac-$FLACVERSION.tar.xz; then
    unxz flac-$FLACVERSION.tar.xz
    tar xvf flac-$FLACVERSION.tar
    ls
    cd flac-$FLACVERSION
    ./configure --enable-static --disable-shared --with-ogg-includes=$OGG_INCDIR --with-ogg-libraries=$OGG_LIBDIR CC="gcc ${FLAGS}" CXX="g++ ${FLAGS}" CPP="gcc -E"  CXXCPP="g++ -E"  >> output.log 2>&1
    make -j$JOBS >> output.log 2>&1
    cd ..
fi

# libopus
echo "   -- Building libopus"
if curl --silent -LO https://archive.mozilla.org/pub/opus/opus-$OPUSVERSION.tar.gz; then
    tar zxvf opus-$OPUSVERSION.tar.gz >> output.log 2>&1
    cd opus-$OPUSVERSION
    ./configure --disable-shared --disable-rtcd CC="gcc ${FLAGS}" CXX="g++ ${FLAGS}" CPP="gcc -E"  CXXCPP="g++ -E"
    make -j$JOBS
    cd ..
fi

# libsndfile

export FLAC_CFLAGS="-I$(pwd)/flac-$FLACVERSION/include"
export FLAC_LIBS="$(pwd)/flac-$FLACVERSION/src/libFLAC/.libs/libFLAC.a"
export OGG_CFLAGS="-I$(pwd)/libogg-$OGGVERSION/include"
export OGG_LIBS="$(pwd)/libogg-$OGGVERSION/src/.libs/libogg.a"
export VORBIS_CFLAGS="-I$(pwd)/libvorbis-$VORBISVERSION/include"
export VORBIS_LIBS="$(pwd)/libvorbis-$VORBISVERSION/lib/.libs/libvorbis.a"
export VORBISENC_CFLAGS="-I$(pwd)/libvorbis-$VORBISVERSION/include"
export VORBISENC_LIBS="$(pwd)/libvorbis-$VORBISVERSION/lib/.libs/libvorbisenc.a"
export OPUS_CFLAGS="-I$(pwd)/opus-$OPUSVERSION/include"
export OPUS_LIBS="$(pwd)/opus-$OPUSVERSION/.libs/libopus.a"

echo "   -- Building libsndfile"
if curl --silent -LO https://github.com/libsndfile/libsndfile/releases/download/$SNDFILE_VERSION/$SNDFILENAME.tar.xz; then
    unxz $SNDFILENAME.tar.xz >> output.log 2>&1
    tar xvf $SNDFILENAME.tar >> output.log 2>&1
    cd $SNDFILENAME
    ./configure --enable-static --disable-alsa --disable-mpeg --disable-full-suite CC="gcc ${FLAGS}" CXX="g++ ${FLAGS}" CPP="gcc -E"  CXXCPP="g++ -E"
    make
    cd ..
fi

echo "   -- Building fluidsynth"
rm -rf fluidsynth
cp -rf ../../Libraries/ELSE/sfont~/fluidsynth ./fluidsynth
cd fluidsynth
mkdir -p build
cd build
cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_OSX_DEPLOYMENT_TARGET="10.11" -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -Denable-libsndfile=1 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-network=0 -Denable-pulseaudio=0 -Denable-dsound=0 -Denable-sdl2=0 -Denable-coreaudio=0 -Denable-coremidi=0 -Denable-framework=0 -Denable-threads=1 -Denable-openmp=0 -Denable-alsa=0 -Denable-pkgconfig=0 -DBUILD_SHARED_LIBS=0 >> output.log
cmake --build . --target libfluidsynth
cd ..
cd ..

cp ./fluidsynth/build/src/libfluidsynth.a ../fluidsynth/lib/libfluidsynth.a
cp -rf ./fluidsynth/build/include/* ../fluidsynth/include/
cp -rf ./fluidsynth/include/* ../fluidsynth/include/
cp $SNDFILENAME/src/.libs/libsndfile.a ../fluidsynth/lib/libsndfile.a
cp $OPUS_LIBS ../fluidsynth/lib/libopus.a
cp $VORBIS_LIBS ../fluidsynth/lib/libvorbis.a
cp $VORBISENC_LIBS ../fluidsynth/lib/libvorbisenc.a
cp $OGG_LIBS ../fluidsynth/lib/libogg.a
cp $FLAC_LIBS ../fluidsynth/lib/libFLAC.a

cd ..
#rm -rf work