# Bundle Blender (official portable build)
#
# Blender is licensed under GPL-2.0-or-later. OpenCK is licensed under GPL-3.0.
# Both are copyleft and compatible. Redistribution is permitted provided that
# the source code (or a written offer to obtain it) and Blender's copyright
# notices are included with the distribution.
#
# This script downloads the official Blender portable build for the current
# platform and extracts it to ${CMAKE_BINARY_DIR}/blender/. It is idempotent:
# if the blender executable already exists at the expected path, the
# download step is skipped.

set(OPENCK_BLENDER_VERSION "4.2.5")
set(OPENCK_BLENDER_REVISION "blender-4.2")

if(WIN32)
    set(OPENCK_BLENDER_PLATFORM "windows-x64")
    set(OPENCK_BLENDER_ARCHIVE_EXT "zip")
    set(OPENCK_BLENDER_EXE_PATH "blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/blender.exe")
elseif(APPLE)
    set(OPENCK_BLENDER_PLATFORM "macos-x64")
    set(OPENCK_BLENDER_ARCHIVE_EXT "dmg")
    set(OPENCK_BLENDER_EXE_PATH "Blender.app/Contents/MacOS/Blender")
else()
    set(OPENCK_BLENDER_PLATFORM "linux-x64")
    set(OPENCK_BLENDER_ARCHIVE_EXT "tar.xz")
    set(OPENCK_BLENDER_EXE_PATH "blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/blender")
endif()

set(OPENCK_BLENDER_URL "https://download.blender.org/release/Blender4.2/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}.${OPENCK_BLENDER_ARCHIVE_EXT}")
set(OPENCK_BLENDER_DIR "${CMAKE_BINARY_DIR}/blender")
set(OPENCK_BLENDER_EXE "${OPENCK_BLENDER_DIR}/${OPENCK_BLENDER_EXE_PATH}")

# First, check if Blender already exists in the build output directory.
if(EXISTS "${OPENCK_BLENDER_EXE}")
    message(STATUS "Bundled Blender ${OPENCK_BLENDER_VERSION} found at ${OPENCK_BLENDER_EXE}")
    return()
endif()

# Second, check if a pre-bundled copy exists in the source tree's external/ directory.
set(OPENCK_EXTERNAL_BLENDER_DIR "${CMAKE_SOURCE_DIR}/external/blender")
set(OPENCK_EXTERNAL_BLENDER_EXE "${OPENCK_EXTERNAL_BLENDER_DIR}/${OPENCK_BLENDER_EXE_PATH}")
if(EXISTS "${OPENCK_EXTERNAL_BLENDER_EXE}")
    message(STATUS "Using pre-bundled Blender ${OPENCK_BLENDER_VERSION} from external/blender/")
    file(COPY "${OPENCK_EXTERNAL_BLENDER_DIR}/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/"
        DESTINATION "${OPENCK_BLENDER_DIR}/")
    message(STATUS "Bundled Blender ${OPENCK_BLENDER_VERSION} installed at ${OPENCK_BLENDER_EXE}")
    return()
endif()

message(STATUS "Downloading Blender ${OPENCK_BLENDER_VERSION} from ${OPENCK_BLENDER_URL}")
set(OPENCK_BLENDER_DOWNLOAD "${CMAKE_BINARY_DIR}/_download_blender.zip")
if(NOT EXISTS "${OPENCK_BLENDER_DOWNLOAD}")
    # Use PowerShell Invoke-WebRequest which reliably downloads with proper TLS.
    # CMake's file(DOWNLOAD) has been unreliable with this server.
    execute_process(
        COMMAND powershell -NoProfile -Command "Invoke-WebRequest -Uri '${OPENCK_BLENDER_URL}' -OutFile '${OPENCK_BLENDER_DOWNLOAD}' -UseBasicParsing"
        RESULT_VARIABLE PS_RESULT
    )
    if(NOT PS_RESULT EQUAL 0 OR NOT EXISTS "${OPENCK_BLENDER_DOWNLOAD}")
        message(FATAL_ERROR "Failed to download Blender from ${OPENCK_BLENDER_URL}")
    endif()
endif()

# Verify the download is actually a zip, not an HTML error page
file(READ "${OPENCK_BLENDER_DOWNLOAD}" OPENCK_BLENDER_HEADER OFFSET 0 LIMIT 4 HEX)
string(TOLOWER "${OPENCK_BLENDER_HEADER}" OPENCK_BLENDER_HEADER_LOWER)
string(TOLOWER "504B0304" OPENCK_BLENDER_HEADER_EXPECTED)
if(NOT OPENCK_BLENDER_HEADER_LOWER STREQUAL OPENCK_BLENDER_HEADER_EXPECTED)
    message(FATAL_ERROR "Downloaded file is not a valid zip (header: ${OPENCK_BLENDER_HEADER})")
endif()

message(STATUS "Extracting Blender to ${OPENCK_BLENDER_DIR}")
file(MAKE_DIRECTORY "${OPENCK_BLENDER_DIR}")

if(WIN32)
    file(ARCHIVE_EXTRACT INPUT "${OPENCK_BLENDER_DOWNLOAD}"
        DESTINATION "${OPENCK_BLENDER_DIR}")
elseif(APPLE)
    # macOS .dmg extraction requires hdiutil, not portable — handled by user
    message(FATAL_ERROR "macOS Blender bundling not yet implemented. Download manually from ${OPENCK_BLENDER_URL}")
else()
    file(ARCHIVE_EXTRACT INPUT "${OPENCK_BLENDER_DOWNLOAD}"
        DESTINATION "${OPENCK_BLENDER_DIR}")
endif()

file(REMOVE "${OPENCK_BLENDER_DOWNLOAD}")

if(NOT EXISTS "${OPENCK_BLENDER_EXE}")
    message(FATAL_ERROR "Blender extraction failed: expected ${OPENCK_BLENDER_EXE}")
endif()

message(STATUS "Bundled Blender ${OPENCK_BLENDER_VERSION} installed at ${OPENCK_BLENDER_EXE}")
