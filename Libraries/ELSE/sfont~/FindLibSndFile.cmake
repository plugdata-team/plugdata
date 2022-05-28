#-------------------------------------------------------------------------------
#
# Copyright 2021 BBC Research and Development
#
# This file is part of Audio Waveform Image Generator.
#
# Author: Chris Needham
#
# Audio Waveform Image Generator is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# Audio Waveform Image Generator is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# Audio Waveform Image Generator.  If not, see <http://www.gnu.org/licenses/>.
#
#-------------------------------------------------------------------------------
#
# Finds libsndfile include file and library. This module sets the following
# variables:
#
#  LIBSNDFILE_FOUND        - Flag if libsndfile was found
#  LIBSNDFILE_INCLUDE_DIRS - libsndfile include directories
#  LIBSNDFILE_LIBRARIES    - libsndfile library paths
#
#-------------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)

find_path(LIBSNDFILE_INCLUDE_DIRS sndfile.h)
find_library(LIBSNDFILE_LIBRARIES sndfile)

if (BUILD_STATIC)
    find_package(LibFLAC REQUIRED)
    find_package(LibVorbis REQUIRED)
    find_package(LibOgg REQUIRED)
    find_package(LibOpus REQUIRED)

    list(
        APPEND
        LIBSNDFILE_LIBRARIES
        ${LIBFLAC_LIBRARIES}
        ${LIBVORBIS_LIBRARIES}
        ${LIBOGG_LIBRARIES}
        ${LIBOPUS_LIBRARIES}
    )

    list(
        APPEND
        LIBSNDFILE_INCLUDE_DIRS
        ${LIBFLAC_INCLUDE_DIRS}
        ${LIBVORBIS_INCLUDE_DIRS}
        ${LIBOGG_INCLUDE_DIRS}
        ${LIBOPUS_INCLUDE_DIRS}
    )
endif()

find_package_handle_standard_args(
    LibSndFile
    DEFAULT_MSG
    LIBSNDFILE_LIBRARIES
    LIBSNDFILE_INCLUDE_DIRS
)

#-------------------------------------------------------------------------------
