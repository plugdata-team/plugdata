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
ARCHS="-arch arm64 -arch x86_64"
else
ARCHS=""
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
curl -LO https://downloads.xiph.org/releases/ogg/libogg-$OGGVERSION.tar.gz >> output.log 2>&1
tar zxvf libogg-$OGGVERSION.tar.gz >> output.log 2>&1
cd libogg-$OGGVERSION
./configure --disable-shared CC="gcc ${ARCHS}" CXX="g++ ${ARCHS}" CPP="gcc -E"  CXXCPP="g++ -E" >> output.log 2>&1
make -j$JOBS >> output.log 2>&1
cd ..


# libvorbis
echo "   -- Building libvorbis"
curl -LO https://downloads.xiph.org/releases/vorbis/libvorbis-$VORBISVERSION.tar.gz >> output.log 2>&1
tar zxvf libvorbis-$VORBISVERSION.tar.gz >> output.log 2>&1
cd libvorbis-$VORBISVERSION
./configure --disable-shared --with-ogg-includes=$OGG_INCDIR --with-ogg-libraries=$OGG_LIBDIR CC="gcc ${ARCHS}" CXX="g++ ${ARCHS}" CPP="gcc -E"  CXXCPP="g++ -E" >> output.log 2>&1
make -j$JOBS >> output.log 2>&1
cd ..


# libFLAC
echo "   -- Building libflac"
curl -LO https://downloads.xiph.org/releases/flac/flac-$FLACVERSION.tar.xz >> output.log 2>&1
unxz flac-$FLACVERSION.tar.xz >> output.log 2>&1
tar xvf flac-$FLACVERSION.tar >> output.log 2>&1
cd flac-$FLACVERSION
./configure --enable-static --disable-shared --with-ogg-includes=$OGG_INCDIR --with-ogg-libraries=$OGG_LIBDIR CC="gcc ${ARCHS}" CXX="g++ ${ARCHS}" CPP="gcc -E"  CXXCPP="g++ -E"  >> output.log 2>&1
make -j$JOBS >> output.log 2>&1
cd ..

# libopus
echo "   -- Building libopus"
curl -LO https://archive.mozilla.org/pub/opus/opus-$OPUSVERSION.tar.gz >> output.log 2>&1
tar zxvf opus-$OPUSVERSION.tar.gz >> output.log 2>&1
cd opus-$OPUSVERSION
./configure --disable-shared CC="gcc ${ARCHS}" CXX="g++ ${ARCHS}" CPP="gcc -E"  CXXCPP="g++ -E" >> output.log 2>&1
make -j$JOBS >> output.log 2>&1
cd ..

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
curl -LO https://github.com/libsndfile/libsndfile/releases/download/$SNDFILE_VERSION/$SNDFILENAME.tar.xz >> output.log 2>&1
unxz $SNDFILENAME.tar.xz >> output.log 2>&1
tar xvf $SNDFILENAME.tar >> output.log 2>&1
mkdir -p $SNDFILENAME/build
cd $SNDFILENAME/build
cmake .. -DBUILD_SHARED_LIBS=0 -DALSA_FOUND=0 -DBUILD_REGTEST=0 -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" >> output.log 2>&1
cmake --build . --target sndfile >> output.log 2>&1
cd ../..

echo "   -- Building fluidsynth"
rm -rf fluidsynth
cp -rf ../../Libraries/ELSE/sfont~/fluidsynth ./fluidsynth
mkdir -p fluidsynth/build
cd fluidsynth/build
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -Denable-libsndfile=1 -Denable-aufile=0 -Denable-dbus=0 -Denable-ipv6=0 -Denable-jack=0 -Denable-ladspa=0 -Denable-midishare=0 -Denable-opensles=0 -Denable-oboe=0 -Denable-oss=0 -Denable-readline=0 -Denable-winmidi=0 -Denable-waveout=0 -Denable-network=0 -Denable-pulseaudio=0 -Denable-dsound=0 -Denable-sdl2=0 -Denable-coreaudio=0 -Denable-coremidi=0 -Denable-framework=0 -Denable-threads=1 -Denable-openmp=0 -Denable-alsa=0 -Denable-pkgconfig=0 -DBUILD_SHARED_LIBS=0 
cmake --build . --target libfluidsynth >> output.log 2>&1
cd ../..

cp ./fluidsynth/build/src/libfluidsynth.a ../fluidsynth/lib/libfluidsynth.a
cp -rf ./fluidsynth/build/include/* ../fluidsynth/include/
cp -rf ./fluidsynth/include/* ../fluidsynth/include/
cp $SNDFILENAME/build/libsndfile.a ../fluidsynth/lib/libsndfile.a
cp $OPUS_LIBS ../fluidsynth/lib/libopus.a
cp $VORBIS_LIBS ../fluidsynth/lib/libvorbis.a
cp $VORBISENC_LIBS ../fluidsynth/lib/libvorbisenc.a
cp $OGG_LIBS ../fluidsynth/lib/libogg.a
cp $FLAC_LIBS ../fluidsynth/lib/libFLAC.a

cd ..
rm -rf work