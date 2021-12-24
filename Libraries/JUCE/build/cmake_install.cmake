# Install script for directory: /Users/timschoen/JUCE

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/timschoen/JUCE/build/modules/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/timschoen/JUCE/build/extras/Build/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/JUCE-6.1.0" TYPE FILE FILES
    "/Users/timschoen/JUCE/build/JUCEConfigVersion.cmake"
    "/Users/timschoen/JUCE/build/JUCEConfig.cmake"
    "/Users/timschoen/JUCE/extras/Build/CMake/JUCEHelperTargets.cmake"
    "/Users/timschoen/JUCE/extras/Build/CMake/JUCECheckAtomic.cmake"
    "/Users/timschoen/JUCE/extras/Build/CMake/JUCEUtils.cmake"
    "/Users/timschoen/JUCE/extras/Build/CMake/LaunchScreen.storyboard"
    "/Users/timschoen/JUCE/extras/Build/CMake/PIPAudioProcessor.cpp.in"
    "/Users/timschoen/JUCE/extras/Build/CMake/PIPComponent.cpp.in"
    "/Users/timschoen/JUCE/extras/Build/CMake/PIPConsole.cpp.in"
    "/Users/timschoen/JUCE/extras/Build/CMake/RecentFilesMenuTemplate.nib"
    "/Users/timschoen/JUCE/extras/Build/CMake/UnityPluginGUIScript.cs.in"
    "/Users/timschoen/JUCE/extras/Build/CMake/copyDir.cmake"
    "/Users/timschoen/JUCE/extras/Build/CMake/juce_runtime_arch_detection.cpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/timschoen/JUCE/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
