cmake_minimum_required(VERSION 3.15)

# ------------------------------------------------------------------------------#
# LIBPD PROJECT IN C
# ------------------------------------------------------------------------------#
project(libpd VERSION 1.0.0 LANGUAGES C CXX)

set(MESSAGE_QUIET ON)
message(STATUS "Configuring fftw3")
add_subdirectory(fftw3)
set(MESSAGE_QUIET OFF)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    option(BUILD_SHARED_LIBS "" FALSE)
    add_subdirectory(pthread-win32)
    include_directories(pthread-win32)
    add_compile_options(/experimental:c11atomics)
endif()

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(JUCE)
add_subdirectory(nanovg)

juce_add_module(melatonin_blur)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
  set(MESSAGE_QUIET ON)
  add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)
  unset(MESSAGE_QUIET)
endif()

set(PD_DIR "pure-data/")

# ------------------------------------------------------------------------------#
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(PD_PATH "pure-data/")
set(LIBPD_PATH "libpd/")
set(LIBPD_OUTPUT_DIRECTORY "./..")

# ------------------------------------------------------------------------------#
# OPTIONS
# ------------------------------------------------------------------------------#
option(PD_UTILS "Compile libpd utilities" OFF)
option(PD_EXTRA "Compile extras" ON)
option(PD_LOCALE "Set the LC_NUMERIC number format to the default C locale" ON)


# ------------------------------------------------------------------------------#
# SOURCES
# ------------------------------------------------------------------------------#
set(SOURCE_FILES)

# PURE DATA SOURCES
# ------------------------------------------------------------------------------#
file(GLOB PD_SOURCES
    ${PD_PATH}/src/d_arithmetic.c
    ${PD_PATH}/src/d_array.c
    ${PD_PATH}/src/d_ctl.c
    ${PD_PATH}/src/d_dac.c
    ${PD_PATH}/src/d_delay.c
    ${PD_PATH}/src/d_fft.c
    ${PD_PATH}/src/d_fft_fftw.c
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
    ${PD_PATH}/src/g_io.c
    ${PD_PATH}/src/g_mycanvas.c
    ${PD_PATH}/src/g_numbox.c
    ${PD_PATH}/src/g_readwrite.c
    ${PD_PATH}/src/g_rtext.c
    ${PD_PATH}/src/g_radio.c
    ${PD_PATH}/src/g_scalar.c
    ${PD_PATH}/src/g_slider.c
    ${PD_PATH}/src/g_template.c
    ${PD_PATH}/src/g_text.c
    ${PD_PATH}/src/g_toggle.c
    ${PD_PATH}/src/g_traversal.c
    ${PD_PATH}/src/g_undo.c
    ${PD_PATH}/src/g_undo.h
    ${PD_PATH}/src/g_vdial.c
    ${PD_PATH}/src/g_vumeter.c
    ${PD_PATH}/src/m_atom.c
    ${PD_PATH}/src/m_binbuf.c
    ${PD_PATH}/src/m_conf.c
    ${PD_PATH}/src/m_class.c
    ${PD_PATH}/src/m_vst_cleanser.c
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
    ${PD_PATH}/src/s_inter_gui.c
    ${PD_PATH}/src/s_inter.c
    ${PD_PATH}/src/s_inter.h
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
    ${PD_PATH}/src/z_hooks.c
    ${PD_PATH}/src/z_libpd.c
    ${PD_PATH}/src/x_libpdreceive.c
    ${PD_PATH}/src/z_print_util.c
)
include_directories(${PD_PATH}/src)
source_group(pd FILES ${PD_SOURCES})
list(APPEND SOURCE_FILES ${PD_SOURCES})

# PURE DATA EXTRA SOURCES
# ------------------------------------------------------------------------------#
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

# ELSE SOURCES
# ------------------------------------------------------------------------------#
file(GLOB_RECURSE PLAITS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/plaits~/plaits~.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/plaits~/*.cc
)

file(GLOB ELSE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Control/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/sfont~/sfont~.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/circuit~/Source/circuit~.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/circuit~/Source/Simulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/circuit~/Libraries/**/*.c
    ${PLAITS_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/libsamplerate/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/link/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/link/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/link/udp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/link/udp/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/aubio/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/aubio/src/*/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Extra/Aliases/*.c
)

if(NOT ENABLE_FFMPEG)
list(REMOVE_ITEM ELSE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/play.file~.c)
list(REMOVE_ITEM ELSE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Control/sfload.c)
list(REMOVE_ITEM ELSE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Control/sfinfo.c)
endif()


if(ENABLE_SFIZZ)
list(APPEND ELSE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/sfz~/sfz~.c)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

source_group(ELSE FILES ${ELSE_SOURCES})

file(GLOB_RECURSE CYCLONE_SOURCES
    ./pd-cyclone/cyclone_objects/binaries/cyclone_lib.c
    ./pd-cyclone/cyclone_objects/binaries/audio/*.c
    ./pd-cyclone/cyclone_objects/binaries/control/*.c
    ./pd-cyclone/shared/common/*.c
    ./pd-cyclone/shared/control/*.c
    ./pd-cyclone/shared/signal/*.c
)

include_directories(pd-cyclone/)
include_directories(pd-cyclone/shared/)
source_group(pd-cyclone FILES ${CYCLONE_SOURCES})

# pdlua sources
set(PDLUA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pd-lua")
set(PDLUA_SOURCES ${PDLUA_PATH}/pdlua.c)

set(LUA_PATH "${PDLUA_PATH}/lua")
set(LUA_INCLUDE_DIR ${LUA_PATH})

add_library(lua STATIC ${LUA_PATH}/onelua.c)
if("${CMAKE_SYSTEM}" MATCHES "Linux")
target_compile_definitions(lua PRIVATE MAKE_LIB=1 LUA_USE_LINUX=1)
elseif(MSVC)
target_compile_definitions(lua PRIVATE MAKE_LIB=1 LUA_USE_WINDOWS=1)
elseif(APPLE)
    if("${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
        target_compile_definitions(lua PRIVATE LUA_USE_IOS=1 PRIVATE MAKE_LIB=1)
    else()
        target_compile_definitions(lua PRIVATE MAKE_LIB=1 LUA_USE_MACOSX=1)
    endif()
else()
target_compile_definitions(lua PRIVATE MAKE_LIB=1)
endif()

source_group(pdlua FILES ${PDLUA_SOURCES})

# live version data for pdlua.c
# set(PDLUA_VERSION 0.11.0)
#execute_process(COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR}/pd-lua describe --tags OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PDLUA_VERSION)
#set_source_files_properties(${PDLUA_PATH}/pdlua.c PROPERTIES COMPILE_DEFINITIONS PDLUA_VERSION=${PDLUA_VERSION})

# ------------------------------------------------------------------------------#
# COMPILE DEFINITIONS
# ------------------------------------------------------------------------------#
set(LIBPD_COMPILE_DEFINITIONS PD=1 USEAPI_DUMMY=1 PD_INTERNAL=1)


if(ENABLE_FFMPEG)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/ffmpeg)
set(FFMPEG_PATH ${CMAKE_CURRENT_BINARY_DIR}/pd-else/Source/Shared/ffmpeg/ffmpeg-7.0.1)
endif()

if(ENABLE_GEM)
add_subdirectory(Gem)
set(GEM_LIBS Gem Gem-plugins ftgl glew_s freetype assimp)
set(GEM_LIBS_MULTI Gem-multi Gem-plugins ftgl glew_s freetype assimp)
target_include_directories(Gem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Gem/plugins/ASSIMP3/assimp/include)
target_include_directories(Gem-multi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Gem/plugins/ASSIMP3/assimp/include)
endif()

if(ENABLE_SFIZZ)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/sfz~)
endif()

# COMPILE DEFINITIONS OPTIONS
# ------------------------------------------------------------------------------#
if(PD_EXTRA)
    list(APPEND LIBPD_COMPILE_DEFINITIONS LIBPD_EXTRA=1)
endif()

# COMPILE DEFINITIONS OS
# ------------------------------------------------------------------------------#
if(WIN32)
    list(APPEND LIBPD_COMPILE_DEFINITIONS WINVER=0x502 WIN32=1 _WIN32=1)

    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        list(APPEND LIBPD_COMPILE_DEFINITIONS "PD_LONGINTTYPE=long long")
    endif()

    if(MSVC)
        list(APPEND LIBPD_COMPILE_DEFINITIONS HAVE_STRUCT_TIMESPEC=1 _CRT_SECURE_NO_WARNINGS=1 HAVE_ALLOCA=1 DONT_USE_ALLOCA=0)
    endif()
elseif(UNIX)
    if(LINUX OR APPLE)
        list(APPEND LIBPD_COMPILE_DEFINITIONS HAVE_LIBDL=1 HAVE_UNISTD_H=1 HAVE_ALLOCA_H=1 HAVE_ALLOCA=1 DONT_USE_ALLOCA=0)
    else() # BSD
        list(APPEND LIBPD_COMPILE_DEFINITIONS HAVE_LIBDL=1 HAVE_UNISTD_H=1 HAVE_ALLOCA=1 DONT_USE_ALLOCA=0 HAVE_ENDIAN_H=0)
    endif()
endif()

# ------------------------------------------------------------------------------#
# C FLAGS
# ------------------------------------------------------------------------------#

# C FLAGS COMPILER OR GENERATOR
# ------------------------------------------------------------------------------#
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    # These are the warning flags I need to get pd to compile withoout warnings
    # Would be nice if these warnings were solved instead
    add_compile_options(-Wno-format -Wno-format-security -Wno-conversion -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-parentheses -Wno-unused-value -Wno-unknown-pragmas)

    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-compare-distinct-pointer-types -Wno-implicit-int-conversion)
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion -Wno-pointer-sign -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -fno-finite-math-only -ffast-math -funroll-loops -fomit-frame-pointer -O3")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0")
 endif()

# ------------------------------------------------------------------------------#
# SFONT~
# ------------------------------------------------------------------------------#

set(FLUIDLITE_DIR pd-else/Source/Audio/sfont~/FluidLite)
list(APPEND SFONT_SOURCES
    ${FLUIDLITE_DIR}/src/fluid_init.c
    ${FLUIDLITE_DIR}/src/fluid_chan.c
    ${FLUIDLITE_DIR}/src/fluid_chorus.c
    ${FLUIDLITE_DIR}/src/fluid_conv.c
    ${FLUIDLITE_DIR}/src/fluid_defsfont.c
    ${FLUIDLITE_DIR}/src/fluid_dsp_float.c
    ${FLUIDLITE_DIR}/src/fluid_gen.c
    ${FLUIDLITE_DIR}/src/fluid_hash.c
    ${FLUIDLITE_DIR}/src/fluid_list.c
    ${FLUIDLITE_DIR}/src/fluid_mod.c
    ${FLUIDLITE_DIR}/src/fluid_ramsfont.c
    ${FLUIDLITE_DIR}/src/fluid_rev.c
    ${FLUIDLITE_DIR}/src/fluid_settings.c
    ${FLUIDLITE_DIR}/src/fluid_synth.c
    ${FLUIDLITE_DIR}/src/fluid_sys.c
    ${FLUIDLITE_DIR}/src/fluid_tuning.c
    ${FLUIDLITE_DIR}/src/fluid_voice.c
)

if(MSVC)

list(APPEND SFONT_SOURCES
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/vorbisenc.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/info.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/analysis.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/bitrate.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/block.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/codebook.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/envelope.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/floor0.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/floor1.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/lookup.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/lpc.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/lsp.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/mapping0.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/mdct.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/psy.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/registry.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/res0.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/sharedbook.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/smallft.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/vorbisfile.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/window.c
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib/synthesis.c

    ${FLUIDLITE_DIR}/libogg-1.3.2/src/bitwise.c
    ${FLUIDLITE_DIR}/libogg-1.3.2/src/framing.c
)
endif()

list(APPEND SFONT_INCLUDES
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/include
    ${FLUIDLITE_DIR}/libvorbis-1.3.5/lib
    ${FLUIDLITE_DIR}/libogg-1.3.2/include
    ${FLUIDLITE_DIR}/include
)

add_library(fluidlite STATIC ${SFONT_SOURCES})
target_include_directories(fluidlite PRIVATE ${SFONT_INCLUDES})
target_include_directories(fluidlite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Audio/sfont~)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
target_compile_options(fluidlite PRIVATE -Wno-compound-token-split-by-macro)
endif()

set(MESSAGE_QUIET ON)
set(OPUS_STATIC_RUNTIME ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pd-else/Source/Shared/opus)
target_compile_definitions(opus PRIVATE FLOAT_APPROX=1)
unset(MESSAGE_QUIET)

# ------------------------------------------------------------------------------#
# TARGETS
# ------------------------------------------------------------------------------#
add_library(externals STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES} ${PDLUA_SOURCES})
add_library(externals-multi STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES} ${PDLUA_SOURCES})

list(APPEND ELSE_INCLUDES
    pd-else/Source/Control/
    pd-else/Source/Audio/
    pd-else/Source/Shared/
    pd-else/Source/Shared/libsamplerate
    pd-else/Source/Shared/aubio/src
    pd-else/Source/Audio/plaits~
    pd-else/Source/Audio/circuit~/Libraries
    pd-else/Source/Audio/sfz~/sfizz/library/src
    pd-else/Source/Shared/opus/include
    pd-else/Source/Shared/link
    ${CMAKE_CURRENT_BINARY_DIR}/pd-else/Source/Shared/ffmpeg/ffmpeg-7.0.1
)

target_include_directories(externals PRIVATE ${ELSE_INCLUDES})
target_include_directories(externals-multi PRIVATE ${ELSE_INCLUDES})

# ag: pdlua needs PLUGDATA symbol at compile time for plugdata integration.
target_compile_definitions(externals PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PLUGDATA=1)
target_compile_definitions(externals-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1 PLUGDATA=1)

add_library(pd SHARED ${SOURCE_FILES})
target_compile_definitions(pd PRIVATE ${LIBPD_COMPILE_DEFINITIONS})

add_library(pd-multi SHARED ${SOURCE_FILES})
target_compile_definitions(pd-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1 VST_CLEANSER=1)

if(MSVC)
    target_compile_definitions(pd-multi PRIVATE PTW32_STATIC_LIB=1)
endif()

if(UNIX)
    target_compile_options(pd PRIVATE -fvisibility=default)
    target_compile_options(pd-multi PRIVATE -fvisibility=default)
    target_compile_options(externals PRIVATE -fvisibility=default) # why?
endif()

set_target_properties(externals-multi PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(pd-multi PROPERTIES CXX_VISIBILITY_PRESET hidden)

set_target_properties(pd PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(pd-multi PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(externals PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(externals-multi PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(externals PRIVATE ${LUA_INCLUDE_DIR} ${SFONT_INCLUDES})
target_include_directories(externals-multi PRIVATE ${LUA_INCLUDE_DIR} ${SFONT_INCLUDES})


# ------------------------------------------------------------------------------#
# LINK
# ------------------------------------------------------------------------------#

set(externals_libs fluidlite lua opus)
if(ENABLE_SFIZZ)
list(APPEND externals_libs sfizz)
endif()

if(ENABLE_FFMPEG)
list(APPEND externals_libs ffmpeg)
if(ENABLE_GEM)
    target_link_libraries(Gem-plugins PUBLIC ffmpeg)
endif()
endif()

if("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_library(MATH_LIB m)
    target_link_libraries(externals PUBLIC ${externals_libs} ${GEM_LIBS})
    target_link_libraries(externals-multi PUBLIC ${externals_libs} ${GEM_LIBS_MULTI})
elseif(MSVC)
    target_link_libraries(pd PUBLIC pthreadVC3 ws2_32 ${externals_libs} ${GEM_LIBS})
    target_link_libraries(pd-multi PUBLIC pthreadVC3 ws2_32 ${externals_libs} ${GEM_LIBS_MULTI})
    target_link_libraries(externals PRIVATE pd)
    target_link_libraries(externals-multi PRIVATE pd-multi)
elseif(APPLE)
    target_link_libraries(externals PUBLIC ${externals_libs} ${GEM_LIBS})
    target_link_libraries(externals-multi PUBLIC ${externals_libs} ${GEM_LIBS_MULTI})
endif()

target_link_libraries(pd PRIVATE fftw3f)
target_link_libraries(pd-multi PRIVATE fftw3f)
target_include_directories(pd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fftw3/api)
target_include_directories(pd-multi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fftw3/api)

# PTHREAD
# ------------------------------------------------------------------------------#
set(THREADS_PREFER_PTHREAD_FLAG On)
set(CMAKE_THREAD_PREFER_PTHREAD True)
