cmake_minimum_required(VERSION 3.16)

# OpenCK's wrapper for the vendored Qt-Advanced-Docking-System (ADS)
# library. Mirrors what the real Starfield Creation Kit uses.
#
# ADS is LGPL-2.1-or-later with the Qt GPL exception. We build it as
# a SHARED library on Windows / Linux / macOS so we don't have to
# LGPL-relink our entire application; the resulting ads.dll/.so/.dylib
# lives next to openck.exe and end users may replace it with their
# own build, which is exactly what the LGPL dynamic-link clause
# intends.
#
# We pin a specific commit (see the comment below) by checking the
# commit hash; ADS_VERSION is set explicitly so the upstream
# CMakeLists doesn't try to query git.

set(ADS_PREFERRED_VERSION "4.4")
set(ADS_PATCH_VERSION    "0")

# Path to the upstream source tree (sibling of this file).
set(ADS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Qt-Advanced-Docking-System")

# The upstream expects a git tag for versioning when no ADS_VERSION
# is set. We always set ADS_VERSION explicitly to avoid the git
# dependency at configure time.
set(ADS_VERSION "${ADS_PREFERRED_VERSION}.${ADS_PATCH_VERSION}")

# We never want the examples / demo sub-projects in our build. The
# upstream CMakeLists respects BUILD_EXAMPLES, so we set it before
# adding the subdirectory.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# The default is BUILD_STATIC=OFF which gives us a shared library,
# which is what we want for LGPL compliance.
set(BUILD_STATIC  OFF CACHE BOOL "" FORCE)

add_subdirectory(${ADS_SRC} ${CMAKE_BINARY_DIR}/external/ads/build)
