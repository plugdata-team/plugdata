cmake_minimum_required(VERSION 3.15)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python3 not found - required for build scripts")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(QUICK_BUILD "" OFF)
option(ENABLE_TESTING "" OFF)
option(ENABLE_SFIZZ "" ON)
option(ENABLE_FFMPEG "" ON)
option(ENABLE_GEM "" OFF)
option(ENABLE_ASAN "" OFF)
option(ENABLE_PERFETTO "" OFF)
option(MACOS_LEGACY "" OFF)
option(VERBOSE "" OFF)

if(APPLE)
option(NANOVG_METAL_IMPLEMENTATION "" ON)
else()
option(NANOVG_METAL_IMPLEMENTATION "" OFF)
endif()

option(NANOVG_GLES_IMPLEMENTATION "" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

# Visiblity needs to be hidden for all plugin targets, otherwise loading both plugdata and plugdata-fx will cause problems. We later undo this for the standalone build, so that externals can load
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)

function(message)
    if (NOT MESSAGE_QUIET)
        _message(${ARGN})
    endif()
endfunction()

if(APPLE)
  set(CMAKE_XCODE_BUILD_SYSTEM "12" CACHE STRING "" FORCE)
  if(MACOS_LEGACY)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;" CACHE STRING "" FORCE)
  else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  endif()

endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Default build type: Release" FORCE)
endif()

set(HARDENED_RUNTIME_ENABLED YES)
set(HARDENED_RUNTIME_OPTIONS com.apple.security.device.audio-input com.apple.security.device.camera)

project(plugdata VERSION 0.9.0 LANGUAGES C CXX)

if(QUICK_BUILD)
    set(ENABLE_SFIZZ OFF)
    set(ENABLE_GEM OFF)
    set(ENABLE_FFMPEG OFF)
endif()

if(("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  OR ("${CMAKE_SYSTEM_NAME}" MATCHES "iOS"))
  message(STATUS "Disabled sfizz")
  set(ENABLE_SFIZZ OFF)
endif()

if(BSD)
message(STATUS "Disabled sfizz")
message(STATUS "Disabled Gem")
set(ENABLE_GEM OFF)
set(ENABLE_SFIZZ OFF)
endif()

add_subdirectory(Libraries/ EXCLUDE_FROM_ALL)
if(ENABLE_PERFETTO)
  add_subdirectory(Libraries/melatonin_perfetto EXCLUDE_FROM_ALL)
endif()

set(PLUGDATA_VERSION "0.9.2")
set_directory_properties(PROPERTIES JUCE_COMPANY_NAME "plugdata")
set_directory_properties(PROPERTIES JUCE_COMPANY_COPYRIGHT "plugdata by Timothy Schoen")
set_directory_properties(PROPERTIES JUCE_COMPANY_WEBSITE "plugdata.org")

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
  add_compile_options("$<$<CONFIG:Debug>:/bigobj>")
  add_link_options(/IGNORE:4286 /IGNORE:4217)
else()
  add_compile_options(-Wall -Wstrict-aliasing -Wuninitialized -Wno-conversion -Wno-overloaded-virtual -Wno-sign-compare -Wno-comment -Wno-unknown-pragmas -Wno-unused-result)
  add_link_options(-Wno-psabi)
endif()

message(STATUS "Preparing documentation")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Resources)
execute_process(COMMAND ${Python3_EXECUTABLE} parse_documentation.py ${CMAKE_CURRENT_BINARY_DIR}/Resources WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Scripts RESULT_VARIABLE PREPARE_DOCUMENTATION_RESULT)

if(NOT PREPARE_DOCUMENTATION_RESULT EQUAL 0)
    message(FATAL_ERROR "Preparing documentation failed with error code ${PREPARE_DOCUMENTATION_RESULT}")
endif()

message(STATUS "Packaging resources")
execute_process(COMMAND ${Python3_EXECUTABLE} package_resources.py ${ENABLE_GEM} ${CMAKE_CURRENT_BINARY_DIR}/Resources WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Scripts RESULT_VARIABLE PACKAGE_RESOURCES_RESULT)

if(NOT PACKAGE_RESOURCES_RESULT EQUAL 0)
    message(FATAL_ERROR "Resource packaging failed with error code ${PACKAGE_RESOURCES_RESULT}")
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterTabular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterBold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterSemiBold.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterThin.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterVariable.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/InterRegular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/RobotoMono-Regular.ttf
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_large_logo.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_logo.png
    # Generated resources
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/Documentation.bin
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/InterUnicode_*.ttf
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/Filesystem_*.zip
    )


file(GLOB StandaloneBinarySources
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Extra/GeneralUser_GS.sf3
    )

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


# Set up testing
if(ENABLE_TESTING)
    list(APPEND plugdata_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Tests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/HelpfileFuzzTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ObjectFuzzTest.h
    )

endif()

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
    JUCE_COREGRAPHICS_RENDER_WITH_MULTIPLE_PAINT_CALLS=0
    JUCE_USE_DIRECTWRITE=0
)

if(LINUX)
 list(APPEND JUCE_COMPILE_DEFINITIONS JUCE_ALSA=1 JUCE_JACK=1 JUCE_JACK_CLIENT_NAME="plugdata")
elseif(UNIX AND NOT APPLE) # BSD
  list(APPEND JUCE_COMPILE_DEFINITIONS JUCE_JACK=1 JUCE_JACK_CLIENT_NAME="plugdata")
  add_compile_definitions(BSD=1 ENABLE_OPUS=0)
endif()

set(PLUGDATA_COMPILE_DEFINITIONS
    PLUGDATA=1
    PLUGDATA_VERSION="${PLUGDATA_VERSION}"
    PLUGDATA_GIT_HASH="${GIT_HASH}"
    PD=1
    ENABLE_TESTING=${ENABLE_TESTING}
    PERFETTO=${ENABLE_PERFETTO}
)
if(ENABLE_SFIZZ)
  list(APPEND PLUGDATA_COMPILE_DEFINITIONS ENABLE_SFIZZ=1)
endif()

if(ENABLE_FFMPEG)
  list(APPEND PLUGDATA_COMPILE_DEFINITIONS ENABLE_FFMPEG=1)
endif()


if(ENABLE_GEM)
  list(APPEND PLUGDATA_COMPILE_DEFINITIONS ENABLE_GEM=1)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Libraries/Gem/src)
endif()

add_library(juce STATIC)
if(MSVC)
  target_link_options(juce INTERFACE /FORCE:MULTIPLE)
endif()

target_compile_definitions(juce
    PUBLIC
        ${JUCE_COMPILE_DEFINITIONS}
    INTERFACE
        $<TARGET_PROPERTY:juce,COMPILE_DEFINITIONS>
    )

if(NOT "${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
target_link_libraries(juce
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        juce::juce_cryptography
        juce::juce_opengl
        melatonin_blur
        )
else()
target_link_libraries(juce
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        juce::juce_cryptography
        melatonin_blur
        )
endif()

target_compile_options(juce PUBLIC $<$<CONFIG:Release>:${JUCE_LTO_FLAGS}>)

if(NANOVG_METAL_IMPLEMENTATION)
add_compile_definitions(NANOVG_METAL_IMPLEMENTATION=1)
elseif(NANOVG_GLES_IMPLEMENTATION)
add_compile_definitions(NANOVG_GLES3_IMPLEMENTATION=1)
else()
add_compile_definitions(NANOVG_GL3_IMPLEMENTATION=1)
endif()

file(GLOB BINARY_DATA_FILES ${CMAKE_CURRENT_BINARY_DIR}/BinaryData/*.cpp)
add_library(BinaryData STATIC ${BINARY_DATA_FILES})
target_include_directories(BinaryData INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/BinaryData)

set(libs
  juce
  BinaryData
  nanovg
)

if(UNIX AND NOT APPLE)
    list(APPEND libs curl X11)
    if(LINUX)
        list(APPEND libs atomic)
    endif()
endif()

# Fixes BSD compilation
if(BSD)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)
endif()

list(APPEND PLUGDATA_COMPILE_DEFINITIONS JUCE_MODAL_LOOPS_PERMITTED=1)

list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/ELSE/sfont~/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/JUCE/modules")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/readerwriterqueue/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/concurrentqueue/")
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/BarelyML/")
if(ENABLE_PERFETTO)
  list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/perfetto-src/sdk")
  list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/melatonin_perfetto")
endif()
list(APPEND PLUGDATA_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Source/")

add_library(plugdata_core STATIC ${plugdata_sources} ${plugdata_global_sources})
target_compile_definitions(plugdata_core PUBLIC ${PLUGDATA_COMPILE_DEFINITIONS})
target_include_directories(plugdata_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Tests ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/raw-keyboard-input-module ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/pure-data/src ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/Libraries)
target_link_libraries(plugdata_core PUBLIC ${libs})
target_include_directories(plugdata_core PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
include_directories(./Libraries/nanovg/src/)

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
set(INSTRUMENT_NAME "plugdata")
set(AU_EFFECT_TYPE kAudioUnitType_MusicEffect)
juce_add_gui_app(plugdata_standalone
    PRODUCT_NAME                "plugdata"
    VERSION                     ${PLUGDATA_VERSION}
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    MICROPHONE_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_ENABLED   TRUE
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    DOCUMENT_EXTENSIONS         pd plugdata
    BUNDLE_ID                   com.plugdata.plugdata
    )
else()
set(INSTRUMENT_NAME "plugdata-instrument")
set(AU_EFFECT_TYPE kAudioUnitType_Effect)

juce_add_plugin(plugdata_standalone
    VERSION                     ${PLUGDATA_VERSION}
    PLUGIN_DESCRIPTION          "A plugin that loads Pure Data patches"
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    LAUNCH_STORYBOARD_FILE      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/LaunchScreen.storyboard
    IPHONE_SCREEN_ORIENTATIONS  UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS    UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
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
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    DOCUMENT_EXTENSIONS         pd plugdata
    DOCUMENT_BROWSER_ENABLED    TRUE
    BUNDLE_ID                   com.plugdata.plugdata
    APP_GROUPS_ENABLED          TRUE
    APP_GROUP_IDS               group.com.plugdata.plugdata
    PLIST_TO_MERGE              [[<plist>
                                    <dict>
                                        <key>CFBundleIdentifier</key>
                                        <string>com.plugdata.plugdata</string>
                                        <key>UIBackgroundModes</key>
                                        <array>
                                            <string>audio</string>
                                        </array>
                                    </dict>
                                </plist>]]
    )

    file(GLOB IOS_LAUNCHSCREEN "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/plugdata_launchscreen_ios.png")
    set_source_files_properties(${IOS_LAUNCHSCREEN} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(plugdata_standalone_Standalone PUBLIC ${IOS_LAUNCHSCREEN})
endif()



juce_add_plugin(plugdata
    VERSION                     ${PLUGDATA_VERSION}
    PLUGIN_DESCRIPTION          "Visual audio programming environment"
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
    PRODUCT_NAME                ${INSTRUMENT_NAME}
    BUNDLE_ID                   com.plugdata.plugdata.instrument
    PLIST_TO_MERGE              [[<plist>
                                    <dict>
                                        <key>CFBundleIdentifier</key>
                                        <string>com.plugdata.plugdata.instrument</string>
                                    </dict>
                                </plist>]]
    APP_GROUPS_ENABLED          TRUE
    APP_GROUP_IDS               group.com.plugdata.plugdata
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument
    VST2_CATEGORY               kPlugCategSynth)

juce_add_plugin(plugdata_fx
    VERSION                     ${PLUGDATA_VERSION}
    PLUGIN_DESCRIPTION          "Visual audio programming environment"
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
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
    PLIST_TO_MERGE              [[<plist>
                                    <dict>
                                        <key>CFBundleIdentifier</key>
                                        <string>com.plugdata.plugdata.fx</string>
                                    </dict>
                                </plist>]]
    APP_GROUPS_ENABLED          TRUE
    APP_GROUP_IDS               group.com.plugdata.plugdata
    AU_MAIN_TYPE                ${AU_EFFECT_TYPE}
    VST3_CATEGORIES             Fx
    VST2_CATEGORY               kPlugCategEffect)



if(APPLE)
juce_add_plugin(plugdata_midi
    VERSION                     ${PLUGDATA_VERSION}
    ICON_BIG                    ${PLUGDATA_ICON_BIG}
    HARDENED_RUNTIME_ENABLED    ${HARDENED_RUNTIME_ENABLED}
    HARDENED_RUNTIME_OPTIONS    ${HARDENED_RUNTIME_OPTIONS}
    PLUGIN_DESCRIPTION          "Visual audio programming environment"
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
      CLAP_FEATURES "audio-effect")

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

if("${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
    set_property(TARGET plugdata_standalone_Standalone PROPERTY XCODE_EMBED_APP_EXTENSIONS plugdata_AUv3 plugdata_fx_AUv3)
    # iOS requires that the app extensions bundle IDs start the same as the app, but with a suffix appended
    set_target_properties(plugdata_AUv3 PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.plugdata.plugdata.instrument")
    set_target_properties(plugdata_fx_AUv3 PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.plugdata.plugdata.fx")
    set_target_properties(plugdata_standalone_Standalone PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.plugdata.plugdata")
    set_target_properties(plugdata_standalone_Standalone PROPERTIES OUTPUT_NAME "plugdata_standalone_app")
    set_target_properties(plugdata_standalone_Standalone PROPERTIES
        XCODE_EMBED_APP_EXTENSIONS_REMOVE_HEADERS_ON_COPY "YES"
        XCODE_EMBED_APP_EXTENSIONS_CODE_SIGN_ON_COPY "YES"
)
endif()

if(LINUX)
    target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd "-Wl,-export-dynamic")
    target_link_libraries(plugdata PRIVATE plugdata_core pd-multi)
    target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-multi)
elseif(UNIX AND NOT APPLE) # BSD
    target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd lua fluidlite "-Wl,-export-dynamic")
    target_link_libraries(plugdata PRIVATE plugdata_core pd-multi lua fluidlite "-Wl,-export-dynamic")
    target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-multi lua fluidlite "-Wl,-export-dynamic")
elseif(APPLE)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 15.0)
    set(MACOS_COMPAT_LINKER_FLAGS "-Wl,-ld_classic")
  endif()

  if(NOT "${CMAKE_SYSTEM_NAME}" MATCHES "iOS")
    set(LINK_CARBON "-framework Carbon")
  endif()
  target_link_libraries(plugdata_standalone PRIVATE plugdata_core pd ${LINK_CARBON} $<$<NOT:$<CONFIG:Debug>>:${MACOS_COMPAT_LINKER_FLAGS}>)
  target_link_libraries(plugdata_midi PRIVATE plugdata_core pd-multi ${LINK_CARBON} $<$<NOT:$<CONFIG:Debug>>:${MACOS_COMPAT_LINKER_FLAGS}>)
  target_link_libraries(plugdata PRIVATE plugdata_core pd-multi ${LINK_CARBON} $<$<NOT:$<CONFIG:Debug>>:${MACOS_COMPAT_LINKER_FLAGS}>)
  target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-multi ${LINK_CARBON} $<$<NOT:$<CONFIG:Debug>>:${MACOS_COMPAT_LINKER_FLAGS}>)
else()
  target_link_libraries(plugdata PRIVATE plugdata_core pd-multi)
  target_link_libraries(plugdata_fx PRIVATE plugdata_core pd-multi)
  target_link_libraries(plugdata_standalone PUBLIC pd plugdata_core)
endif()

function(copy_dylib_lv2 TARGET LIB_NAME PATH)
    add_custom_command(TARGET ${TARGET} PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${LIB_NAME}>"
                "${PATH}"
    )
    if(MSVC)
	target_link_options(${TARGET} PRIVATE "/DELAYLOAD:$<TARGET_FILE_BASE_NAME:${LIB_NAME}>.dll")
    endif()
endfunction()

function(copy_dylib TARGET LIB_NAME)
    # Determine the platform
    if(WIN32)
        set(LIB_EXTENSION ".dll")
        set(DEST_DIR "$<TARGET_FILE_DIR:${TARGET}>")
    elseif(APPLE)
        set(LIB_EXTENSION ".dylib")
        set(DEST_DIR "$<TARGET_BUNDLE_CONTENT_DIR:${TARGET}>/Frameworks")
    else()  # Assuming Linux
        set(LIB_EXTENSION ".so")
        set(DEST_DIR "$<TARGET_FILE_DIR:${TARGET}>/lib")
    endif()

    # Define the library path
    set(LIB_PATH $<TARGET_FILE:${LIB_NAME}>)

    # Create destination directory and copy the library
    add_custom_command(TARGET ${TARGET} PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIB_PATH}"
                "${DEST_DIR}/"
    )

    # Platform-specific handling for install name or equivalent on different OSes
    if(APPLE)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND install_name_tool -change "@rpath/libpd.dylib"
                    "@executable_path/../Frameworks/libpd.dylib"
                    "$<TARGET_FILE:${TARGET}>"
        )
    elseif(MSVC)
        target_link_options(${TARGET} PRIVATE "/DELAYLOAD:$<TARGET_FILE_BASE_NAME:${LIB_NAME}>.dll")
    elseif(UNIX)
        add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND patchelf --set-rpath "$ORIGIN/lib" "$<TARGET_FILE:${TARGET}>"
    )
    endif()
endfunction()

copy_dylib(plugdata_standalone pd)
copy_dylib(plugdata_VST3 pd-multi)
copy_dylib(plugdata_CLAP pd-multi)
copy_dylib(plugdata_fx_VST3 pd-multi)
copy_dylib(plugdata_fx_CLAP pd-multi)

copy_dylib_lv2(plugdata_LV2 pd-multi ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2)
copy_dylib_lv2(plugdata_fx_LV2 pd-multi ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2)

if(APPLE)
copy_dylib(plugdata_fx_AU pd-multi)
#copy_dylib(plugdata_fx_AUv3 pd-multi)
copy_dylib(plugdata_AU pd-multi)
#copy_dylib(plugdata_AUv3 pd-multi)
copy_dylib(plugdata_midi_AU pd-multi)
endif()

if(ENABLE_PERFETTO)
  if(MSVC)
    target_compile_options(perfetto
        PUBLIC "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>"
    )
    set_target_properties(perfetto PROPERTIES CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$" )
  endif()

  target_link_libraries(plugdata PRIVATE Melatonin::Perfetto)
  target_link_libraries(plugdata_fx PRIVATE Melatonin::Perfetto)
  target_link_libraries(plugdata_standalone PRIVATE Melatonin::Perfetto)
  if(APPLE)
    target_link_libraries(plugdata_midi PRIVATE Melatonin::Perfetto)
  endif()
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

if(APPLE)
# 2x speedup for standalone debug builds in xcode
set_target_properties(plugdata_standalone PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
endif()

if(MSVC)
set_target_properties(pthreadVSE3 pthreadVCE3 PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
endif()

if(APPLE)
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "/Library/Audio/Plug-ins/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "/Library/Audio/Plug-ins/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata.clap DESTINATION "/Library/Audio/Plug-ins/CLAP")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata-fx.clap DESTINATION "/Library/Audio/Plug-ins/CLAP")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata-fx.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/AU/plugdata-midi.component DESTINATION "/Library/Audio/Plug-ins/Components")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata.app DESTINATION "/Applications")
elseif(WIN32)
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "$ENV{PROGRAMFILES}/Common Files/VST3")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "$ENV{PROGRAMFILES}/Common Files/LV2")
  install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata.clap DESTINATION "$ENV{PROGRAMFILES}/Common Files/CLAP")
  install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata-fx.clap DESTINATION "$ENV{PROGRAMFILES}/Common Files/CLAP")
  install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata.exe DESTINATION "$ENV{PROGRAMFILES}/plugdata/")
  install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/Standalone/pd.dll DESTINATION "$ENV{PROGRAMFILES}/plugdata/")
elseif(UNIX AND NOT APPLE) # Linux or BSD
    if(FLATPAK_PLUGIN)
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION extensions/Plugins/vst3)
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION extensions/Plugins/vst3)
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION extensions/Plugins/lv2)
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION extensions/Plugins/lv2)
      install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata.clap DESTINATION extensions/Plugins/clap)
      install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata-fx.clap DESTINATION extensions/Plugins/clap)
    elseif(FLATPAK_STANDALONE)
      install(FILES ${CMAKE_SOURCE_DIR}/Resources/Icons/plugdata_logo_linux.png DESTINATION share/icons/hicolor/512x512/apps RENAME plugdata.png)
      install(FILES ${CMAKE_SOURCE_DIR}/Resources/Installer/plugdata.desktop DESTINATION share/applications)
      install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata DESTINATION bin)
if(APPLE)
target_include_directories(plugdata_midi PUBLIC "$<BUILD_INTERFACE:${PLUGDATA_INCLUDE_DIRECTORY}>")
endif()
    else()
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata.vst3 DESTINATION "$ENV{HOME}/.vst3")
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/VST3/plugdata-fx.vst3 DESTINATION "$ENV{HOME}/.vst3")
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata.lv2 DESTINATION "$ENV{HOME}/.lv2")
      install(DIRECTORY ${PLUGDATA_PLUGINS_LOCATION}/LV2/plugdata-fx.lv2 DESTINATION "$ENV{HOME}/.lv2")
      install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata.clap DESTINATION "$ENV{HOME}/.clap")
      install(FILES ${PLUGDATA_PLUGINS_LOCATION}/CLAP/plugdata-fx.clap DESTINATION "$ENV{HOME}/.clap")
      install(FILES ${CMAKE_SOURCE_DIR}/Resources/Icons/plugdata_logo_linux.png DESTINATION share/icons/hicolor/512x512/apps RENAME plugdata.png)
      install(FILES ${CMAKE_SOURCE_DIR}/Resources/Installer/plugdata.desktop DESTINATION share/applications)
      install(PROGRAMS ${PLUGDATA_PLUGINS_LOCATION}/Standalone/plugdata DESTINATION bin)
    endif()
endif()
