# Copyright (c) 2011 Stanford University. All rights reserved.
# Use of this source code is governed by a MIT-style license that can
# be found in the LICENSE file.

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

IF(COMMAND CMAKE_POLICY)
  # Use new policy for absolute path libraries.  We have no reason to depend on the
  # old version which adds library search paths unnecessarily
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(APPLE)
    IF(IS_DIRECTORY /Developer/SDKs/MacOSX10.5.sdk)
        SET(CMAKE_CXX_COMPILER g++-4.0)
        SET(CMAKE_C_COMPILER gcc-4.0)
    ELSE()
        IF(EXISTS /usr/bin/g++-4.2)
          SET(CMAKE_CXX_COMPILER g++-4.2)
          SET(CMAKE_C_COMPILER gcc-4.2)
        ELSE()
          # Don't try to override. This is probably a newer setup
          # using XCode 4.2+ which doesn't provide versioned gcc's
          # since it uses llvm.
          #SET(CMAKE_CXX_COMPILER g++)
          #SET(CMAKE_C_COMPILER gcc)
        ENDIF()
    ENDIF()
ENDIF()

PROJECT(liboauthcpp C CXX)
ENABLE_LANGUAGE(C)

# Disable some annoying, unnecessary Windows warnings. Enable unwinding for exception handlers.
IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -EHsc)
  SET(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS")
ENDIF()

# Force a default build type
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
    "Build types include: Default Debug Release None RelWithDebInfo." FORCE
    )
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
# And add a definition so code can tell
IF(CMAKE_BUILD_TYPE EQUAL "Debug")
  ADD_DEFINITIONS(-DLIBOAUTHCPP_DEBUG)
ENDIF()

# Define a few helpful directories
IF(NOT LIBOAUTHCPP_TOP_LEVEL)
  SET(LIBOAUTHCPP_TOP_LEVEL ${CMAKE_CURRENT_SOURCE_DIR}/..)
ENDIF()
GET_FILENAME_COMPONENT(LIBOAUTHCPP_TOP_LEVEL ${LIBOAUTHCPP_TOP_LEVEL} ABSOLUTE)
MESSAGE(STATUS "Base directory: ${LIBOAUTHCPP_TOP_LEVEL}")
SET(LIBOAUTHCPP_INCLUDE ${LIBOAUTHCPP_TOP_LEVEL}/include)
SET(LIBOAUTHCPP_SRC ${LIBOAUTHCPP_TOP_LEVEL}/src)
SET(LIBOAUTHCPP_TEST ${LIBOAUTHCPP_TOP_LEVEL}/tests)
SET(LIBOAUTHCPP_DEMO ${LIBOAUTHCPP_TOP_LEVEL}/demo)

# CMake doesn't seem to allow adding include directories for specific
# projects...
INCLUDE_DIRECTORIES(${LIBOAUTHCPP_INCLUDE})

# Handle configuration options from parent projects
IF(LIBOAUTHCPP_ADDED_DEFINITIONS)
  ADD_DEFINITIONS(${LIBOAUTHCPP_ADDED_DEFINITIONS})
ENDIF()

# The main library
SET(LIBOAUTHCPP_LIB_SOURCES
  ${LIBOAUTHCPP_SRC}/base64.cpp
  ${LIBOAUTHCPP_SRC}/HMAC_SHA1.cpp
  ${LIBOAUTHCPP_SRC}/liboauthcpp.cpp
  ${LIBOAUTHCPP_SRC}/SHA1.cpp
  ${LIBOAUTHCPP_SRC}/urlencode.cpp
  )
ADD_LIBRARY(oauthcpp STATIC ${LIBOAUTHCPP_LIB_SOURCES})
INSTALL(TARGETS oauthcpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib
)
INSTALL(DIRECTORY
  ${LIBOAUTHCPP_INCLUDE}/ DESTINATION include
)

# Allow disabling of tests
IF(NOT DEFINED LIBOAUTHCPP_BUILD_TESTS)
  SET(LIBOAUTHCPP_BUILD_TESTS TRUE CACHE BOOL "Whether to build tests")
ELSE()
  SET(LIBOAUTHCPP_BUILD_TESTS LIBOAUTHCPP_BUILD_TESTS CACHE BOOL "Whether to build tests")
ENDIF()

IF(LIBOAUTHCPP_BUILD_TESTS)

  # A simple demo of authentication -- getting from consumer
  # key/secret all the way to an access token for a user.The user
  # manually performs HTTP steps in a browser.
  SET(LIBOATHCPP_TEST_SOURCES
    ${LIBOAUTHCPP_TEST}/main.cpp
    ${LIBOAUTHCPP_TEST}/testutil.cpp
    )
  ADD_EXECUTABLE(tests ${LIBOATHCPP_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(tests oauthcpp)

  # Target for running the tests
  ADD_CUSTOM_TARGET(test
    tests
    )
ENDIF() #LIBOAUTHCPP_BUILD_TESTS

# Allow disabling of the demos
IF(NOT DEFINED LIBOAUTHCPP_BUILD_DEMOS)
  SET(LIBOAUTHCPP_BUILD_DEMOS TRUE CACHE BOOL "If enabled, builds demos if their dependencies are available.")
ELSE()
  SET(LIBOAUTHCPP_BUILD_DEMOS LIBOAUTHCPP_BUILD_DEMOS CACHE BOOL "If enabled, builds demos if their dependencies are available.")
ENDIF()

IF(LIBOAUTHCPP_BUILD_DEMOS)

  # A simple demo of authentication -- getting from consumer
  # key/secret all the way to an access token for a user.The user
  # manually performs HTTP steps in a browser.
  SET(LIBOATHCPP_SIMPLEAUTHDEMO_SOURCES
    ${LIBOAUTHCPP_DEMO}/simple_auth.cpp
    )
  ADD_EXECUTABLE(simple_auth ${LIBOATHCPP_SIMPLEAUTHDEMO_SOURCES})
  TARGET_LINK_LIBRARIES(simple_auth oauthcpp)

  # A simple demo of performing a request -- after acquiring an access
  # token for a user, sets up the request to access a resource. The
  # user manually performs HTTP steps in a browser.
  SET(LIBOATHCPP_SIMPLEREQUESTDEMO_SOURCES
    ${LIBOAUTHCPP_DEMO}/simple_request.cpp
    )
  ADD_EXECUTABLE(simple_request ${LIBOATHCPP_SIMPLEREQUESTDEMO_SOURCES})
  TARGET_LINK_LIBRARIES(simple_request oauthcpp)

ENDIF() #LIBOAUTHCPP_BUILD_DEMOS
