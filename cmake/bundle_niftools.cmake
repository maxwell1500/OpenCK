# Bundle NifTools Blender addon (BSD-3-Clause, redistributable with attribution)
#
# Downloads the official NifTools Blender addon (io_scene_niftools) from the
# niftools/blender_niftools_addon GitHub repository and installs it into the
# bundled Blender's scripts/addons/ directory. The C++ BlenderLauncher enables
# it on first use and the Python scripts in scripts/blender/ detect and use it.
#
# This is idempotent: if the addon is already installed in the bundled Blender,
# the download step is skipped.

set(OPENCK_NIFTOOLS_REPO "https://github.com/niftools/blender_niftools_addon")
set(OPENCK_NIFTOOLS_ADDON_PATH "io_scene_niftools")
set(OPENCK_NIFTOOLS_RELEASE_TAG "v0.1.1")

# Locate the bundled Blender's addons directory.
# Blender 4.x stores user addons in <blender>/4.x/scripts/addons/
set(OPENCK_BLENDER_DIR "${CMAKE_BINARY_DIR}/blender")
set(OPENCK_BLENDER_ADDONS_DIR "${OPENCK_BLENDER_DIR}/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/4.2/scripts/addons")
set(OPENCK_NIFTOOLS_TARGET_DIR "${OPENCK_BLENDER_ADDONS_DIR}/${OPENCK_NIFTOOLS_ADDON_PATH}")

if(EXISTS "${OPENCK_NIFTOOLS_TARGET_DIR}/__init__.py")
    message(STATUS "Bundled NifTools addon found at ${OPENCK_NIFTOOLS_TARGET_DIR}")
    return()
endif()

if(NOT EXISTS "${OPENCK_BLENDER_DIR}")
    message(STATUS "Skipping NifTools addon download: bundled Blender not found at ${OPENCK_BLENDER_DIR}")
    return()
endif()

# Use PowerShell Invoke-WebRequest which reliably downloads with proper TLS.
# CMake's file(DOWNLOAD) has been unreliable with GitHub redirects.
set(OPENCK_NIFTOOLS_ZIP "${CMAKE_BINARY_DIR}/niftools_addon.zip")
set(OPENCK_NIFTOOLS_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_niftools_download")

message(STATUS "Downloading NifTools addon from ${OPENCK_NIFTOOLS_REPO}")

# Step 1: Download the GitHub release/latest page (returns HTML with redirect to actual release)
execute_process(
    COMMAND powershell -NoProfile -Command "Invoke-WebRequest -Uri '${OPENCK_NIFTOOLS_REPO}/releases/latest' -OutFile '${OPENCK_NIFTOOLS_ZIP}' -UseBasicParsing"
    RESULT_VARIABLE PS_RESULT
)
if(NOT PS_RESULT EQUAL 0 OR NOT EXISTS "${OPENCK_NIFTOOLS_ZIP}")
    message(WARNING "Failed to download NifTools addon release page")
    return()
endif()

# Step 2: Parse the HTML to find the actual asset zip filename
file(READ "${OPENCK_NIFTOOLS_ZIP}" NIFTOOLS_HTML)
string(REGEX MATCH "blender_niftools_addon-v[^\"]+\\.zip" NIFTOOLS_ASSET_NAME "${NIFTOOLS_HTML}")
file(REMOVE "${OPENCK_NIFTOOLS_ZIP}")
if(NOT NIFTOOLS_ASSET_NAME)
    message(WARNING "Could not determine NifTools addon asset name from release page")
    return()
endif()

# Step 3: Download the actual zip asset
execute_process(
    COMMAND powershell -NoProfile -Command "Invoke-WebRequest -Uri '${OPENCK_NIFTOOLS_REPO}/releases/download/${OPENCK_NIFTOOLS_RELEASE_TAG}/${NIFTOOLS_ASSET_NAME}' -OutFile '${OPENCK_NIFTOOLS_ZIP}' -UseBasicParsing"
    RESULT_VARIABLE PS_RESULT
)
if(NOT PS_RESULT EQUAL 0 OR NOT EXISTS "${OPENCK_NIFTOOLS_ZIP}")
    message(WARNING "Failed to download NifTools addon asset")
    return()
endif()

# Verify the download is actually a zip, not an HTML error page
file(READ "${OPENCK_NIFTOOLS_ZIP}" NIFTOOLS_HEADER OFFSET 0 LIMIT 4 HEX)
string(TOLOWER "${NIFTOOLS_HEADER}" NIFTOOLS_HEADER_LOWER)
string(TOLOWER "504B0304" ZIP_MAGIC)
if(NOT NIFTOOLS_HEADER_LOWER STREQUAL ZIP_MAGIC)
    message(WARNING "Downloaded NifTools addon is not a valid zip (header: ${NIFTOOLS_HEADER})")
    file(REMOVE "${OPENCK_NIFTOOLS_ZIP}")
    return()
endif()

# Step 4: Extract the zip and copy io_scene_niftools/ to the bundled Blender's addons directory
file(REMOVE_RECURSE "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}")
file(MAKE_DIRECTORY "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}")
file(ARCHIVE_EXTRACT INPUT "${OPENCK_NIFTOOLS_ZIP}"
    DESTINATION "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}")

# The release zip contains io_scene_niftools/ at the root, but the directory
# structure may vary. Check both the root and any nested location.
file(GLOB NIFTOOLS_EXTRACTED_DIRS LIST_DIRECTORIES true
    "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}/${OPENCK_NIFTOOLS_ADDON_PATH}"
    "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}/*/${OPENCK_NIFTOOLS_ADDON_PATH}"
)

list(LENGTH NIFTOOLS_EXTRACTED_DIRS DIR_COUNT)
if(DIR_COUNT EQUAL 0)
    message(WARNING "Could not find ${OPENCK_NIFTOOLS_ADDON_PATH} in downloaded archive")
    file(REMOVE "${OPENCK_NIFTOOLS_ZIP}")
    file(REMOVE_RECURSE "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}")
    return()
endif()

list(GET NIFTOOLS_EXTRACTED_DIRS 0 NIFTOOLS_SOURCE_DIR)
file(MAKE_DIRECTORY "${OPENCK_BLENDER_ADDONS_DIR}")
file(REMOVE_RECURSE "${OPENCK_NIFTOOLS_TARGET_DIR}")
file(COPY "${NIFTOOLS_SOURCE_DIR}" DESTINATION "${OPENCK_BLENDER_ADDONS_DIR}")

# Clean up
file(REMOVE_RECURSE "${OPENCK_NIFTOOLS_DOWNLOAD_DIR}")
file(REMOVE "${OPENCK_NIFTOOLS_ZIP}")

if(EXISTS "${OPENCK_NIFTOOLS_TARGET_DIR}/__init__.py")
    message(STATUS "Bundled NifTools addon installed at ${OPENCK_NIFTOOLS_TARGET_DIR}")
else()
    message(WARNING "NifTools addon installation failed: __init__.py not found in ${OPENCK_NIFTOOLS_TARGET_DIR}")
endif()
