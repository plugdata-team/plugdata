cmake_minimum_required(VERSION 3.12)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(RUN_CLANG_TIDY "" OFF)
option(ENABLE_TESTING "" OFF)
option(ENABLE_SFONT "" ON)

set (CMAKE_CXX_STANDARD 20)


if(APPLE)
  set(CMAKE_XCODE_BUILD_SYSTEM "12" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version")
  if(RUN_CLANG_TIDY)
  set(CMAKE_OSX_ARCHITECTURES "arm64;" CACHE STRING "" FORCE)
  else()
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  endif()
endif()

if(RUN_CLANG_TIDY)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES}) 
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(HARDENED_RUNTIME_ENABLED YES)
set(HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input")

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

project(PlugData VERSION 0.6.1 LANGUAGES C CXX)

add_subdirectory(Libraries/ EXCLUDE_FROM_ALL)

set(PLUGDATA_VERSION                    "0.6.1")
set(PLUGDATA_COMPANY_NAME               "PlugData")
set(PLUGDATA_COMPANY_COPYRIGHT          "PlugData by Timothy Schoen")
set(PLUGDATA_COMPANY_WEBSITE            "github.com/timothyschoen/plugdata")
set(PLUGDATA_ICON_BIG                   "${CMAKE_CURRENT_SOURCE_DIR}/Resources/plugd_logo.png")
set(PLUGDATA_PLUGINS_LOCATION           "${CMAKE_CURRENT_SOURCE_DIR}/Plugins")




set(SOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source)

file(GLOB PlugDataGlobalSources
    ${CMAKE_CACHEFILE_DIR}/PlugData_artefacts/JuceLibraryCode/JuceHeader.h)
source_group("Source" FILES ${PlugDataGlobalSources})

file(GLOB PlugDataSources
    ${SOURCES_DIRECTORY}/*.h
    ${SOURCES_DIRECTORY}/*.cpp
    ${SOURCES_DIRECTORY}/Utility/*.h
    ${SOURCES_DIRECTORY}/Utility/*.cpp
    ${SOURCES_DIRECTORY}/Objects/*.h
    ${SOURCES_DIRECTORY}/Objects/*.cpp
    ${SOURCES_DIRECTORY}/Sidebar/*.h
    ${SOURCES_DIRECTORY}/Sidebar/*.cpp
    ${SOURCES_DIRECTORY}/Dialogs/*.h
    ${SOURCES_DIRECTORY}/Dialogs/*.cpp
    ${SOURCES_DIRECTORY}/Pd/*.cpp
    ${SOURCES_DIRECTORY}/Pd/*.h
)

if(APPLE)
  list(APPEND PlugDataSources ${SOURCES_DIRECTORY}/Utility/FileSystemWatcher.mm)
else()
  list(APPEND PlugDataSources ${SOURCES_DIRECTORY}/Utility/FileSystemWatcher.cxx)
endif()

foreach(core_SOURCE ${PlugDataSources})
		# Get the path of the file relative to the current source directory
		file(RELATIVE_PATH core_SOURCE_relative "${SOURCES_DIRECTORY}" "${core_SOURCE}")

		# Get the relative folder path
		get_filename_component(core_SOURCE_dir "${core_SOURCE_relative}" PATH)

		# Convert forward slashes to backslashes to get source group identifiers
		string(REPLACE "/" "\\" core_SOURCE_group "${core_SOURCE_dir}")

		source_group("Source\\${core_SOURCE_group}" FILES "${core_SOURCE}")
endforeach()

file(GLOB PlugDataStandaloneSources
    ${SOURCES_DIRECTORY}/Standalone/*.cpp
    ${SOURCES_DIRECTORY}/Standalone/*.h)
source_group("Source\\Standalone" FILES ${PlugDataStandaloneSources})

file(GLOB PdRemoteSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Remote/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Remote/Source/*.h)
source_group("Remote\\Source" FILES ${PdRemoteSources})


#source_group("Source\\Pd" FILES ${PlugDataPdSources})

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Info.plist" INFO_PLIST)

juce_add_gui_app(PdRemote
    VERSION                     ${PLUGDATA_VERSION}
    COMPANY_NAME                ${PLUGDATA_COMPANY_NAME}
    COMPANY_COPYRIGHT           ${PLUGDATA_COMPANY_COPYRIGHT}
    COMPANY_WEBSITE             ${PLUGDATA_COMPANY_WEBSITE}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    MICROPHONE_PERMISSION_ENABLED TRUE
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    PRODUCT_NAME                "PdRemote"
    PLIST_TO_MERGE              ${INFO_PLIST}
    )

juce_add_plugin(PlugDataStandalone
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
    PLUGIN_CODE                 PlDt
    FORMATS                     Standalone
    PRODUCT_NAME                "PlugData"
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument
    VST2_CATEGORY               kPlugCategSynth
    PLIST_TO_MERGE              ${INFO_PLIST}
    )

juce_add_plugin(PlugData
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
    FORMATS                     AU VST3 LV2
    LV2URI                      https://github.com/timothyschoen/PlugData
    PRODUCT_NAME                "PlugData"
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument
    VST2_CATEGORY               kPlugCategSynth)

juce_add_plugin(PlugDataFx
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
    FORMATS                     AU VST3 LV2
    LV2URI                      https://github.com/timothyschoen/PlugDataFx
    PRODUCT_NAME                "PlugDataFx"
    AU_MAIN_TYPE                kAudioUnitType_MusicEffect
    VST3_CATEGORIES             Fx
    VST2_CATEGORY               kPlugCategEffect)

if(APPLE)
juce_add_plugin(PlugDataMidi
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
    PRODUCT_NAME                "PlugDataMidi"
    AU_MAIN_TYPE                kAudioUnitType_MIDIProcessor)
endif()

if(APPLE)
set_target_properties(PlugData PROPERTIES CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set_target_properties(PlugDataFx PROPERTIES CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
endif()

juce_generate_juce_header(PdRemote)
set_target_properties(PdRemote PROPERTIES CXX_STANDARD 20)
target_sources(PdRemote PRIVATE ${PdRemoteSources})

juce_generate_juce_header(PlugDataStandalone)
set_target_properties(PlugDataStandalone PROPERTIES CXX_STANDARD 20)
target_sources(PlugDataStandalone PRIVATE ${PlugDataSources} ${PlugDataStandaloneSources})

juce_generate_juce_header(PlugData)
set_target_properties(PlugData PROPERTIES CXX_STANDARD 20)
target_sources(PlugData PRIVATE ${PlugDataSources} ${PlugDataStandaloneSources})

juce_generate_juce_header(PlugDataFx)
set_target_properties(PlugDataFx PROPERTIES CXX_STANDARD 20)
target_sources(PlugDataFx PRIVATE ${PlugDataSources})

if(APPLE)
juce_generate_juce_header(PlugDataMidi)

set_target_properties(PlugDataMidi PROPERTIES CXX_STANDARD 20)
set_target_properties(PlugDataMidi PROPERTIES CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
target_sources(PlugDataMidi PRIVATE ${PlugDataSources})
endif()


set(LIBPD_MULTI_COMPILE_DEFINITIONS
    PDINSTANCE=1
    PDTHREADS=1
)

set(STANDALONE_COMPILE_DEFINITIONS
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    PLUGDATA_STANDALONE=1)

set(PLUGDATA_COMPILE_DEFINITIONS
    JUCE_APP_CONFIG_HEADER="${SOURCES_DIRECTORY}/PluginConfig.h"
    JUCE_DONT_DECLARE_PROJECTINFO=1
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_LOG_ASSERTIONS=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WEB_BROWSER=0
)


if(UNIX AND NOT APPLE)
    set(PLUGDATA_COMPILE_DEFINITIONS
    ${PLUGDATA_COMPILE_DEFINITIONS}
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_USE_XRENDER=1
    )
    target_link_libraries(PlugDataStandalone PUBLIC "-Wl,-export-dynamic")
    target_link_libraries(PdRemote PUBLIC "-Wl,-export-dynamic")
endif()

target_compile_definitions(PdRemote PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS} ${STANDALONE_COMPILE_DEFINITIONS})
target_compile_definitions(PlugDataStandalone PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS} ${STANDALONE_COMPILE_DEFINITIONS})

target_compile_definitions(PlugData PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_compile_definitions(PlugData PUBLIC ${LIBPD_MULTI_COMPILE_DEFINITIONS})

target_compile_definitions(PlugDataFx PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_compile_definitions(PlugDataFx PUBLIC ${LIBPD_MULTI_COMPILE_DEFINITIONS})

if(APPLE)
target_compile_definitions(PlugDataMidi PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_compile_definitions(PlugDataMidi PUBLIC ${LIBPD_MULTI_COMPILE_DEFINITIONS})
endif()

list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/libpd/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/concurrentqueue/")

list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Source/")

target_include_directories(PdRemote PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
target_include_directories(PlugDataStandalone PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
target_include_directories(PlugData PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
target_include_directories(PlugDataFx PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")

if(APPLE)
target_include_directories(PlugDataMidi PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
endif()

file(GLOB PlugDataBinaryDataSources
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/PlugDataFont.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/InterRegular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/plugd_logo.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filesystem.zip
    )

juce_add_binary_data(PlugDataBinaryData SOURCES ${PlugDataBinaryDataSources})
set_target_properties(PlugDataBinaryData PROPERTIES POSITION_INDEPENDENT_CODE ON)


set(libs
  PlugDataBinaryData
  juce::juce_audio_utils
  juce::juce_audio_plugin_client
  juce::juce_dsp
)

# Add pd file icons for mac
if(APPLE)
set_target_properties(PlugDataStandalone PROPERTIES
  RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd-file.icns")
endif()

if(MSVC)
  list(APPEND libs libpthreadVC3)
elseif(APPLE)
  target_link_libraries(PlugDataMidi PRIVATE pd-multi ${libs})
elseif(UNIX AND NOT APPLE)
  list(APPEND libs curl)
endif()

target_link_libraries(PdRemote PRIVATE pd ${libs})

if(MSVC)
#target_compile_definitions(PlugDataStandalone PRIVATE PD_INTERNAL=0)
target_link_libraries(PlugDataStandalone PRIVATE pd ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Standalone/pd.lib ${libs})
else()
target_link_libraries(PlugDataStandalone PRIVATE pd ${libs})
endif()
target_link_libraries(PlugData PRIVATE pd-multi ${libs})
target_link_libraries(PlugDataFx PRIVATE pd-multi ${libs})

set_target_properties(PlugDataStandalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(PlugDataStandalone PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})

set_target_properties(PlugData PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(PlugData PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})

set_target_properties(PlugDataFx PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(PlugDataFx PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})

if(APPLE)
set_target_properties(PlugDataMidi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
set_target_properties(PlugDataMidi PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PLUGDATA_PLUGINS_LOCATION})
endif()

if(RUN_CLANG_TIDY)
  find_program( CLANG_TIDY_EXE NAMES "clang-tidy" DOC "Path to clang-tidy executable" ) 
  if(NOT CLANG_TIDY_EXE) 
    message(STATUS "clang-tidy not found.") 
  else() 
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}") 
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=clang-diagnostic-*,clang-diagnostic-unused-value,clang-analyzer-*,bugprone-*,performance-*,readability-*,-readability-magic-numbers,-readability-braces-around-statements,-readability-inconsistent-declaration-parameter-name,-readability-named-parameter  --header-filter='.*'") 
  endif()
  set_target_properties(PlugDataStandalone PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

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

target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain PlugDataStandalone ${libs})

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
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugData.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugDataFx.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugData.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugDataFx.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/AU/PlugData.component DESTINATION "/Library/Audio/Plug-ins/Components/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/AU/PlugDataFx.component DESTINATION "/Library/Audio/Plug-ins/Components/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/AU/PlugDataMidi.component DESTINATION "/Library/Audio/Plug-ins/Components/")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Standalone/PlugData.app DESTINATION "/Applications")
elseif(WIN32)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugData.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugDataFx.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugData.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugDataFx.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Standalone/PlugData.exe DESTINATION "$ENV{PROGRAMFILES}/PlugData/")
  install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/pd.dll DESTINATION "$ENV{PROGRAMFILES}/PlugData/")
elseif(UNIX AND NOT APPLE) # Linux or BSD
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugData.vst3 DESTINATION "$ENV{HOME}/.vst3")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/VST3/PlugDataFx.vst3 DESTINATION "$ENV{HOME}/.vst3")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugData.lv2 DESTINATION "$ENV{HOME}/.lv2")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/LV2/PlugDataFx.lv2 DESTINATION "$ENV{HOME}/.lv2")
  #TODO: install standalone and Kott's .desktop file
endif()

# 
