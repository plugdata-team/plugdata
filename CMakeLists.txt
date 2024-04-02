cmake_minimum_required(VERSION 3.15)

# Set project name and version
project(plugdata VERSION 0.8.3 LANGUAGES C CXX)

# Set compiler options and flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type: Release" FORCE)
endif()

# Set macOS-specific options
if(APPLE)
  set(CMAKE_XCODE_BUILD_SYSTEM "12" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
endif()

# Enable/disable certain features based on platform
if(("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  OR ("${CMAKE_SYSTEM_NAME}" MATCHES "iOS"))
  message(STATUS "Disabled sfizz")
  set(ENABLE_SFIZZ OFF)
endif()

# Set up build flags and options
option(RUN_CLANG_TIDY "Enable clang-tidy" OFF)
option(ENABLE_TESTING "Enable testing" OFF)
option(ENABLE_SFIZZ "Enable sfizz" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(VERBOSE "Verbose output" OFF)

# Include necessary scripts
include(Utilities.cmake)

# Add subdirectories
add_subdirectory(Libraries/ EXCLUDE_FROM_ALL)

# Set global properties
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Set compiler flags
set_compiler_flags()

# Set up macOS-specific linker flags
set_macos_linker_flags()

# Set up testing framework if enabled
if(ENABLE_TESTING)
  setup_testing_framework()
endif()

# Add target: juce
add_juce_target()

# Add target: plugdata_core
add_plugdata_core_target()

# Add target: plugdata_standalone
add_plugdata_standalone_target()

# Add target: plugdata
add_plugdata_target()

# Add target: plugdata_fx
add_plugdata_fx_target()

# Add target: plugdata_midi (if applicable)
if(APPLE)
  add_plugdata_midi_target()
endif()

# Install targets and resources
install_targets_and_resources()
