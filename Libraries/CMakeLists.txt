cmake_minimum_required(VERSION 3.1)

# ------------------------------------------------------------------------------#
# LIBPD PROJECT IN C
# ------------------------------------------------------------------------------#
project(libpd VERSION 1.0.0 LANGUAGES C)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
    add_subdirectory(pthread-win32)
    include_directories(pthread-win32)
endif()

add_subdirectory(JUCE)

set(SFONT_LIBS "")

# ------------------------------------------------------------------------------#
set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Switch to enable/disable sfont~, in case the build fails or you want to speed up the build
if(ENABLE_SFONT)
    message("-- Building fluidsynth")

    set(SFONT_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/fluidsynth")

    if(MSVC)
        list(APPEND SFONT_LIBS
            ${SFONT_LIB_DIR}/lib64/FLAC.lib
            ${SFONT_LIB_DIR}/lib64/libfluidsynth.lib
            ${SFONT_LIB_DIR}/lib64/libmp3lame-static.lib
            ${SFONT_LIB_DIR}/lib64/libmpghip-static.lib
            ${SFONT_LIB_DIR}/lib64/mpg123.lib
            ${SFONT_LIB_DIR}/lib64/ogg.lib
            ${SFONT_LIB_DIR}/lib64/opus.lib
            ${SFONT_LIB_DIR}/lib64/sndfile.lib
            ${SFONT_LIB_DIR}/lib64/vorbis.lib
            ${SFONT_LIB_DIR}/lib64/vorbisenc.lib

            ${SFONT_LIB_DIR}/lib/FLAC.lib
            ${SFONT_LIB_DIR}/lib/libfluidsynth.lib
            ${SFONT_LIB_DIR}/lib/libmp3lame-static.lib
            ${SFONT_LIB_DIR}/lib/libmpghip-static.lib
            ${SFONT_LIB_DIR}/lib/mpg123.lib
            ${SFONT_LIB_DIR}/lib/ogg.lib
            ${SFONT_LIB_DIR}/lib/opus.lib
            ${SFONT_LIB_DIR}/lib/sndfile.lib
            ${SFONT_LIB_DIR}/lib/vorbis.lib
            ${SFONT_LIB_DIR}/lib/vorbisenc.lib
        )

        # Building fluidsynth on Windows is a hassle, so I've prebuilt all the static deps once, shipped in a zipfile
        add_custom_target(extract_fluidsynth             
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/sfont~/win_deps.zip
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            )

         add_library(
                fluidsynth
                STATIC
                IMPORTED
            )

        add_dependencies(fluidsynth extract_fluidsynth)
            
        set_target_properties(
            fluidsynth
             PROPERTIES 
             IMPORTED_LOCATION ${SFONT_LIBS})

    else()

        list(APPEND SFONT_LIBS
            ${SFONT_LIB_DIR}/lib/libfluidsynth.a
            ${SFONT_LIB_DIR}/lib/libsndfile.a
            ${SFONT_LIB_DIR}/lib/libogg.a
            ${SFONT_LIB_DIR}/lib/libopus.a
            ${SFONT_LIB_DIR}/lib/libFLAC.a
            ${SFONT_LIB_DIR}/lib/libvorbis.a
            ${SFONT_LIB_DIR}/lib/libvorbisenc.a
        )

        # Unpack sources of dependencies
        add_custom_target(extract_sfont_sources       
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/sfont~/unix_deps.zip
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )


        # Build fluidsynth and dependencies from source, silence the makefile
        add_custom_target(build_fluidsynth 
            COMMAND make -f ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/sfont~/make-static-libs.mk config USE_CCACHE=${USE_CCACHE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )

        add_library(
            fluidsynth
            STATIC
            IMPORTED
        )

        set_target_properties(
            fluidsynth
             PROPERTIES 
             IMPORTED_LOCATION ${SFONT_LIBS})
    
    add_dependencies(build_fluidsynth extract_sfont_sources)    
    add_dependencies(fluidsynth extract_sfont_sources)
        

    endif()
endif()


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
    ${LIBPD_PATH}/s_libpd_inter.c
    ${LIBPD_PATH}/s_libpd_inter.h
    ${LIBPD_PATH}/m_libpd_class.c
)

include_directories(${LIBPD_PATH})
source_group(libpd FILES ${LIBPD_SOURCES})
list(APPEND SOURCE_FILES ${LIBPD_SOURCES})

# ELSE SOURCES
# ------------------------------------------------------------------------------#
file(GLOB_RECURSE ELSE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/Aliases/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/shared/*.c
)

if(ENABLE_SFONT)
    list(APPEND ELSE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ELSE/sfont~/sfont~.c)
    include_directories(${PROJECT_BINARY_DIR}/fluidsynth/include)
endif()

include_directories(ELSE/Source/)
include_directories(ELSE/Aliases/)
include_directories(ELSE/shared)

source_group(ELSE FILES ${ELSE_SOURCES})

file(GLOB_RECURSE CYCLONE_SOURCES
    ./cyclone/cyclone_lib.c
    ./cyclone/audio/*.c
    ./cyclone/control/*.c
    ./cyclone/shared/*.c
)

include_directories(cyclone/)
include_directories(cyclone/shared/)
source_group(cyclone FILES ${CYCLONE_SOURCES})

# ------------------------------------------------------------------------------#
# COMPILE DEFINITIONS
# ------------------------------------------------------------------------------#
set(LIBPD_COMPILE_DEFINITIONS PD=1 USEAPI_DUMMY=1 PD_INTERNAL=1)

if(ENABLE_SFONT)
    list(APPEND LIBPD_COMPILE_DEFINITIONS ENABLE_SFONT=1)
endif()

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
add_library(externals STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES})
add_library(externals-multi STATIC ${ELSE_SOURCES} ${CYCLONE_SOURCES})

target_compile_definitions(externals PRIVATE ${LIBPD_COMPILE_DEFINITIONS})
target_compile_definitions(externals-multi PRIVATE ${LIBPD_COMPILE_DEFINITIONS} PDINSTANCE=1 PDTHREADS=1)

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

    target_link_libraries(pd ${MATH_LIB} ${CMAKE_DL_LIBS} externals fluidsynth)
    target_link_libraries(pd-multi ${MATH_LIB} ${CMAKE_DL_LIBS} externals-multi fluidsynth)

elseif(MSVC)
    target_link_libraries(pd PUBLIC libpthreadVC3 ws2_32 Shlwapi.lib externals fluidsynth)
    target_link_libraries(pd-multi PUBLIC libpthreadVC3 ws2_32 Shlwapi.lib externals-multi fluidsynth)

    add_custom_command(TARGET pd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/pd.dll"
        ${CMAKE_SOURCE_DIR}/Plugins/Standalone/pd.dll)

    add_custom_command(TARGET pd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/pd.lib"
        ${CMAKE_SOURCE_DIR}/Plugins/Standalone/pd.lib)
elseif(APPLE)
    target_link_libraries(pd PUBLIC externals fluidsynth)
    target_link_libraries(pd-multi PUBLIC externals-multi fluidsynth)
endif()

# LINK PTHREAD
# ------------------------------------------------------------------------------#
set(THREADS_PREFER_PTHREAD_FLAG On)
set(CMAKE_THREAD_PREFER_PTHREAD True)
