cmake_minimum_required(VERSION 3.1)

#------------------------------------------------------------------------------#
# LIBPD PROJECT IN C
#------------------------------------------------------------------------------#
project(libpd VERSION 1.0.0 LANGUAGES C)

if (MSVC)
    add_compile_definitions(PTW32_STATIC_LIB)
    add_subdirectory(pthread-win32)
    include_directories(pthread-win32)
endif()

add_subdirectory(JUCE)


set ( enable-aufile off CACHE BOOL "compile support for sound file output")
set ( BUILD_SHARED_LIBS off CACHE BOOL "Build a shared object or DLL")
set ( enable-dbus off CACHE BOOL "compile DBUS support (if it is available)")
set ( enable-ipv6  off CACHE BOOL "enable ipv6 support")
set ( enable-jack off CACHE BOOL "compile JACK support (if it is available)")
set ( enable-ladspa off CACHE BOOL "enable LADSPA effect units")
set ( enable-libinstpatch off CACHE BOOL "use libinstpatch (if available) to load DLS and GIG files")
set ( enable-libsndfile off CACHE BOOL "compile libsndfile support (if it is available)")
set ( enable-midishare off CACHE BOOL "compile MidiShare support (if it is available)")
set ( enable-opensles off CACHE BOOL "compile OpenSLES support (if it is available)")
set ( enable-oboe off CACHE BOOL "compile Oboe support (requires OpenSLES and/or AAudio)")
set ( enable-network off CACHE BOOL "enable network support (requires BSD sockets)")
set ( enable-oss off CACHE BOOL "compile OSS support (if it is available)")
set ( enable-dsound off CACHE BOOL "compile DirectSound support (if it is available)")
set ( enable-wasapi off CACHE BOOL "compile Windows WASAPI support (if it is available)")
set ( enable-waveout off CACHE BOOL "compile Windows WaveOut support (if it is available)")
set ( enable-winmidi off CACHE BOOL "compile Windows MIDI support (if it is available)")
set ( enable-sdl2 off CACHE BOOL "compile SDL2 audio support (if it is available)")
set ( enable-pulseaudio off CACHE BOOL "compile PulseAudio support (if it is available)")
set ( enable-pipewire off CACHE BOOL "compile PipeWire support (if it is available)")
set ( enable-readline on CACHE BOOL "compile readline lib line editing (if it is available)")
set ( enable-threads off CACHE BOOL "enable multi-threading support (such as parallel voice synthesis)")
set ( enable-openmp off CACHE BOOL "enable OpenMP support (parallelization of soundfont decoding, vectorization of voice mixing, etc.)")

set ( enable-lash off CACHE BOOL "compile LASH support (if it is available)")
set ( enable-alsa off CACHE BOOL "compile ALSA support (if it is available)")
set ( enable-systemd off CACHE BOOL "compile systemd support (if it is available)")
set ( enable-coreaudio  off CACHE BOOL "compile CoreAudio support (if it is available)")
set ( enable-coremidi off CACHE BOOL "compile CoreMIDI support (if it is available)")
set ( enable-framework off CACHE BOOL "create a Mac OSX style FluidSynth.framework")
set ( enable-dart off CACHE BOOL "compile DART support (if it is available)")

# Hide messages from fluidsynth
# There's many, and some could be wrongly interpreted as being about pure-data or PlugData

if(MSVC)
link_directories(ELSE/sfont~/libsndfile/lib/)
endif()

# check for libsndfile
include(ELSE/sfont~/FindLibSndFile.cmake)

if(LIBSNDFILE_FOUND)
message("-- Configuring fluidsynth")
function(message)
endfunction()


#add_subdirectory(ELSE/sfont~/fluidsynth)
#include_directories(${FluidSynth_BINARY_DIR}/include/)
#include_directories(ELSE/sfont~/fluidsynth/include/)

function(message)
        _message(${ARGN})
endfunction()
endif(LIBSNDFILE_FOUND)
#------------------------------------------------------------------------------#

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PD_PATH "pure-data/")
set(LIBPD_PATH "libpd/")
set(LIBPD_OUTPUT_DIRECTORY "./..")

#------------------------------------------------------------------------------#
# OPTIONS
#------------------------------------------------------------------------------#
option(PD_UTILS "Compile libpd utilities" OFF)
option(PD_EXTRA "Compile extras" ON)
option(PD_LOCALE "Set the LC_NUMERIC number format to the default C locale" ON)

#------------------------------------------------------------------------------#
# OUTPUT DIRECTORY
#------------------------------------------------------------------------------#
if(DEFINED LIBPD_OUTPUT_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIBPD_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBPD_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIBPD_OUTPUT_DIRECTORY})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${LIBPD_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${LIBPD_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${LIBPD_OUTPUT_DIRECTORY})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
endif()

#------------------------------------------------------------------------------#
# SOURCES
#------------------------------------------------------------------------------#
set(SOURCE_FILES)

# PURE DATA SOURCES
#------------------------------------------------------------------------------#
file(GLOB PD_SOURCES
    ${PD_PATH}/src/d_arithmetic.c
    ${PD_PATH}/src/d_array.c
    ${PD_PATH}/src/d_ctl.c
    ${PD_PATH}/src/d_dac.c
    ${PD_PATH}/src/d_delay.c
    ${PD_PATH}/src/d_fft.c
    ${PD_PATH}/src/d_fft_fftsg.c
    ${PD_PATH}/src/d_filter.c
    ${PD_PATH}/src/d_global.c
    ${PD_PATH}/src/d_math.c
    ${PD_PATH}/src/d_misc.c
    ${PD_PATH}/src/d_osc.c
    ${PD_PATH}/src/d_resample.c
    ${PD_PATH}/src/d_soundfile_aiff.c
    ${PD_PATH}/src/d_soundfile_caf.c
    ${PD_PATH}/src/d_soundfile_next.c
    ${PD_PATH}/src/d_soundfile_wave.c
    ${PD_PATH}/src/d_soundfile.c
    ${PD_PATH}/src/d_ugen.c
    ${PD_PATH}/src/g_all_guis.c
    ${PD_PATH}/src/g_all_guis.h
    ${PD_PATH}/src/g_array.c
    ${PD_PATH}/src/g_bang.c
    ${PD_PATH}/src/g_canvas.c
    ${PD_PATH}/src/g_canvas.h
    ${PD_PATH}/src/g_clone.c
		${PD_PATH}/src/g_editor_extras.c
    ${PD_PATH}/src/g_editor.c
    ${PD_PATH}/src/g_graph.c
    ${PD_PATH}/src/g_guiconnect.c
    ${PD_PATH}/src/g_hdial.c
    ${PD_PATH}/src/g_hslider.c
    ${PD_PATH}/src/g_io.c
    ${PD_PATH}/src/g_mycanvas.c
    ${PD_PATH}/src/g_numbox.c
    ${PD_PATH}/src/g_readwrite.c
    ${PD_PATH}/src/g_rtext.c
    ${PD_PATH}/src/g_scalar.c
    ${PD_PATH}/src/g_template.c
    ${PD_PATH}/src/g_text.c
    ${PD_PATH}/src/g_toggle.c
    ${PD_PATH}/src/g_traversal.c
		${PD_PATH}/src/g_undo.c
		${PD_PATH}/src/g_undo.h
    ${PD_PATH}/src/g_vdial.c
    ${PD_PATH}/src/g_vslider.c
    ${PD_PATH}/src/g_vumeter.c
    ${PD_PATH}/src/m_atom.c
    ${PD_PATH}/src/m_binbuf.c
    ${PD_PATH}/src/m_class.c
    ${PD_PATH}/src/m_conf.c
    ${PD_PATH}/src/m_glob.c
    ${PD_PATH}/src/m_imp.h
    ${PD_PATH}/src/m_memory.c
    ${PD_PATH}/src/m_obj.c
    ${PD_PATH}/src/m_pd.c
    ${PD_PATH}/src/m_pd.h
    ${PD_PATH}/src/m_sched.c
    ${PD_PATH}/src/s_audio.c
    ${PD_PATH}/src/s_audio_dummy.c
    ${PD_PATH}/src/s_loader.c
    ${PD_PATH}/src/s_main.c
		${PD_PATH}/src/s_net.c
		${PD_PATH}/src/s_net.h
    ${PD_PATH}/src/s_path.c
    ${PD_PATH}/src/s_print.c
    ${PD_PATH}/src/s_stuff.h
    ${PD_PATH}/src/s_utf8.c
    ${PD_PATH}/src/s_utf8.h
    ${PD_PATH}/src/x_acoustics.c
    ${PD_PATH}/src/x_arithmetic.c
    ${PD_PATH}/src/x_array.c
    ${PD_PATH}/src/x_connective.c
    ${PD_PATH}/src/x_file.c
    ${PD_PATH}/src/x_gui.c
    ${PD_PATH}/src/x_interface.c
    ${PD_PATH}/src/x_list.c
    ${PD_PATH}/src/x_midi.c
    ${PD_PATH}/src/x_misc.c
    ${PD_PATH}/src/x_net.c
    ${PD_PATH}/src/x_scalar.c
    ${PD_PATH}/src/x_text.c
    ${PD_PATH}/src/x_time.c
    ${PD_PATH}/src/x_vexp.c
    ${PD_PATH}/src/x_vexp_fun.c
    ${PD_PATH}/src/x_vexp_if.c
)
include_directories(${PD_PATH}/src)
source_group(pd FILES ${PD_SOURCES})
list(APPEND SOURCE_FILES ${PD_SOURCES})

# PURE DATA EXTRA SOURCES
#------------------------------------------------------------------------------#
if(PD_EXTRA)
    file(GLOB PD_EXTRA_SOURCES
    ${PD_PATH}/extra/bob~/bob~.c
    ${PD_PATH}/extra/bonk~/bonk~.c
    ${PD_PATH}/extra/choice/choice.c
    ${PD_PATH}/extra/fiddle~/fiddle~.c
    ${PD_PATH}/extra/loop~/loop~.c
    ${PD_PATH}/extra/lrshift~/lrshift~.c
    ${PD_PATH}/extra/pd~/pd~.c
    ${PD_PATH}/extra/pd~/pdsched.c
    ${PD_PATH}/extra/pique/pique.c
    ${PD_PATH}/extra/sigmund~/sigmund~.c
    ${PD_PATH}/extra/stdout/stdout.c
    )
    include_directories(${PD_PATH}/pure-data/extra)
    source_group(pd\\extra FILES ${PD_EXTRA_SOURCES})
    list(APPEND SOURCE_FILES ${PD_EXTRA_SOURCES})
endif()

# LIBPD SOURCES
#------------------------------------------------------------------------------#
set(LIBPD_SOURCES
    ${LIBPD_PATH}/s_libpdmidi.c
    ${LIBPD_PATH}/x_libpdreceive.c
    ${LIBPD_PATH}/x_libpdreceive.h
    ${LIBPD_PATH}/z_hooks.c
    ${LIBPD_PATH}/z_hooks.h
    ${LIBPD_PATH}/z_libpd.c
    ${LIBPD_PATH}/x_libpd_extra_utils.c
    ${LIBPD_PATH}/x_libpd_extra_utils.h
    ${LIBPD_PATH}/x_libpd_mod_utils.c
    ${LIBPD_PATH}/x_libpd_mod_utils.h
    ${LIBPD_PATH}/x_libpd_multi.c
    ${LIBPD_PATH}/x_libpd_multi.h
    ${LIBPD_PATH}/s_libpd_inter.c
    ${LIBPD_PATH}/s_libpd_inter.h

)

include_directories(${LIBPD_PATH})
source_group(libpd FILES ${LIBPD_SOURCES})
list(APPEND SOURCE_FILES ${LIBPD_SOURCES})

# ELSE SOURCES
#------------------------------------------------------------------------------#
file(GLOB_RECURSE ELSE_SOURCES
    ./ELSE/Source/*.c
    ./ELSE/Aliases/*.c
    ./ELSE/shared/*.c
    )
    
if(LIBSNDFILE_FOUND)
 #   list(APPEND ELSE_SOURCES ./ELSE/sfont~/sfont~.c)
endif()

include_directories(ELSE/Source/)
include_directories(ELSE/Aliases/)
include_directories(ELSE/shared)
source_group(ELSE FILES ${ELSE_SOURCES})
list(APPEND SOURCE_FILES ${ELSE_SOURCES})

file(GLOB_RECURSE CYCLONE_SOURCES
    ./cyclone/cyclone_lib.c
    ./cyclone/audio/*.c
    ./cyclone/control/*.c
    ./cyclone/shared/*.c
)

include_directories(cyclone/)
include_directories(cyclone/shared/)
source_group(cyclone FILES ${CYCLONE_SOURCES})
list(APPEND SOURCE_FILES ${CYCLONE_SOURCES})

#------------------------------------------------------------------------------#
# COMPILE DEFINITIONS
#------------------------------------------------------------------------------#
set(LIBPD_COMPILE_DEFINITIONS PD=1 USEAPI_DUMMY=1 PD_INTERNAL=1)

# COMPILE DEFINITIONS OPTIONS
#------------------------------------------------------------------------------#
if(PD_EXTRA)
    list(APPEND LIBPD_COMPILE_DEFINITIONS LIBPD_EXTRA=1)
endif()
if(NOT PD_LOCALE)
    list(APPEND LIBPD_COMPILE_DEFINITIONS LIBPD_NO_NUMERIC=1)
endif()

# COMPILE DEFINITIONS OS
#------------------------------------------------------------------------------#
if(WIN32)
    list(APPEND LIBPD_COMPILE_DEFINITIONS WINVER=0x502 WIN32=1 _WIN32=1)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        list(APPEND LIBPD_COMPILE_DEFINITIONS "PD_LONGINTTYPE=long long")
	endif()
    if(MSVC)
        list(APPEND LIBPD_COMPILE_DEFINITIONS HAVE_STRUCT_TIMESPEC=1 _CRT_SECURE_NO_WARNINGS=1 HAVE_ALLOCA=0)
    endif()
elseif(UNIX)
    list(APPEND LIBPD_COMPILE_DEFINITIONS HAVE_LIBDL=1 HAVE_UNISTD_H=1)
endif()


#------------------------------------------------------------------------------#
# C FLAGS
#------------------------------------------------------------------------------#

# C FLAGS COMPILER OR GENERATOR
#------------------------------------------------------------------------------#


if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
    if(MSVC_STATIC_RUNTIME)
        foreach(RFLAG CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${RFLAG} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${RFLAG} "${${RFLAG}}")
            endif()
        endforeach()
    endif()
else(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-compare-distinct-pointer-types -Wno-pointer-sign -Wno-int-conversion -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types -Wno-parentheses -Wno-unused-value")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -ffast-math -funroll-loops -fomit-frame-pointer -O3")
endif()

#------------------------------------------------------------------------------#
# TARGETS
#------------------------------------------------------------------------------#

add_library(pd STATIC ${SOURCE_FILES})
target_compile_definitions(pd PRIVATE ${LIBPD_COMPILE_DEFINITIONS})
if(MSVC)
  target_compile_definitions(pd PRIVATE PTW32_STATIC_LIB=1)
endif()
set_target_properties(pd PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(pd-multi STATIC ${SOURCE_FILES})
target_compile_definitions(pd-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1)
if(MSVC)
  target_compile_definitions(pd-multi PRIVATE PTW32_STATIC_LIB=1 "EXTERN= ")
endif()
set_target_properties(pd-multi PROPERTIES POSITION_INDEPENDENT_CODE ON)


#------------------------------------------------------------------------------#
# GENERATOR OPTIONS
#------------------------------------------------------------------------------#
if(CMAKE_GENERATOR STREQUAL Xcode)
      set_target_properties(pd PROPERTIES GCC_WARN_UNUSED_VARIABLE False)
      set_target_properties(pd PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO[variant=Release] True)
      set_target_properties(pd PROPERTIES GCC_OPTIMIZATION_LEVEL[variant=Release] 3)
      set_target_properties(pd PROPERTIES GCC_UNROLL_LOOPS[variant=Release] True)
      set_target_properties(pd PROPERTIES GCC_FAST_MATH[variant=Release] True)
      set_target_properties(pd-multi PROPERTIES GCC_WARN_UNUSED_VARIABLE False)
      set_target_properties(pd-multi PROPERTIES XCODE_ATTRIBUTE_LLVM_LTO[variant=Release] True)
      set_target_properties(pd-multi PROPERTIES GCC_OPTIMIZATION_LEVEL[variant=Release] 3)
      set_target_properties(pd-multi PROPERTIES GCC_UNROLL_LOOPS[variant=Release] True)
      set_target_properties(pd-multi PROPERTIES GCC_FAST_MATH[variant=Release] True)
endif()

#------------------------------------------------------------------------------#
# LINK
#------------------------------------------------------------------------------#

# LINK OS
#------------------------------------------------------------------------------#
if("${CMAKE_SYSTEM}" MATCHES "Linux")

  find_library(MATH_LIB m)

  target_link_libraries(pd ${MATH_LIB})
  target_link_libraries(pd ${CMAKE_DL_LIBS})
  target_link_libraries(pd-multi ${MATH_LIB})
  target_link_libraries(pd-multi ${CMAKE_DL_LIBS})

elseif(MSVC)
  target_link_libraries(pd PUBLIC "-Wl,--export-all-symbols,--whole-archive ws2_32 kernel32 -static-libgcc")
endif()

# LINK PTHREAD
#------------------------------------------------------------------------------#
set(THREADS_PREFER_PTHREAD_FLAG On)
set(CMAKE_THREAD_PREFER_PTHREAD True)

#if(LIBSNDFILE_FOUND)
#set_target_properties(fluidsynth PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
#endif()