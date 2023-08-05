cmake_minimum_required(VERSION 3.1)
project(fluidlite-test LANGUAGES C)

#1. To find an installed fluidlite with pkg-config:
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(fluidlite REQUIRED fluidlite)

#2. To find an installed fluidlite with cmake only:
#find_package(fluidlite REQUIRED)

#3. using a subdirectory (for instance, a git submodule):
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite)
include(${CMAKE_CURRENT_SOURCE_DIR}/FluidLite/aliases.cmake)

add_executable(${PROJECT_NAME}
    src/main.c
)

if(UNIX AND NOT APPLE)
    find_library(MATH_LIB m)
endif()

# if you include 'aliases.cmake' or after find_package(fluidlite),
# you get two targets defined, that you may link directly here:
# 1. 'fluidlite::fluidlite-static' is the static library
# 2. 'fluidlite::fluidlite' is the shared dynamic library
target_link_libraries(${PROJECT_NAME} PRIVATE
    fluidlite::fluidlite-static
    ${MATH_LIB}
)
