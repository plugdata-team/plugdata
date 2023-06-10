cmake_minimum_required(VERSION 3.15)

# ------------------------------------------------------------------------------#
# LIBPD PROJECT IN C
# ------------------------------------------------------------------------------#
project(libpd VERSION 1.0.0 LANGUAGES C CXX)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS FALSE)
    add_subdirectory(pthread-win32)
    include_directories(pthread-win32)
endif()


cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(JUCE)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
  set(MESSAGE_QUIET ON)
  add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)
  unset(MESSAGE_QUIET)
endif()

# ------------------------------------------------------------------------------#
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

# LIBPD SOURCES
# ------------------------------------------------------------------------------#
set(LIBPD_SOURCES
    ${LIBPD_PATH}/s_libpd_midi.c
    ${LIBPD_PATH}/x_libpd_receive.c
    ${LIBPD_PATH}/x_libpd_receive.h
    ${LIBPD_PATH}/x_libpd_extra_utils.c
    ${LIBPD_PATH}/x_libpd_extra_utils.h
    ${LIBPD_PATH}/x_libpd_mod_utils.c
    ${LIBPD_PATH}/x_libpd_mod_utils.h
    ${LIBPD_PATH}/x_libpd_multi.c
    ${LIBPD_PATH}/x_libpd_multi.h
    ${LIBPD_PATH}/x_libpd_ofelia.cpp
)

include_directories(${LIBPD_PATH})
source_group(libpd FILES ${LIBPD_SOURCES})
list(APPEND SOURCE_FILES ${LIBPD_SOURCES})

# ELSE SOURCES
# ------------------------------------------------------------------------------#
file(GLOB ELSE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/Aliases/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/shared/*.c
)

file(GLOB_RECURSE PLAITS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/plaits~/plaits~.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/plaits~/*.cc
)

list(APPEND ELSE_SOURCES ${PLAITS_SOURCES})

file(GLOB_RECURSE AUBIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/shared/aubio/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/shared/aubio/src/*/*.c
)

include_directories(ELSE/Source/)
include_directories(ELSE/Aliases/)
include_directories(ELSE/shared)
include_directories(ELSE/shared/aubio/src)
include_directories(ELSE/plaits~)

source_group(ELSE FILES ${ELSE_SOURCES})

file(GLOB_RECURSE CYCLONE_SOURCES
    ./cyclone/cyclone_objects/binaries/cyclone_lib.c
    ./cyclone/cyclone_objects/binaries/audio/*.c
    ./cyclone/cyclone_objects/binaries/control/*.c
    ./cyclone/shared/common/*.c
    ./cyclone/shared/control/*.c
    ./cyclone/shared/signal/*.c
)

include_directories(cyclone/)
include_directories(cyclone/shared/)
source_group(cyclone FILES ${CYCLONE_SOURCES})

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
target_compile_definitions(lua PRIVATE MAKE_LIB=1 LUA_USE_MACOSX=1)
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

# COMPILE DEFINITIONS OPTIONS
# ------------------------------------------------------------------------------#
if(PD_EXTRA)
    list(APPEND LIBPD_COMPILE_DEFINITIONS LIBPD_EXTRA=1)
endif()

if(NOT PD_LOCALE)
    list(APPEND LIBPD_COMPILE_DEFINITIONS LIBPD_NO_NUMERIC=1)
endif()

# COMPILE DEFINITIONS OS
# ------------------------------------------------------------------------------#
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

# ------------------------------------------------------------------------------#
# SFONT~
# ------------------------------------------------------------------------------#

if(ENABLE_SFONT)

    list(APPEND LIBPD_COMPILE_DEFINITIONS ENABLE_SFONT=1)

    set(SFONT_DIR ELSE/sfont~)
    set(FLUIDLIGHT_DIR ${SFONT_DIR}/FluidLite)

    list(APPEND SFONT_SOURCES
        ${FLUIDLIGHT_DIR}/src/fluid_init.c
        ${FLUIDLIGHT_DIR}/src/fluid_chan.c
        ${FLUIDLIGHT_DIR}/src/fluid_chorus.c
        ${FLUIDLIGHT_DIR}/src/fluid_conv.c
        ${FLUIDLIGHT_DIR}/src/fluid_defsfont.c
        ${FLUIDLIGHT_DIR}/src/fluid_dsp_float.c
        ${FLUIDLIGHT_DIR}/src/fluid_gen.c
        ${FLUIDLIGHT_DIR}/src/fluid_hash.c
        ${FLUIDLIGHT_DIR}/src/fluid_list.c
        ${FLUIDLIGHT_DIR}/src/fluid_mod.c
        ${FLUIDLIGHT_DIR}/src/fluid_ramsfont.c
        ${FLUIDLIGHT_DIR}/src/fluid_rev.c
        ${FLUIDLIGHT_DIR}/src/fluid_settings.c
        ${FLUIDLIGHT_DIR}/src/fluid_synth.c
        ${FLUIDLIGHT_DIR}/src/fluid_sys.c
        ${FLUIDLIGHT_DIR}/src/fluid_tuning.c
        ${FLUIDLIGHT_DIR}/src/fluid_voice.c

        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/vorbisenc.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/info.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/analysis.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/bitrate.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/block.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/codebook.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/envelope.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/floor0.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/floor1.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/lookup.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/lpc.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/lsp.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/mapping0.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/mdct.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/psy.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/registry.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/res0.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/sharedbook.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/smallft.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/vorbisfile.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/window.c
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib/synthesis.c

        ${FLUIDLIGHT_DIR}/libogg-1.3.2/src/bitwise.c
        ${FLUIDLIGHT_DIR}/libogg-1.3.2/src/framing.c
    )

    list(APPEND SFONT_INCLUDES
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/include
        ${FLUIDLIGHT_DIR}/libvorbis-1.3.5/lib
        ${FLUIDLIGHT_DIR}/libogg-1.3.2/include
        ${FLUIDLIGHT_DIR}/include
    )

    add_library(fluidlite STATIC ${SFONT_SOURCES})
    target_include_directories(fluidlite PRIVATE ${SFONT_INCLUDES})

    list(APPEND ELSE_SOURCES
        ${SFONT_DIR}/sfont~.c
    )

endif()

# ------------------------------------------------------------------------------#
# C FLAGS
# ------------------------------------------------------------------------------#

# C FLAGS COMPILER OR GENERATOR
# ------------------------------------------------------------------------------#
if(MSVC)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
else(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-compare-distinct-pointer-types -Wno-pointer-sign -Wno-int-conversion -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types -Wno-parentheses -Wno-unused-value")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -ffast-math -funroll-loops -fomit-frame-pointer -O3")
endif()

# ------------------------------------------------------------------------------#
# TARGETS
# ------------------------------------------------------------------------------#
add_library(externals STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES} ${PDLUA_SOURCES} ${AUBIO_SOURCES})
add_library(externals-multi STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES} ${PDLUA_SOURCES} ${AUBIO_SOURCES})

# ag: pdlua needs PLUGDATA symbol at compile time for PlugData integration.
target_compile_definitions(externals PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PLUGDATA=1)
target_compile_definitions(externals-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1 PLUGDATA=1)

if(MSVC)
    add_library(pd SHARED ${SOURCE_FILES})
    target_compile_definitions(pd PRIVATE ${LIBPD_COMPILE_DEFINITIONS})
else()
    add_library(pd STATIC ${SOURCE_FILES})
    target_compile_definitions(pd PRIVATE ${LIBPD_COMPILE_DEFINITIONS})
endif()

add_library(pd-multi STATIC ${SOURCE_FILES})
target_compile_definitions(pd-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1)

if(MSVC)
    target_compile_definitions(pd-multi PRIVATE PTW32_STATIC_LIB=1 "EXTERN= ")
endif()

set_target_properties(pd PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(pd-multi PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(externals PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(externals-multi PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(BUILD_OFELIA)
message(STATUS "Cloning plugdata-ofelia (this can take a while)")
include(FetchContent)

FetchContent_Declare(
    plugdata-ofelia
    GIT_REPOSITORY https://github.com/plugdata-team/plugdata-ofelia.git
    GIT_TAG main
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/plugdata-ofelia"
)

FetchContent_GetProperties(plugdata-ofelia)
if(NOT plugdata-ofelia_POPULATED)
    FetchContent_Populate(plugdata-ofelia)
endif()

message(STATUS "Building Ofelia (this can take a while)")

add_subdirectory(plugdata-ofelia)

if(MSVC)
execute_process(COMMAND ./Resources/build.bat
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugdata-ofelia OUTPUT_QUIET)
else()
execute_process(COMMAND ./Resources/build.sh
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugdata-ofelia OUTPUT_QUIET)
endif()

target_link_libraries(externals ofelia)
target_link_libraries(externals-multi ofelia)
endif()


target_link_libraries(externals fluidlite lua)
target_link_libraries(externals-multi fluidlite lua)

target_include_directories(externals PRIVATE ${LUA_INCLUDE_DIR})
target_include_directories(externals-multi PRIVATE ${LUA_INCLUDE_DIR})
# ------------------------------------------------------------------------------#
# GENERATOR OPTIONS
# ------------------------------------------------------------------------------#
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

# ------------------------------------------------------------------------------#
# LINK
# ------------------------------------------------------------------------------#

# LINK OS
# ------------------------------------------------------------------------------#
if("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_library(MATH_LIB m)

    target_link_libraries(pd ${MATH_LIB} ${CMAKE_DL_LIBS} externals)
    target_link_libraries(pd-multi ${MATH_LIB} ${CMAKE_DL_LIBS} externals-multi)

elseif(MSVC)
    target_link_libraries(pd PUBLIC pthreadVC3 ws2_32 externals)
    target_link_libraries(pd-multi PUBLIC pthreadVC3 ws2_32 externals-multi)

    add_custom_command(TARGET pd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/pd.dll"
        ${CMAKE_SOURCE_DIR}/Plugins/Standalone/pd.dll)

    add_custom_command(TARGET pd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/pd.lib"
        ${CMAKE_SOURCE_DIR}/Plugins/Standalone/pd.lib)
elseif(APPLE)
    target_link_libraries(pd PUBLIC externals)
    target_link_libraries(pd-multi PUBLIC externals-multi)
endif()

# LINK PTHREAD
# ------------------------------------------------------------------------------#
set(THREADS_PREFER_PTHREAD_FLAG On)
set(CMAKE_THREAD_PREFER_PTHREAD True)
