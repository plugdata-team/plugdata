cmake_minimum_required(VERSION 3.1)

project(fluidlite
    LANGUAGES C
    VERSION 1.2.2
)

include(GNUInstallDirs)

option(ENABLE_SF3 "Enable SF3 files (ogg/vorbis compressed samples)" FALSE)
option(STB_VORBIS "Use STB VORBIS library instead of ogg/vorbis" FALSE)
option(WITH_FLOAT "Use 32 bit float type samples (instead of 64 bit double type)" TRUE)
option(CMAKE_POSITION_INDEPENDENT_CODE "Use PIC for building all sources" TRUE)

string(TOLOWER "${CMAKE_BUILD_TYPE}" LOWERCASE_BUILD_TYPE)
if("${LOWERCASE_BUILD_TYPE}" STREQUAL "debug")
    set(DEBUG TRUE)
endif()
include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
include(CheckIncludeFiles)
check_include_files("stdlib.h;stdio.h;stdarg.h;string.h;float.h;limits.h;math.h" STDC_HEADERS)
set(HAVE_STDLIB_H ${STDC_HEADERS} CACHE INTERNAL "Have include stdlib.h")
set(HAVE_STDIO_H ${STDC_HEADERS} CACHE INTERNAL "Have include stdio.h")
set(HAVE_STDARG_H ${STDC_HEADERS} CACHE INTERNAL "Have include stdarg.h")
set(HAVE_STRING_H ${STDC_HEADERS} CACHE INTERNAL "Have include string.h")
set(HAVE_FLOAT_H ${STDC_HEADERS} CACHE INTERNAL "Have include float.h")
set(HAVE_LIMITS_H ${STDC_HEADERS} CACHE INTERNAL "Have include limits.h")
set(HAVE_MATH_H ${STDC_HEADERS} CACHE INTERNAL "Have include math.h")
include(CheckIncludeFile)
check_include_file(fcntl.h HAVE_FCNTL_H)

list(APPEND HEADERS
    include/fluidlite.h
)

configure_file(src/version.cmake ${PROJECT_BINARY_DIR}/fluidlite/version.h @ONLY)

list(APPEND SCOPED_HEADERS
    include/fluidlite/types.h
    include/fluidlite/settings.h
    include/fluidlite/synth.h
    include/fluidlite/sfont.h
    include/fluidlite/ramsfont.h
    include/fluidlite/log.h
    include/fluidlite/misc.h
    include/fluidlite/mod.h
    include/fluidlite/gen.h
    include/fluidlite/voice.h
    ${PROJECT_BINARY_DIR}/fluidlite/version.h
)

list(APPEND SOURCES
    src/fluid_init.c
    src/fluid_chan.c
    src/fluid_chorus.c
    src/fluid_conv.c
    src/fluid_defsfont.c
    src/fluid_dsp_float.c
    src/fluid_gen.c
    src/fluid_hash.c
    src/fluid_list.c
    src/fluid_mod.c
    src/fluid_ramsfont.c
    src/fluid_rev.c
    src/fluid_settings.c
    src/fluid_synth.c
    src/fluid_sys.c
    src/fluid_tuning.c
    src/fluid_voice.c
)

# Dependencies:

set(ADDITIONAL_LIBS "")
# find the math lib, except in macOS/Windows
set(M_LIBRARY "")
if (UNIX AND NOT APPLE)
    find_library(M_LIBRARY m)
    message(STATUS "Math library: ${M_LIBRARY}")
    if(M_LIBRARY)
        set(ADDITIONAL_LIBS "-lm")
    endif()
endif()

if (ENABLE_SF3 AND NOT STB_VORBIS)
    find_package(PkgConfig QUIET)
    pkg_check_modules(LIBVORBIS vorbis>=1.3.5)
    pkg_check_modules(LIBVORBISFILE vorbisfile>=1.3.5)
    if (NOT LIBVORBIS_FOUND OR NOT LIBVORBISFILE_FOUND)
        list(APPEND SOURCES
            libvorbis-1.3.5/lib/vorbisenc.c
            libvorbis-1.3.5/lib/info.c
            libvorbis-1.3.5/lib/analysis.c
            libvorbis-1.3.5/lib/bitrate.c
            libvorbis-1.3.5/lib/block.c
            libvorbis-1.3.5/lib/codebook.c
            libvorbis-1.3.5/lib/envelope.c
            libvorbis-1.3.5/lib/floor0.c
            libvorbis-1.3.5/lib/floor1.c
            libvorbis-1.3.5/lib/lookup.c
            libvorbis-1.3.5/lib/lpc.c
            libvorbis-1.3.5/lib/lsp.c
            libvorbis-1.3.5/lib/mapping0.c
            libvorbis-1.3.5/lib/mdct.c
            libvorbis-1.3.5/lib/psy.c
            libvorbis-1.3.5/lib/registry.c
            libvorbis-1.3.5/lib/res0.c
            libvorbis-1.3.5/lib/sharedbook.c
            libvorbis-1.3.5/lib/smallft.c
            libvorbis-1.3.5/lib/vorbisfile.c
            libvorbis-1.3.5/lib/window.c
            libvorbis-1.3.5/lib/synthesis.c
        )
        list(APPEND LIBVORBIS_INCLUDE_DIRS
            ${PROJECT_SOURCE_DIR}/libvorbis-1.3.5/include
            ${PROJECT_SOURCE_DIR}/libvorbis-1.3.5/lib
        )
        message(WARNING "Using libvorbis shipped sources.")
    else()
        message(STATUS "Using pkg-config provided libvorbis")
        set(ADDITIONAL_LIBS "${ADDITIONAL_LIBS} ${LIBVORBIS_LDFLAGS} ${LIBVORBISFILE_LDFLAGS}")
    endif()

    pkg_check_modules(LIBOGG ogg>=1.3.2)
    if (NOT LIBOGG_FOUND)
        list(APPEND SOURCES
            libogg-1.3.2/src/bitwise.c
            libogg-1.3.2/src/framing.c
        )
        set(LIBOGG_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libogg-1.3.2/include)

        message(WARNING "Using libogg shipped sources.")
    else()
        message(STATUS "Using pkg-config provided libogg")
        set(ADDITIONAL_LIBS "${ADDITIONAL_LIBS} ${LIBOGG_LDFLAGS}")
    endif()
endif()

if (ENABLE_SF3 AND STB_VORBIS)
    list(APPEND SOURCES stb/stb_vorbis.c)
endif()

if (ENABLE_SF3)
    if (STB_VORBIS)
        set(SF3_SUPPORT "SF3_STB_VORBIS")
    else()
        set(SF3_SUPPORT "SF3_XIPH_VORBIS")
    endif()
else()
    set(SF3_SUPPORT "SF3_DISABLED")
endif()

configure_file(src/fluid_config.cmake ${PROJECT_BINARY_DIR}/fluid_config.h @ONLY)

# Common object library for the static and dynamic targets

add_library(${PROJECT_NAME}-obj OBJECT ${SOURCES})
set_target_properties(${PROJECT_NAME}-obj PROPERTIES C_STANDARD 99)
if(WIN32)
    target_compile_definitions(${PROJECT_NAME}-obj PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(${PROJECT_NAME}-obj PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(${PROJECT_NAME}-obj PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME}-obj PRIVATE ${PROJECT_SOURCE_DIR}/include)

if (ENABLE_SF3 AND NOT STB_VORBIS)
    target_include_directories(${PROJECT_NAME}-obj PRIVATE ${LIBOGG_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME}-obj PRIVATE ${LIBVORBIS_INCLUDE_DIRS})
endif()
if (ENABLE_SF3 AND STB_VORBIS)
    target_include_directories(${PROJECT_NAME}-obj PRIVATE ${PROJECT_SOURCE_DIR}/stb)
endif()

# Static library target

option(FLUIDLITE_BUILD_STATIC "Build static library" TRUE)
if(FLUIDLITE_BUILD_STATIC)
    add_library(${PROJECT_NAME}-static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}-obj>)
    target_link_libraries(${PROJECT_NAME}-static PUBLIC
        ${LIBVORBIS_LIBRARIES}
        ${LIBVORBISFILE_LIBRARIES}
        ${LIBOGG_LIBRARIES}
        ${M_LIBRARY}
    )
    set_target_properties(${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
    list(APPEND FLUIDLITE_INSTALL_TARGETS ${PROJECT_NAME}-static)
    target_include_directories(${PROJECT_NAME}-static PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

# Shared dynamic library target

option(FLUIDLITE_BUILD_SHARED "Build shared library" TRUE)
if(FLUIDLITE_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:${PROJECT_NAME}-obj>)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LIBVORBIS_LIBRARIES}
        ${LIBVORBISFILE_LIBRARIES}
        ${LIBOGG_LIBRARIES}
        ${M_LIBRARY}
    )
    list(APPEND FLUIDLITE_INSTALL_TARGETS ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
    target_include_directories(${PROJECT_NAME} PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${PROJECT_BINARY_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if((NOT FLUIDLITE_BUILD_SHARED) AND (NOT FLUIDLITE_BUILD_STATIC))
    message(FATAL_ERROR "Neither dynamic nor static library build is selected.")
endif()

if(IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
    set(fluidlite_libdir "${CMAKE_INSTALL_LIBDIR}")
else()
    set(fluidlite_libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE ${CMAKE_INSTALL_INCLUDEDIR})
    set(fluidlite_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(fluidlite_includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
configure_file(fluidlite.pc.in ${PROJECT_BINARY_DIR}/fluidlite.pc @ONLY)

install(TARGETS ${FLUIDLITE_INSTALL_TARGETS}
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${SCOPED_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fluidlite)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fluidlite.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Exported targets

install(EXPORT ${PROJECT_NAME}-targets
	FILE ${PROJECT_NAME}-targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

export(EXPORT ${PROJECT_NAME}-targets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake"
	NAMESPACE ${PROJECT_NAME}::
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${PROJECT_NAME}-config-version.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
	${PROJECT_NAME}-config.cmake.in
	${PROJECT_NAME}-config.cmake
	INSTALL_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
