cmake_minimum_required(VERSION 3.15)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RUN_CLANG_TIDY "" OFF)
option(ENABLE_TESTING "" OFF)
option(ENABLE_SFIZZ "" ON)
option(ENABLE_ASAN "" OFF)
option(VERBOSE "" OFF)

set (CMAKE_CXX_STANDARD 20)

# Visiblity needs to be hidden for all plugin targets, otherwise loading both plugdata and plugdata-fx will cause problems. We later undo this for the standalone build, so that externals can load
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

function(message)
    if (NOT MESSAGE_QUIET)
        _message(${ARGN})
    endif()
endfunction()

if(APPLE)
  set(CMAKE_XCODE_BUILD_SYSTEM "12" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
  if(RUN_CLANG_TIDY)
  set(CMAKE_OSX_ARCHITECTURES "arm64;" CACHE STRING "" FORCE)
  else()
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  endif()
endif()

if(RUN_CLANG_TIDY)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
    "Default build type: Release" FORCE)
endif()

set(HARDENED_RUNTIME_ENABLED YES)
set(HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input")

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(plugdata VERSION 0.8.3 LANGUAGES C CXX)

if(("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  OR ("${CMAKE_SYSTEM_NAME}" MATCHES "iOS"))
  message(STATUS "Disabled sfizz")
  set(ENABLE_SFIZZ OFF)
endif()

add_subdirectory(Libraries/ EXCLUDE_FROM_ALL)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Make sure missing return types will fail compilation with Clang
if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=return-type")
endif()

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(MSVC)
  add_compile_options(/MP /wd4244 /wd4311 /wd4003 /wd4047 /wd4477 /wd4068 /wd4133 /wd4311)
  add_link_options(/IGNORE:4286 /IGNORE:4217)
else()
  add_compile_options(-Wall -Wstrict-aliasing -Wuninitialized -Wno-conversion -Wno-overloaded-virtual -Wno-sign-compare -Wno-comment -Wno-unknown-pragmas -Wno-unused-result)
  add_link_options(-Wno-psabi)
endif()

message(STATUS "Preparing documentation")
execute_process(COMMAND python3 parse_documentation.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Scripts)

message(STATUS "Packaging resources")
execute_process(COMMAND python3 package_resources.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Scripts)


set(PLUGDATA_VERSION                    "0.8.3")
set(PLUGDATA_COMPANY_NAME               "plugdata")
set(PLUGDATA_COMPANY_COPYRIGHT          "plugdata by Timothy Schoen")
set(PLUGDATA_COMPANY_WEBSITE            "github.com/plugdata-team/plugdata")

if("${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
set(PLUGDATA_ICON_BIG                   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_logo_ios.png")
elseif(APPLE)
set(PLUGDATA_ICON_BIG                   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_logo_mac.png")
else()
set(PLUGDATA_ICON_BIG                   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_logo.png")
endif()

set(PLUGDATA_PLUGINS_LOCATION           "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")


set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB plugdata_resources
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/IconFont.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterUnicode_*.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterTabular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterBold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterSemiBold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterThin.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterVariable.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/RobotoMono-Regular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_large_logo.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_logo.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filesystem_*.zip
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Documentation.bin
    )

file(GLOB StandaloneBinarySources
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Extra/GeneralUser_GS.sf3
    )

juce_add_binary_data(PlugDataBinaryData SOURCES ${plugdata_resources})
set_target_properties(PlugDataBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_binary_data(StandaloneBinaryData HEADER_NAME "StandaloneBinaryData.h" NAMESPACE StandaloneBinaryData SOURCES ${StandaloneBinarySources} )
set_target_properties(StandaloneBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)


file(GLOB plugdata_global_sources
    ${CMAKE_CACHEFILE_DIR}/plugdata_artefacts/JuceLibraryCode/JuceHeader.h)

file(GLOB plugdata_sources
    ${SOURCES_DIRECTORY}/*.h
    ${SOURCES_DIRECTORY}/*.cpp
    ${SOURCES_DIRECTORY}/Utility/*.h
    ${SOURCES_DIRECTORY}/Utility/*.cpp
    ${SOURCES_DIRECTORY}/Tabbar/*.h
    ${SOURCES_DIRECTORY}/Tabbar/*.cpp
    ${SOURCES_DIRECTORY}/Components/*.h
    ${SOURCES_DIRECTORY}/Components/*.cpp
    ${SOURCES_DIRECTORY}/Objects/*.h
    ${SOURCES_DIRECTORY}/Objects/*.cpp
    ${SOURCES_DIRECTORY}/Sidebar/*.h
    ${SOURCES_DIRECTORY}/Sidebar/*.cpp
    ${SOURCES_DIRECTORY}/Dialogs/*.h
    ${SOURCES_DIRECTORY}/Dialogs/*.cpp
    ${SOURCES_DIRECTORY}/Pd/*.cpp
    ${SOURCES_DIRECTORY}/Pd/*.h
    ${SOURCES_DIRECTORY}/Heavy/*.cpp
    ${SOURCES_DIRECTORY}/Heavy/*.h
)

if(APPLE)
  list(APPEND plugdata_sources ${SOURCES_DIRECTORY}/Utility/FileSystemWatcher.mm ${SOURCES_DIRECTORY}/Utility/OSUtils.mm)
else()
  list(APPEND plugdata_sources ${SOURCES_DIRECTORY}/Utility/FileSystemWatcher.cxx)
endif()

# Get branch name
execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

if(NOT ${GIT_BRANCH} STREQUAL "main")
# Get current git hash if we're not on the main branch (meaning it's a pre-release version)
  execute_process(
          COMMAND git log -1 --format=%h
          WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
          OUTPUT_VARIABLE GIT_HASH
          OUTPUT_STRIP_TRAILING_WHITESPACE
          )
endif()

set(JUCE_COMPILE_DEFINITIONS
    JUCE_DONT_DECLARE_PROJECTINFO=1
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_ENABLE_REPAINT_DEBUGGING=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WEB_BROWSER=0 
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_USE_COREIMAGE_LOADER=0
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
    JUCE_USE_XRENDER=1
    JUCE_ALSA=1
    JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=1
    #JUCE_JACK=1
    #JUCE_JACK_CLIENT_NAME="plugdata"
)

set(PLUGDATA_COMPILE_DEFINITIONS
    PLUGDATA=1
    PLUGDATA_VERSION="${PLUGDATA_VERSION}"
    PLUGDATA_GIT_HASH="${GIT_HASH}"
    PD=1
)
if(ENABLE_SFIZZ)
  list(APPEND PLUGDATA_COMPILE_DEFINITIONS ENABLE_SFIZZ=1)
endif()

add_library(juce STATIC)
target_compile_definitions(juce 
    PUBLIC 
        ${JUCE_COMPILE_DEFINITIONS} 
    INTERFACE
        $<TARGET_PROPERTY:juce,COMPILE_DEFINITIONS>
    )

target_link_libraries(juce
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        juce::juce_cryptography
    PUBLIC
        juce::juce_recommended_lto_flags
        #juce::juce_recommended_warning_flags
        )

set(libs
  juce
  PlugDataBinaryData
)

if(UNIX AND NOT APPLE)
    list(APPEND libs curl X11)
endif()

list(APPEND PLUGDATA_COMPILE_DEFINITIONS JUCE_MODAL_LOOPS_PERMITTED=1)

list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/ELSE/sfont~/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/JUCE/modules")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/readerwriterqueue/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/concurrentqueue/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/BarelyML/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Source/")

add_library(plugdata_core STATIC ${plugdata_sources} ${plugdata_global_sources})
target_compile_definitions(plugdata_core PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_include_directories(plugdata_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Tests ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/raw-keyboard-input-module ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/Libraries)
target_link_libraries(plugdata_core PUBLIC ${libs})
target_include_directories(plugdata_core PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")

source_group("Source" FILES ${plugdata_global_sources})

foreach(core_SOURCE ${plugdata_sources})
		# Get the path of the file relative to the current source directory
		file(RELATIVE_PATH core_SOURCE_relative "${SOURCES_DIRECTORY}" "${core_SOURCE}")

		# Get the relative folder path
		get_filename_component(core_SOURCE_dir "${core_SOURCE_relative}" PATH)

		# Convert forward slashes to backslashes to get source group identifiers
		string(REPLACE "/" "\\" core_SOURCE_group "${core_SOURCE_dir}")

		source_group("Source\\${core_SOURCE_group}" FILES "${core_SOURCE}")
endforeach()

file(GLOB plugdata_standalone_sources
    ${SOURCES_DIRECTORY}/Standalone/PlugDataApp.cpp
    ${SOURCES_DIRECTORY}/Standalone/PlugDataWindow.h
    ${SOURCES_DIRECTORY}/Standalone/InternalSynth.h)
source_group("Source\\Standalone" FILES ${plugdata_standalone_sources})

if(NOT "${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
juce_add_gui_app(plugdata_standalone
    PRODUCT_NAME                "plugdata"
    VERSION                     ${PLUGDATA_VERSION}
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    MICROPHONE_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_ENABLED   TRUE
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    DOCUMENT_BROWSER_ENABLED     TRUE
    DOCUMENT_EXTENSIONS          .pd
    BUNDLE_ID                    com.plugdata.plugdata
    )
else()
set(IOS_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight)
juce_add_plugin(plugdata_standalone
    VERSION                     ${PLUGDATA_VERSION}
    COMPANY_NAME                ${PLUGDATA_COMPANY_NAME}
    COMPANY_COPYRIGHT           ${PLUGDATA_COMPANY_COPYRIGHT}
    COMPANY_WEBSITE             ${PLUGDATA_COMPANY_WEBSITE}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    LAUNCH_STORYBOARD_FILE      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/LaunchScreen.storyboard
    IPHONE_SCREEN_ORIENTATIONS  ${IOS_SCREEN_ORIENTATIONS}
    STATUS_BAR_HIDDEN           TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    PlDt
    PLUGIN_CODE                 PdIn
    FORMATS                     Standalone
    LV2URI                      https://github.com/timothyschoen/plugdata
    PRODUCT_NAME                "plugdata"
    BUNDLE_ID                   com.plugdata.plugdata
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument
    VST2_CATEGORY               kPlugCategSynth)
endif()

juce_add_plugin(plugdata
    VERSION                     ${PLUGDATA_VERSION}
    COMPANY_NAME                ${PLUGDATA_COMPANY_NAME}
    COMPANY_COPYRIGHT           ${PLUGDATA_COMPANY_COPYRIGHT}
    COMPANY_WEBSITE             ${PLUGDATA_COMPANY_WEBSITE}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    MICROPHONE_PERMISSION_ENABLED TRUE
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    PlDt
    PLUGIN_CODE                 PdIn
    FORMATS                     AU AUv3 VST3 LV2 CLAP
    LV2URI                      https://github.com/timothyschoen/plugdata
    PRODUCT_NAME                "plugdata"
    BUNDLE_ID                   com.plugdata.plugdata.instrument
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument
    VST2_CATEGORY               kPlugCategSynth)

juce_add_plugin(plugdata_fx
    VERSION                     ${PLUGDATA_VERSION}
    COMPANY_NAME                ${PLUGDATA_COMPANY_NAME}
    COMPANY_COPYRIGHT           ${PLUGDATA_COMPANY_COPYRIGHT}
    COMPANY_WEBSITE             ${PLUGDATA_COMPANY_WEBSITE}
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    PlDt
    PLUGIN_CODE                 PdFx
    FORMATS                     AU AUv3 VST3 LV2 CLAP
    LV2URI                      https://github.com/timothyschoen/plugdata-fx
    PRODUCT_NAME                "plugdata-fx"
    BUNDLE_ID                   com.plugdata.plugdata.fx
    AU_MAIN_TYPE                kAudioUnitType_MusicEffect
    VST3_CATEGORIES             Fx
    VST2_CATEGORY               kPlugCategEffect)



if(APPLE)
juce_add_plugin(plugdata_midi
    VERSION                     ${PLUGDATA_VERSION}
    COMPANY_NAME                ${PLUGDATA_COMPANY_NAME}
    COMPANY_COPYRIGHT           ${PLUGDATA_COMPANY_COPYRIGHT}
    COMPANY_WEBSITE             ${PLUGDATA_COMPANY_WEBSITE}
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    PlDt
    PLUGIN_CODE                 PdMd
    FORMATS                     AU
    PRODUCT_NAME                "plugdata-midi"
    AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
  # Silence excessive messaging from juce-clap-extensions
  # Enable verbose flag to see these messages
  if(VERBOSE)
  else()
  set(MESSAGE_QUIET ON)
  endif()
  clap_juce_extensions_plugin(TARGET plugdata
      CLAP_ID "com.timothyschoen.plugdata"
      CLAP_FEATURES "instrument")

  clap_juce_extensions_plugin(TARGET plugdata_fx
      CLAP_ID "com.timothyschoen.plugdata-fx"
      CLAP_FEATURES "effect")

  if(VERBOSE)
  else()
  unset(MESSAGE_QUIET)
  endif()
else()
  message(NOTICE "\n\nWarning: Building CLAP plugins requires at least cmake 3.21, you have ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
  message(NOTICE "CLAP plugin build will be disabed\n\n")
endif()

target_sources(plugdata_standalone PUBLIC ${plugdata_standalone_sources} ${SOURCES_DIRECTORY}/Utility/Config.cpp ${SOURCES_DIRECTORY}/Standalone/InternalSynth.cpp)
target_sources(plugdata PUBLIC ${SOURCES_DIRECTORY}/Utility/Config.cpp ${SOURCES_DIRECTORY}/Standalone/InternalSynth.cpp)
target_sources(plugdata_fx PUBLIC ${SOURCES_DIRECTORY}/Utility/Config.cpp ${SOURCES_DIRECTORY}/Standalone/InternalSynth.cpp)
if(APPLE)
target_sources(plugdata_midi PUBLIC ${SOURCES_DIRECTORY}/Utility/Config.cpp ${SOURCES_DIRECTORY}/Standalone/InternalSynth.cpp)
endif()

target_compile_definitions(plugdata_standalone PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS} JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 PLUGDATA_STANDALONE=1)
target_compile_definitions(plugdata PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_compile_definitions(plugdata_fx PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS} PLUGDATA_FX=1)

if(APPLE)
target_compile_definitions(plugdata_midi PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS} PLUGDATA_MIDI=1)
endif()

target_include_directories(plugdata_standalone PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
target_include_directories(plugdata PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
target_include_directories(plugdata_fx PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")

if(APPLE)
target_include_directories(plugdata_midi PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
endif()

# Add pd file icons for mac
if(APPLE)
set_target_properties(plugdata_standalone PROPERTIES
  RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/pd-file.icns")
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd-src externals StandaloneBinaryData "-Wl,-export-dynamic")
    target_link_libraries(plugdata PRIVATE plugdata_core pd-src-multi externals-multi)
    target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-src-multi externals-multi)
elseif(APPLE)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 15.0)
    set(MACOS_COMPAT_LINKER_FLAGS "-Wl,-ld_classic")
  endif()

  if(NOT "${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
    set(LINK_CARBON "-framework Carbon")
  endif()
  target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd-src externals StandaloneBinaryData ${LINK_CARBON} ${MACOS_COMPAT_LINKER_FLAGS})
  target_link_libraries(plugdata_midi PRIVATE plugdata_core pd-src-multi externals-multi ${LINK_CARBON} ${MACOS_COMPAT_LINKER_FLAGS})
  target_link_libraries(plugdata PRIVATE plugdata_core pd-src-multi externals-multi ${LINK_CARBON} ${MACOS_COMPAT_LINKER_FLAGS})
  target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-src-multi externals-multi ${LINK_CARBON} ${MACOS_COMPAT_LINKER_FLAGS})
else()
  target_link_libraries(plugdata PRIVATE plugdata_core pd-multi)
  target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-multi)
  target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd StandaloneBinaryData)
endif()

set_target_properties(plugdata_standalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES BUNDLE_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES BUNDLE_OUTPUT_DIRECTORY_DEBUG ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES BUNDLE_OUTPUT_DIRECTORY_RELEASE ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PLUGDATA_PLUGINS_LOCATION}/Standalone)
set_target_properties(plugdata_standalone PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PLUGDATA_PLUGINS_LOCATION}/Standalone)

set_target_properties(plugdata PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(plugdata PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})

set_target_properties(plugdata_fx PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(plugdata_fx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})

if(APPLE)
set_target_properties(plugdata_midi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(plugdata_midi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
endif()

if(RUN_CLANG_TIDY)
  find_program( CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable" )
  if(NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
  else()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=clang-diagnostic-*,clang-diagnostic-unused-value,clang-analyzer-*,bugprone-*,performance-*,readability-*,-readability-magic-numbers,-readability-braces-around-statements,-readability-inconsistent-declaration-parameter-name,-readability-named-parameter  --header-filter='.*'")
  endif()
  set_target_properties(plugdata_standalone PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
endif()

# Set up testing framework
if(ENABLE_TESTING)

enable_testing()
if(APPLE)
file(GLOB_RECURSE TestFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.mm")
else()
file(GLOB_RECURSE TestFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp")
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Tests PREFIX "" FILES ${TestFiles})

# Include Catch2
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Catch2)
set(Catch2_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Catch2)

add_executable(Tests ${TestFiles})
set_target_properties(Tests PROPERTIES CXX_STANDARD 20)

target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain plugdata ${libs})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Tests PREFIX "" FILES ${TestFiles})
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(Tests)

target_compile_definitions(Tests PUBLIC TESTING=1)

target_include_directories(Tests PUBLIC PLUGDATA_INCLUDE_DIRECTORY)
target_include_directories(Tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Tests/)
target_include_directories(Tests PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")

set_target_properties(Tests PROPERTIES PREFIX "")
set_target_properties(Tests PROPERTIES CMAKE_CXX_VISIBILITY_PRESET hidden)
set_target_properties(Tests PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(Tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_property(TARGET Tests PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET Tests PROPERTY VISIBILITY_INLINES_HIDDEN ON)

endif()

if(MSVC)
set_target_properties(pthreadVC3 pthreadVSE3 pthreadVCE3 PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif()

set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
  WORLD_READ
  WORLD_WRITE
  WORLD_EXECUTE
)

if(APPLE)
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata-fx.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata-midi.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata.app DESTINATION "/Applications")
elseif(WIN32)
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata.exe DESTINATION "$ENV{PROGRAMFILES}/plugdata/")
  install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/LV2/pd.dll DESTINATION "$ENV{PROGRAMFILES}/plugdata/")
elseif(UNIX AND NOT APPLE) # Linux or BSD
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "$ENV{HOME}/.vst3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "$ENV{HOME}/.vst3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "$ENV{HOME}/.lv2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "$ENV{HOME}/.lv2")
  #TODO: install standalone and Kott's .desktop file
endif()

#add_custom_target(plugdata_standalone ALL)
