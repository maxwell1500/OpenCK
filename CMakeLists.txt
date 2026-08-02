cmake_minimum_required(VERSION 3.16)

project(openck
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Hide all symbols by default on Linux/macOS for smaller binaries and faster
# link times. The MSVC build uses __declspec(dllexport) explicitly per-target.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_DEBUG_POSTFIX d)

# ─── Qt6 ──────────────────────────────────────────────────────────────────
# Search common Qt6 install locations on Windows before falling back to
# CMAKE_PREFIX_PATH. This makes "cmake -B build" work out-of-the-box when
# Qt is installed at the standard path (C:\Qt\6.7.3\msvc2019_64) without
# requiring the user to pass -DCMAKE_PREFIX_PATH manually.
if(WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(_qt6_search_paths
        "C:/Qt/6.7.3/msvc2019_64"
        "C:/Qt/6.7.2/msvc2019_64"
        "C:/Qt/6.7.1/msvc2019_64"
        "C:/Qt/6.7.0/msvc2019_64"
    )
    foreach(_path IN LISTS _qt6_search_paths)
        if(EXISTS "${_path}/lib/cmake/Qt6/Qt6Config.cmake")
            list(APPEND CMAKE_PREFIX_PATH "${_path}")
            message(STATUS "Found Qt6 at ${_path}")
            break()
        endif()
    endforeach()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DExtras 3DInput OpenGLWidgets Xml)

# ─── Bundle Blender (GPL-2.0+, redistributable with attribution) ────────
# Downloads the official Blender portable build for the current platform
# and extracts it to ${CMAKE_BINARY_DIR}/blender/. The C++ BlenderLauncher
# checks this path first, falling back to a system installation if missing.
include(cmake/bundle_blender.cmake)

# Bundle NifTools addon (GPL-2.0+) into the bundled Blender's addons directory
include(cmake/bundle_niftools.cmake)

# ─── Library: openck_esm ───────────────────────────────────────────────────
# ESM file reading/writing, record type definitions, game parsers
set(ESM_SOURCES
    libs/files/esm/esmreader.cpp
    libs/files/esm/esmwriter.cpp
    libs/files/esm/glob.cpp
    libs/files/esm/gmst.cpp
    libs/files/esm/lcrt.cpp
    libs/files/esm/records.cpp
    libs/files/esm/tes4.cpp
    libs/files/esm/variant.cpp
    libs/files/esm/skyrimgameparser.cpp
    libs/files/esm/obliviongameparser.cpp
    libs/files/esm/morrowindgameparser.cpp
    libs/files/esm/npcrecord.cpp
    libs/files/esm/weaprecord.cpp
    libs/files/esm/armorrecord.cpp
    libs/files/esm/spellrecord.cpp
    libs/files/esm/magicrecord.cpp
    libs/files/esm/questrecord.cpp
    libs/files/esm/dialrecord.cpp
    libs/files/esm/inforecord.cpp
    libs/files/esm/Packagerecord.cpp
    libs/files/esm/materialrecord.cpp
    libs/files/esm/treerecord.cpp
    libs/files/esm/alchrecord.cpp
    libs/files/esm/ingrrecord.cpp
    libs/files/esm/contrecord.cpp
    libs/files/esm/enchrecord.cpp
    libs/files/esm/bookrecord.cpp
    libs/files/esm/miscrecord.cpp
    libs/files/esm/actirecord.cpp
    libs/files/esm/statrecord.cpp
    libs/files/esm/racerecord.cpp
    libs/files/esm/classrecord.cpp
    libs/files/esm/factrecord.cpp
    libs/files/esm/perkrecord.cpp
    libs/files/esm/cellrecord.cpp
    libs/files/esm/worldspacerecord.cpp
    libs/files/esm/locationrecord.cpp
    libs/files/esm/landrecord.cpp
    libs/files/esm/refrecord.cpp
    libs/files/esm/pluginvalidator.cpp
    libs/files/esm/nifrecord.cpp
    libs/files/esm/sounrecord.cpp
    libs/files/esm/wthrrecord.cpp
    libs/files/esm/ltexrecord.cpp
    libs/files/esm/actorvalueinforecord.cpp
    libs/files/esm/ammorecord.cpp
    libs/files/esm/apparatusrecord.cpp
    libs/files/esm/birthsignrecord.cpp
    libs/files/esm/climaterecord.cpp
    libs/files/esm/clothrecord.cpp
    libs/files/esm/combatstylerecord.cpp
    libs/files/esm/constructibleobjectrecord.cpp
    libs/files/esm/creaturerecord.cpp
    libs/files/esm/doorrecord.cpp
    libs/files/esm/effectshaderrecord.cpp
    libs/files/esm/explosionrecord.cpp
    libs/files/esm/eyesrecord.cpp
    libs/files/esm/florrecord.cpp
    libs/files/esm/formlistrecord.cpp
    libs/files/esm/furnrecord.cpp
    libs/files/esm/grassrecord.cpp
    libs/files/esm/hairrecord.cpp
    libs/files/esm/idleanimationrecord.cpp
    libs/files/esm/idlemarkerrecord.cpp
    libs/files/esm/imagespacerecord.cpp
    libs/files/esm/keymrecord.cpp
    libs/files/esm/keywordrecord.cpp
    libs/files/esm/lighrecord.cpp
    libs/files/esm/loadscreenrecord.cpp
    libs/files/esm/lvlcreaturerecord.cpp
    libs/files/esm/lvlistrecord.cpp
    libs/files/esm/lvspellrecord.cpp
    libs/files/esm/messagerecord.cpp
    libs/files/esm/msttrecord.cpp
    libs/files/esm/navmrecord.cpp
    libs/files/esm/pndrecord.cpp
    libs/files/esm/noterecord.cpp
    libs/files/esm/outfitrecord.cpp
    libs/files/esm/projectilerecord.cpp
    libs/files/esm/regionrecord.cpp
    libs/files/esm/roadrecord.cpp
    libs/files/esm/scriptrecord.cpp
    libs/files/esm/scrollrecord.cpp
    libs/files/esm/shaderparticlerecord.cpp
    libs/files/esm/slgmrecord.cpp
    libs/files/esm/soundmarkerrecord.cpp
    libs/files/esm/staticcollectionrecord.cpp
    libs/files/esm/scenrecord.cpp
    libs/files/esm/texturesetrecord.cpp
    libs/files/esm/waterecord.cpp
    libs/files/esm/conditionrecord.cpp
    libs/files/data/strings.cpp
)

set(ESM_HEADERS
    libs/files/filepaths.hpp
    libs/files/data/strings.hpp
    libs/files/esm/common.hpp
    libs/files/esm/esmfile.hpp
    libs/files/esm/esmreader.hpp
    libs/files/esm/esmwriter.hpp
    libs/files/esm/glob.hpp
    libs/files/esm/gmst.hpp
    libs/files/esm/lcrt.hpp
    libs/files/esm/records.hpp
    libs/files/esm/tes4.hpp
    libs/files/esm/variant.hpp
    libs/files/esm/igameparser.hpp
    libs/files/esm/skyrimgameparser.hpp
    libs/files/esm/obliviongameparser.hpp
    libs/files/esm/morrowindgameparser.hpp
    libs/files/esm/gameparserfactory.hpp
    libs/files/esm/npcrecord.hpp
    libs/files/esm/weaprecord.hpp
    libs/files/esm/armorrecord.hpp
    libs/files/esm/spellrecord.hpp
    libs/files/esm/magicrecord.hpp
    libs/files/esm/questrecord.hpp
    libs/files/esm/dialrecord.hpp
    libs/files/esm/inforecord.hpp
    libs/files/esm/Packagerecord.hpp
    libs/files/esm/materialrecord.hpp
    libs/files/esm/treerecord.hpp
    libs/files/esm/alchrecord.hpp
    libs/files/esm/ingrrecord.hpp
    libs/files/esm/contrecord.hpp
    libs/files/esm/enchrecord.hpp
    libs/files/esm/bookrecord.hpp
    libs/files/esm/miscrecord.hpp
    libs/files/esm/actirecord.hpp
    libs/files/esm/statrecord.hpp
    libs/files/esm/racerecord.hpp
    libs/files/esm/classrecord.hpp
    libs/files/esm/factrecord.hpp
    libs/files/esm/perkrecord.hpp
    libs/files/esm/cellrecord.hpp
    libs/files/esm/worldspacerecord.hpp
    libs/files/esm/locationrecord.hpp
    libs/files/esm/landrecord.hpp
    libs/files/esm/refrecord.hpp
    libs/files/esm/cellreferencedata.hpp
    libs/files/esm/nifrecord.hpp
    libs/files/esm/sharedfiledevice.hpp
    libs/files/esm/pluginvalidator.hpp
    libs/files/esm/sounrecord.hpp
    libs/files/esm/wthrrecord.hpp
    libs/files/esm/ltexrecord.hpp
    libs/files/esm/actorvalueinforecord.hpp
    libs/files/esm/ammorecord.hpp
    libs/files/esm/apparatusrecord.hpp
    libs/files/esm/birthsignrecord.hpp
    libs/files/esm/climaterecord.hpp
    libs/files/esm/clothrecord.hpp
    libs/files/esm/combatstylerecord.hpp
    libs/files/esm/constructibleobjectrecord.hpp
    libs/files/esm/creaturerecord.hpp
    libs/files/esm/doorrecord.hpp
    libs/files/esm/effectshaderrecord.hpp
    libs/files/esm/explosionrecord.hpp
    libs/files/esm/eyesrecord.hpp
    libs/files/esm/florrecord.hpp
    libs/files/esm/formlistrecord.hpp
    libs/files/esm/furnrecord.hpp
    libs/files/esm/grassrecord.hpp
    libs/files/esm/hairrecord.hpp
    libs/files/esm/idleanimationrecord.hpp
    libs/files/esm/idlemarkerrecord.hpp
    libs/files/esm/imagespacerecord.hpp
    libs/files/esm/conditionrecord.hpp
    libs/files/esm/keymrecord.hpp
    libs/files/esm/keywordrecord.hpp
    libs/files/esm/lighrecord.hpp
    libs/files/esm/loadscreenrecord.hpp
    libs/files/esm/lvlcreaturerecord.hpp
    libs/files/esm/lvlistrecord.hpp
    libs/files/esm/lvspellrecord.hpp
    libs/files/esm/messagerecord.hpp
    libs/files/esm/msttrecord.hpp
    libs/files/esm/navmrecord.hpp
    libs/files/esm/pndrecord.hpp
    libs/files/esm/noterecord.hpp
    libs/files/esm/outfitrecord.hpp
    libs/files/esm/projectilerecord.hpp
    libs/files/esm/regionrecord.hpp
    libs/files/esm/roadrecord.hpp
    libs/files/esm/scriptrecord.hpp
    libs/files/esm/scrollrecord.hpp
    libs/files/esm/shaderparticlerecord.hpp
    libs/files/esm/slgmrecord.hpp
    libs/files/esm/soundmarkerrecord.hpp
    libs/files/esm/staticcollectionrecord.hpp
    libs/files/esm/scenrecord.hpp
    libs/files/esm/texturesetrecord.hpp
    libs/files/esm/waterecord.hpp
    libs/components/component.hpp
    libs/components/editorproperty.hpp
    libs/components/formcomponents.hpp
    libs/components/tier1_components.hpp
    libs/components/tier2_components.hpp
    libs/components/tesfullname.hpp
    libs/components/tier3_components.hpp
)

add_library(openck_esm STATIC
    ${ESM_SOURCES}
    ${ESM_HEADERS}
)

target_include_directories(openck_esm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/esm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/data
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/log
)

target_link_libraries(openck_esm PUBLIC
    Qt6::Core
    Qt6::Xml
    Qt6::ZlibPrivate
)

if(WIN32)
    target_compile_definitions(openck_esm PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(openck_esm PRIVATE /FS)
    endif()
endif()

# ─── Vendored: libogg (BSD) ────────────────────────────────────────────────
# Used by libvorbis and oggdecoder. Vendored under external/ogg.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ogg EXCLUDE_FROM_ALL)
set_target_properties(ogg PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)
if(MSVC)
    target_compile_options(ogg PRIVATE /FS)
endif()

# ─── Vendored: libvorbis (BSD) ─────────────────────────────────────────────
# Used by oggencoder for real WAV -> OGG Vorbis conversion. Vendored under
# external/vorbis.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/vorbis EXCLUDE_FROM_ALL)
foreach(_vorbis_target IN ITEMS vorbis vorbisenc vorbisfile)
    if(TARGET ${_vorbis_target})
        set_target_properties(${_vorbis_target} PROPERTIES
            C_VISIBILITY_PRESET hidden
            POSITION_INDEPENDENT_CODE ON
        )
        if(MSVC)
            target_compile_options(${_vorbis_target} PRIVATE /FS)
        endif()
    endif()
endforeach()

# ─── Vendored: Qt-Advanced-Docking-System (LGPL-2.1+) ─────────────────────
# ADS provides the tear-off / tabbed / redockable window system that mirrors
# the real Creation Kit. It is built as a SHARED library so the LGPL
# dynamic-link clause is satisfied (end users may swap qtadvanceddocking-qt6.dll).
# The wrapper at external/ads/CMakeLists.txt pins the version and disables
# the upstream examples before adding the source tree.
add_subdirectory(external/ads)

# ─── Library: openck_files ─────────────────────────────────────────────────
# NIF/BA2 parsing, data export/import, logging
set(FILES_SOURCES
    libs/files/data/dataexporter2.cpp
    libs/files/data/dataimporter2.cpp
    libs/files/data/zipwriter.cpp
    libs/files/data/csvsnippet.cpp
    libs/files/nif/nifparser.cpp
    libs/files/nif/ddsencoder.cpp
    libs/files/nif/ddsdecoder.cpp
    libs/files/nif/tgaencoder.cpp
    libs/files/ba2/ba2archive.cpp
    libs/files/ba2/bsaarchive.cpp
    libs/files/nifanim/nifanimationexporter.cpp
    libs/files/nifanim/nifanimationimporter.cpp
    libs/files/nifanim/nifanimationwriter.cpp
    libs/files/nif/particle/particleeffects.cpp
    libs/files/audio/oggdecoder.cpp
    # stb_vorbis.c is normally pulled in via the header (STB_VORBIS_HEADER_ONLY
    # + #include "stb_vorbis.c"), but we list it here explicitly so the dependency
    # is visible and survives any future refactor that conditionally excludes
    # oggdecoder.cpp from the build.
    external/stb_vorbis.c
    libs/files/audio/wavencoder.cpp
    libs/files/audio/oggencoder.cpp
    libs/files/audio/fuzparser.cpp
    libs/files/audio/xwmadecoder.cpp
)

set(FILES_HEADERS
    libs/files/data/dataexporter.hpp
    libs/files/data/dataimporter.hpp
    libs/files/data/zipwriter.hpp
    libs/files/data/csvsnippet.hpp
    libs/files/log/logger.hpp
    libs/files/nif/nifparser.hpp
    libs/files/nif/ddsencoder.hpp
    libs/files/nif/ddsdecoder.hpp
    libs/files/nif/tgaencoder.hpp
    libs/files/ba2/ba2archive.hpp
    libs/files/ba2/bsaarchive.hpp
    libs/files/nifanim/nifanimation.hpp
    libs/files/nifanim/nifanimationexporter.hpp
    libs/files/nifanim/nifanimationimporter.hpp
    libs/files/nifanim/nifanimationwriter.hpp
    libs/files/nif/particle/particleeffects.hpp
    libs/files/audio/oggdecoder.hpp
    libs/files/audio/wavencoder.hpp
    libs/files/audio/oggencoder.hpp
    libs/files/audio/fuzparser.hpp
    libs/files/audio/xwmadecoder.hpp
)

add_library(openck_files STATIC
    ${FILES_SOURCES}
    ${FILES_HEADERS}
)

target_include_directories(openck_files PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/data
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/log
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/esm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/nif
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/nif/particle
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/ba2
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/nifanim
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/audio
)

target_link_libraries(openck_files PUBLIC
    openck_esm
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::ZlibPrivate
    vorbis
    vorbisenc
    ogg
)

if(WIN32)
    # Windows Media Foundation xWMA decoder (WMA decoder MFT + WMF plumbing).
    target_link_libraries(openck_files PRIVATE mfplat mf mfreadwrite mfuuid wmcodecdspuuid ole32)
    target_compile_definitions(openck_files PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Expose bundled Blender version/platform to C++ code so BlenderLauncher
# can locate the bundled binary at runtime. Only Windows is currently
# supported for bundling; other platforms fall back to system installation.
if(WIN32)
    target_compile_definitions(openck_files PRIVATE
        OPENCK_BUNDLED_BLENDER_VERSION="${OPENCK_BLENDER_VERSION}"
        OPENCK_BUNDLED_BLENDER_PLATFORM="${OPENCK_BLENDER_PLATFORM}"
    )
endif()

if(WIN32)
    target_compile_definitions(openck_files PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(openck_files PRIVATE /FS)
    endif()
endif()

# ─── Library: openck_model ──────────────────────────────────────────────────
# Document, tools, world model (data structures, collections, commands)
set(MODEL_SOURCES
    src/model/tools/brushdefinition.cpp
    src/model/tools/brushalphamask.cpp
    src/model/tools/autopainter.cpp
    src/model/tools/spellchecker.cpp
    src/model/tools/terrainblock.cpp
    src/model/tools/iconrenderer.cpp
    src/model/tools/plugincompactor.cpp
    src/model/tools/mastermanagement.cpp
    src/model/tools/textureconversionrules.cpp
    src/model/tools/referencebatchactions.cpp
    src/model/tools/gitrepository.cpp
    src/model/tools/primitivemeshgenerator.cpp
    src/model/tools/scriptflagfile.cpp
    src/model/tools/papyrusproject.cpp
    src/model/tools/particleprojectilebindings.cpp
    src/model/tools/opallist.cpp
    src/model/tools/particleslodpresets.cpp
    src/model/tools/particlebundle.cpp
    src/model/tools/scenephasemodel.cpp
    src/model/tools/materialruletemplate.cpp
    src/model/tools/materialpropertygraph.cpp
    src/model/tools/meshlodconfig.cpp
    src/model/doc/document.cpp
    src/model/doc/documentmediator.cpp
    src/model/doc/loader.cpp
    src/model/doc/messages.cpp
    src/model/tools/reports.cpp
    src/model/tools/searchalgorithm.cpp
    src/model/tools/validator.cpp
    src/model/tools/undostack.cpp
    src/model/tools/macrocommand.cpp
    src/model/tools/setcolumncommand.cpp
    src/model/tools/addrecordcommand.cpp
    src/model/tools/deleterecordcommand.cpp
    src/model/tools/movekeyframecommand.cpp
    src/model/tools/addkeyframecommand.cpp
    src/model/tools/removekeyframecommand.cpp
    src/model/tools/navmeshgenerator.cpp
    src/model/tools/navmeshtoolkit.cpp
    src/model/tools/nifanimationstate.cpp
    src/model/tools/bashedpatchgenerator.cpp
    src/model/tools/lootwrapper.cpp
    src/model/tools/columnvalidator.cpp
    src/model/tools/assetconverter.cpp
    src/model/tools/assetvalidator.cpp
    src/model/tools/papyrusprevalidator.cpp
    src/model/tools/papyrustypechecker.cpp
    src/model/tools/papyruslanguageserver.cpp
    src/model/tools/papyrusremotedebugger.cpp
    src/model/tools/audiopipelinetools.cpp
    src/model/tools/nodegraph.cpp
    src/model/tools/animationeventvalidator.cpp
    src/model/tools/blendtreemodel.cpp
    src/model/tools/terrainoverlaymask.cpp
    src/model/tools/collisionshapegenerator.cpp
    src/model/tools/fbximporter.cpp
    src/model/tools/planetdefinition.cpp
    src/model/tools/creatureattachpoints.cpp
    src/model/tools/btdfile.cpp
    src/model/tools/bnetclient.cpp
    src/model/tools/objectwindowfilter.cpp
    src/model/tools/assetdependencyscanner.cpp
    src/model/tools/modmanagerdetection.cpp
    src/model/tools/pluginsignature.cpp
    src/model/tools/cloudsave.cpp
    src/model/tools/batchtools.cpp
    src/model/tools/lodgenerator.cpp
    src/model/tools/treelodgenerator.cpp
    src/model/tools/textureatlasgenerator.cpp
    src/model/tools/terrainlodgenerator.cpp
    src/model/tools/blenderlauncher.cpp
    src/model/tools/nifpynifly/nifpyfilewrapper.cpp
    src/model/window/datatable.cpp
    src/model/window/masterslist.cpp
    src/model/window/objectwindow.cpp
    src/model/world/basecollection.cpp
    src/model/world/basecolumn.cpp
    src/model/world/baseidtable.cpp
    src/model/world/ckid.cpp
    src/model/world/columns.cpp
    src/model/world/data.cpp
    src/model/world/idtable.cpp
    src/model/world/metadata.cpp
    src/model/world/record.cpp
    src/model/world/shortcutmanager.cpp
)

set(MODEL_HEADERS
    src/model/doc/document.hpp
    src/model/doc/documentmediator.hpp
    src/model/doc/documentstate.hpp
    src/model/doc/loader.hpp
    src/model/doc/messages.hpp
    src/model/tools/reports.hpp
    src/model/tools/searchalgorithm.hpp
    src/model/tools/validator.hpp
    src/model/tools/npcvalidator.hpp
    src/model/tools/weaponvalidator.hpp
    src/model/tools/questvalidator.hpp
    src/model/tools/command.hpp
    src/model/tools/macrocommand.hpp
    src/model/tools/undostack.hpp
    src/model/tools/setcolumncommand.hpp
    src/model/tools/addrecordcommand.hpp
    src/model/tools/deleterecordcommand.hpp
    src/model/tools/movekeyframecommand.hpp
    src/model/tools/addkeyframecommand.hpp
    src/model/tools/removekeyframecommand.hpp
    src/model/tools/commandundoadapter.hpp
    src/model/tools/recordeditcommand.hpp
    src/model/tools/editrecordcommand.hpp
    src/model/tools/landscapeeditcommand.hpp
    src/model/tools/deleterecordcommandbase.hpp
    src/model/tools/navmeshgenerator.hpp
    src/model/tools/navmeshtoolkit.hpp
    src/model/tools/nifanimationstate.hpp
    src/model/tools/bashedpatchgenerator.hpp
    src/model/tools/lootwrapper.hpp
    src/model/tools/columnvalidator.hpp
    src/model/tools/assetconverter.hpp
    src/model/tools/assetvalidator.hpp
    src/model/tools/papyrusprevalidator.hpp
    src/model/tools/papyrustypechecker.hpp
    src/model/tools/papyruslanguageserver.hpp
    src/model/tools/papyrusremotedebugger.hpp
    src/model/tools/audiopipelinetools.hpp
    src/model/tools/nodegraph.hpp
    src/model/tools/animationeventvalidator.hpp
    src/model/tools/blendtreemodel.hpp
    src/model/tools/terrainoverlaymask.hpp
    src/model/tools/collisionshapegenerator.hpp
    src/model/tools/fbximporter.hpp
    src/model/tools/planetdefinition.hpp
    src/model/tools/creatureattachpoints.hpp
    src/model/tools/btdfile.hpp
    src/model/tools/bnetclient.hpp
    src/model/tools/objectwindowfilter.hpp
    src/model/tools/assetdependencyscanner.hpp
    src/model/tools/modmanagerdetection.hpp
    src/model/tools/pluginsignature.hpp
    src/model/tools/cloudsave.hpp
    src/model/tools/batchtools.hpp
    src/model/tools/brushdefinition.hpp
    src/model/tools/brushalphamask.hpp
    src/model/tools/autopainter.hpp
    src/model/tools/spellchecker.hpp
    src/model/tools/terrainblock.hpp
    src/model/tools/iconrenderer.hpp
    src/model/tools/plugincompactor.hpp
    src/model/tools/mastermanagement.hpp
    src/model/tools/textureconversionrules.hpp
    src/model/tools/referencebatchactions.hpp
    src/model/tools/gitrepository.hpp
    src/model/tools/primitivemeshgenerator.hpp
    src/model/tools/scriptflagfile.hpp
    src/model/tools/papyrusproject.hpp
    src/model/tools/particleprojectilebindings.hpp
    src/model/tools/opallist.hpp
    src/model/tools/particleslodpresets.hpp
    src/model/tools/particlebundle.hpp
    src/model/tools/scenephasemodel.hpp
    src/model/tools/materialruletemplate.hpp
    src/model/tools/materialpropertygraph.hpp
    src/model/tools/meshlodconfig.hpp
    src/model/tools/lodgenerator.hpp
    src/model/tools/treelodgenerator.hpp
    src/model/tools/textureatlasgenerator.hpp
    src/model/tools/terrainlodgenerator.hpp
    src/model/tools/blenderlauncher.hpp
    src/model/tools/nifpynifly/nifpyfilewrapper.hpp
    src/model/window/datatable.hpp
    src/model/window/fileinfo.hpp
    src/model/window/masterslist.hpp
    src/model/window/objectwindow.hpp
    src/model/world/basecollection.hpp
    src/model/world/basecolumn.hpp
    src/model/world/baseidtable.hpp
    src/model/world/ckid.hpp
    src/model/world/collection.hpp
    src/model/world/columnimp.hpp
    src/model/world/columns.hpp
    src/model/world/data.hpp
    src/model/world/shortcuts.hpp
    src/model/world/shortcutmanager.hpp
    src/model/world/idcollection.hpp
    src/model/world/idtable.hpp
    src/model/world/metadata.hpp
    src/model/world/record.hpp
    src/model/world/irecordcollection.hpp
    src/model/world/collection_impl.hpp
)

add_library(openck_model STATIC
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
)

target_include_directories(openck_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/esm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/data
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/log
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/tools/nifpynifly
)

target_link_libraries(openck_model PUBLIC
    openck_files
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_compile_definitions(openck_model PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(openck_model PRIVATE /FS /bigobj)
    endif()
endif()

# ─── Library: openck_view ───────────────────────────────────────────────────
# All UI code: windows, editors, dialogs, delegates, widgets
set(VIEW_SOURCES
    src/view/doc/loader.cpp
    src/view/messageboxhelper.cpp
    src/view/viewmediator.cpp
    src/view/window/datadialog.cpp
    src/view/window/gmstdialog.cpp
    src/view/window/loadorderdialog.cpp
    src/view/window/masterslistdialog.cpp
    src/view/window/conflictdialog.cpp
    src/view/window/conflictresolutiondialog.cpp
    src/view/window/dialoguetreeeditor.cpp
    src/view/window/questgrapheditor.cpp
    src/view/window/aipackageeditor.cpp
    src/view/window/weatherlighteditor.cpp
    src/view/window/navmesheditor.cpp
    src/view/window/navmesheditordialog.cpp
    src/view/window/watereditor.cpp
    src/view/window/celltransitionseditor.cpp
    src/view/window/materialeditor.cpp
    src/view/window/materialeditor.hpp
    src/view/window/papyrusdebugger.cpp
    src/view/window/papyrusdebugger.hpp
    src/view/window/mainwindow.cpp
    src/view/window/inspectorwidget.cpp
    src/view/window/inspectorwidget.hpp
    src/view/window/objectwindowdialog.cpp
    src/view/window/weaponeditor.cpp
    src/view/window/armor_editor.cpp
    src/view/window/spell_editor.cpp
    src/view/window/quest_editor.cpp
    src/view/window/questaliaseditor.cpp
    src/view/window/queststageeditor.cpp
    src/view/window/globvar_editor.cpp
    src/view/window/tree_editor.cpp
    src/view/window/stat_editor.cpp
    src/view/window/actieditor.cpp
    src/view/window/misceditor.cpp
    src/view/window/alch_editor.cpp
    src/view/window/ingr_editor.cpp
    src/view/window/book_editor.cpp
    src/view/window/ench_editor.cpp
    src/view/window/cont_editor.cpp
    src/view/window/race_editor.cpp
    src/view/window/perk_editor.cpp
    src/view/window/magic_editor.cpp
    src/view/window/pack_editor.cpp
    src/view/window/lcrteditor.cpp
    src/view/window/classeditor.cpp
    src/view/window/facteditor.cpp
    src/view/window/cell_editor.cpp
    src/view/window/cellreferenceeditor.cpp
src/view/window/referencebatchdialog.cpp
    src/view/window/worldspace_editor.cpp
    src/view/window/location_editor.cpp
    src/view/window/ref_editor.cpp
    src/view/window/preferencesdialog.cpp
    src/view/window/worldspacedialog.cpp
    src/view/window/cellsdialog.cpp
    src/view/window/cellmapview.cpp
    src/view/window/npceditor.cpp
    src/view/window/npcrecorddatawidget.cpp
    src/view/window/racedatawidget.cpp
    src/view/window/wthrdatawidget.cpp
    src/view/window/sounddatawidget.cpp
    src/view/window/classdatawidget.cpp
    src/view/window/dialdatawidget.cpp
    src/view/window/infodatawidget.cpp
    src/view/window/celldatawidget.cpp
    src/view/window/questdatawidget.cpp
    src/view/window/creatureeditor.cpp
    src/view/window/packdatawidget.cpp
    src/view/window/worldspacedatawidget.cpp
    src/view/window/locationdatawidget.cpp
    src/view/window/warningsdockwidget.cpp
    src/view/window/rawsubrecordwidget.cpp
    src/view/window/searchdialog.cpp
    src/view/world/enumdelegate.cpp
    src/view/world/genericdelegate.cpp
    src/view/world/variantdelegate.cpp
    src/view/window/nifviewportwidget.cpp
    src/view/window/gizmomath.cpp
    src/view/window/mesheditordialog.cpp
    src/view/window/texturepaintwidget.cpp
    src/view/window/textureeditordialog.cpp
    src/view/window/soundeditor.cpp
    src/view/window/waveformwidget.cpp
    src/view/window/scenetimelinewidget.cpp
    src/view/window/nodegraphwidget.cpp
    src/view/window/waveplayer.cpp
    src/view/window/animationeditor.cpp
    src/view/window/timeline.cpp
    src/view/window/particleeffectseffecteditor.cpp
    src/view/window/colorgradientwidget.cpp
    src/view/window/sizecurvewidget.cpp
    src/view/window/particlesystem.cpp
    src/view/window/particlerenderer.cpp
    src/view/widgets/quickfilterbar.cpp
    src/view/widgets/editorpropertygrid.cpp
    src/view/widgets/formcomponentwidget.cpp
    src/view/window/qtformdialog.cpp
    src/view/window/qtformdialogmanager.cpp
    src/view/window/windowlayout.cpp
    src/view/window/scripteditorwidget.cpp
src/view/window/scriptmanagerdialog.cpp
    src/view/window/dialogueeditorwidget.cpp
    src/view/window/formideditorwidget.cpp
    src/view/window/assetbrowserwidget.cpp
    src/view/window/spellwizard.cpp
    src/view/window/pluginmergedialog.cpp
    src/view/window/loadorderoptimizerdialog.cpp
    src/view/window/lootsortdialog.cpp
    src/view/window/externaltoolsdialog.cpp
    src/view/window/thememanager.cpp
    src/view/window/batchexportdialog.cpp
    src/view/window/exportdialog.cpp
    src/view/window/exporttemplatesdialog.cpp
    src/view/window/landscapeeditor.cpp
    src/view/window/objectpalette.cpp
    src/view/window/brushtool.cpp
    src/view/window/papyruscompiler.cpp
    src/view/window/dialeditor.cpp
    src/view/window/infoeditor.cpp
    src/view/window/bashedpatchdialog.cpp
    src/view/window/modmanagerdialog.cpp
    src/view/window/validationreportdialog.cpp
    src/view/window/nifpreviewdialog.cpp
    src/view/window/nifcomparisondialog.cpp
    src/view/window/shortcuteditordialog.cpp
    src/view/window/toolbarcustomizationdialog.cpp
)

set(VIEW_HEADERS
    src/view/doc/loader.hpp
    src/view/messageboxhelper.hpp
    src/view/viewmediator.hpp
    src/view/window/datadialog.hpp
    src/view/window/gmstdialog.hpp
    src/view/window/loadorderdialog.hpp
    src/view/window/mainwindow.hpp
    src/view/window/objectwindowdialog.hpp
    src/view/window/weaponeditor.hpp
    src/view/window/armor_editor.hpp
    src/view/window/spell_editor.hpp
    src/view/window/quest_editor.hpp
    src/view/window/questaliaseditor.hpp
    src/view/window/queststageeditor.hpp
    src/view/window/globvar_editor.hpp
    src/view/window/tree_editor.hpp
    src/view/window/stat_editor.hpp
    src/view/window/actieditor.hpp
    src/view/window/misceditor.hpp
    src/view/window/alch_editor.hpp
    src/view/window/ingr_editor.hpp
    src/view/window/book_editor.hpp
    src/view/window/ench_editor.hpp
    src/view/window/cont_editor.hpp
    src/view/window/race_editor.hpp
    src/view/window/perk_editor.hpp
    src/view/window/magic_editor.hpp
    src/view/window/pack_editor.hpp
    src/view/window/lcrteditor.hpp
    src/view/window/classeditor.hpp
    src/view/window/facteditor.hpp
    src/view/window/preferencesdialog.hpp
    src/view/window/worldspacedialog.hpp
    src/view/window/cellsdialog.hpp
    src/view/window/cellmapview.hpp
    src/view/window/npceditor.hpp
    src/view/window/npcrecorddatawidget.hpp
    src/view/window/racedatawidget.hpp
    src/view/window/wthrdatawidget.hpp
    src/view/window/sounddatawidget.hpp
    src/view/window/classdatawidget.hpp
    src/view/window/dialdatawidget.hpp
    src/view/window/infodatawidget.hpp
    src/view/window/celldatawidget.hpp
    src/view/window/questdatawidget.hpp
    src/view/window/creatureeditor.hpp
    src/view/window/packdatawidget.hpp
    src/view/window/worldspacedatawidget.hpp
    src/view/window/locationdatawidget.hpp
    src/view/window/warningsdockwidget.hpp
    src/view/window/rawsubrecordwidget.hpp
    src/view/window/searchdialog.hpp
    src/view/world/delegatefactory.hpp
    src/view/world/enumdelegate.hpp
    src/view/world/genericdelegate.hpp
    src/view/world/variantdelegate.hpp
    src/view/window/nifviewportwidget.hpp
    src/view/window/gizmomath.hpp
    src/view/window/mesheditordialog.hpp
    src/view/window/texturepaintwidget.hpp
    src/view/window/textureeditordialog.hpp
    src/view/window/soundeditor.hpp
    src/view/window/waveformwidget.hpp
    src/view/window/scenetimelinewidget.hpp
    src/view/window/nodegraphwidget.hpp
    src/view/window/animationeditor.hpp
    src/view/window/timeline.hpp
    src/view/window/particleeffectseffecteditor.hpp
    src/view/window/colorgradientwidget.hpp
    src/view/window/sizecurvewidget.hpp
    src/view/window/particlesystem.hpp
    src/view/window/particlerenderer.hpp
    src/view/widgets/quickfilterbar.hpp
    src/view/widgets/editorpropertygrid.hpp
    src/view/widgets/formcomponentwidget.hpp
    src/view/window/qtformdialog.hpp
    src/view/window/qtformdialogmanager.hpp
    src/view/window/windowlayout.hpp
    src/view/window/scripteditorwidget.hpp
    src/view/window/dialogueeditorwidget.hpp
    src/view/window/formideditorwidget.hpp
    src/view/window/assetbrowserwidget.hpp
    src/view/window/spellwizard.hpp
    src/view/window/pluginmergedialog.hpp
    src/view/window/conflictresolutiondialog.hpp
    src/view/window/dialoguetreeeditor.hpp
    src/view/window/questgrapheditor.hpp
    src/view/window/aipackageeditor.hpp
    src/view/window/weatherlighteditor.hpp
    src/view/window/navmesheditor.hpp
    src/view/window/navmesheditordialog.hpp
    src/view/window/watereditor.hpp
    src/view/window/celltransitionseditor.hpp
    src/view/window/loadorderoptimizerdialog.hpp
    src/view/window/lootsortdialog.hpp
    src/view/window/externaltoolsdialog.hpp
    src/view/window/thememanager.hpp
    src/view/window/batchexportdialog.hpp
    src/view/window/exportdialog.hpp
    src/view/window/exporttemplatesdialog.hpp
    src/view/window/landscapeeditor.hpp
    src/view/window/objectpalette.hpp
    src/view/window/brushtool.hpp
    src/view/window/papyruscompiler.hpp
    src/view/window/dialeditor.hpp
    src/view/window/infoeditor.hpp
    src/view/window/bashedpatchdialog.hpp
    src/view/window/cellreferenceeditor.hpp
    src/view/window/modmanagerdialog.hpp
    src/view/window/validationreportdialog.hpp
    src/view/window/nifpreviewdialog.hpp
    src/view/window/nifcomparisondialog.hpp
    src/view/window/shortcuteditordialog.hpp
    src/view/window/toolbarcustomizationdialog.hpp
)

add_library(openck_view STATIC
    ${VIEW_SOURCES}
    ${VIEW_HEADERS}
)

target_include_directories(openck_view PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/esm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/data
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/log
)

target_link_libraries(openck_view PUBLIC
    openck_model
    Qt6::Widgets
    Qt6::3DCore
    Qt6::3DRender
    Qt6::3DExtras
    Qt6::3DInput
    Qt6::OpenGLWidgets
    ads::qtadvanceddocking-qt6
)

if(WIN32)
    target_compile_definitions(openck_view PRIVATE NOMINMAX)
    if(MSVC)
        target_compile_options(openck_view PRIVATE /FS)
    endif()
endif()

# ─── Main executable ────────────────────────────────────────────────────────
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    if(MSVC)
        add_compile_options(/W4 /permissive- /EHa /FS)
        add_compile_options(/wd4100)
    endif()
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

add_executable(openck
    src/main.cpp
    src/cli.cpp
    src/cli.hpp
    src/editor.cpp
    src/editor.hpp
    ui/datadialog.ui
    ui/gmstdialog.ui
    ui/loaderdialog.ui
    ui/mainwindow.ui
    ${RESOURCES_DIR}/resources.qrc
)

if(WIN32)
    target_sources(openck PRIVATE
        data/manifest/openck.rc
    )
    target_compile_definitions(openck PRIVATE NOMINMAX)
endif()

target_include_directories(openck PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/esm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/data
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/files/log
)

target_link_libraries(openck
    openck_view
    Qt6::Widgets
    Qt6::3DCore
    Qt6::3DRender
    Qt6::3DExtras
    Qt6::3DInput
    Qt6::OpenGLWidgets
    Qt6::Xml
    OpenGL32
    ads::qtadvanceddocking-qt6
)

# Copy settings file to build directory (multi-config safe)
add_custom_target(copy_ini ALL)
add_custom_command(
    TARGET copy_ini POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/data/editor.ini
        $<TARGET_FILE_DIR:openck>/
)

# Copy scripts to build output directory
add_custom_target(copy_scripts ALL)
add_custom_command(
    TARGET copy_scripts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:openck>/scripts/blender
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/nif_operations.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/nif_batch.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/nif_import.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/nif_export.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/preview_generator.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/blender_operations.py $<TARGET_FILE_DIR:openck>/scripts/blender/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/blender/icon_generator.py $<TARGET_FILE_DIR:openck>/scripts/blender/
)

# Deploy Qt6 runtime DLLs to output directory
if(WIN32)
    get_target_property(_qt6_loc Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT6_INSTALL_PREFIX "${_qt6_loc}/../../.." ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_INSTALL_PREFIX}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        # windeployqt doesn't detect Qt6 3D modules; copy them manually.
        # Match both Debug (*d.dll) and Release (*.dll) variants of all Qt6 3D
        # modules so the binary works in both configurations.
        get_target_property(_qt3dcore_loc Qt6::3DCore IMPORTED_LOCATION)
        get_filename_component(_qt3d_dll_dir "${_qt3dcore_loc}" DIRECTORY)
        file(GLOB _qt3d_dlls
            "${_qt3d_dll_dir}/Qt63D*d.dll"
            "${_qt3d_dll_dir}/Qt63D*.dll"
        )

        add_custom_command(TARGET openck POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-opengl-sw --no-system-d3d-compiler --dir "$<TARGET_FILE_DIR:openck>" "$<TARGET_FILE:openck>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_qt3d_dlls} "$<TARGET_FILE_DIR:openck>"
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:openck>"
            COMMENT "Deploying Qt6 runtime DLLs..."
        )
    endif()
endif()

# Install targets for `cmake --install`
install(TARGETS openck
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
install(FILES data/editor.ini
    DESTINATION bin
)
install(DIRECTORY scripts/blender
    DESTINATION bin/scripts
)

# Install Qt6 runtime DLLs alongside the executable (Windows only).
# windeployqt copies these to the build dir but `cmake --install` doesn't
# pick them up automatically. Without this, a user installing the build
# output would get a non-functional binary.
if(WIN32)
    install(DIRECTORY "$<TARGET_FILE_DIR:openck>/"
        DESTINATION bin
        FILES_MATCHING PATTERN "Qt6*.dll"
    )
endif()

# Bundle Blender (GPL-2.0+, redistributable with license attribution).
# The bundled Blender is at ${CMAKE_BINARY_DIR}/blender/blender-*-windows-x64/.
# We install it to bin/blender/ and include Blender's license tree
# (license/license.md + SPDX per-library licenses) to satisfy the GPL
# attribution requirement.
if(EXISTS "${CMAKE_BINARY_DIR}/blender")
    install(DIRECTORY "${CMAKE_BINARY_DIR}/blender"
        DESTINATION bin
        PATTERN "*.tmp" EXCLUDE
    )
    install(DIRECTORY "${CMAKE_BINARY_DIR}/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/license"
        DESTINATION bin/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}
        OPTIONAL
    )
    install(FILES "${CMAKE_BINARY_DIR}/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/copyright.txt"
        DESTINATION bin/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}
        RENAME "BLENDER_COPYRIGHT.txt"
        OPTIONAL
    )
    install(FILES "${CMAKE_BINARY_DIR}/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/${OPENCK_NIFTOOLS_ADDON_PATH}/LICENSE*"
        DESTINATION bin/blender/blender-${OPENCK_BLENDER_VERSION}-${OPENCK_BLENDER_PLATFORM}/${OPENCK_NIFTOOLS_ADDON_PATH}
        RENAME "NIFTOOLS_LICENSE.txt"
        OPTIONAL
    )
endif()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Localization: lupdate/lrelease .ts -> .qm build steps (optional)
include(cmake/i18n.cmake)

# ─── CPack: distributable installer packages ────────────────────────────────
# `cmake --build build --config Release && cpack -C Release`
# produces NSIS .exe (Windows) or TGZ/STGZ (Unix).
if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
    set(CPACK_PACKAGE_NAME "OpenCK")
    set(CPACK_PACKAGE_VENDOR "OpenCK Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source Creation Kit alternative")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_PACKAGE_VERSION_MAJOR "1")
    set(CPACK_PACKAGE_VERSION_MINOR "0")
    set(CPACK_PACKAGE_VERSION_PATCH "0")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenCK")
    set(CPACK_NSIS_DISPLAY_NAME "OpenCK ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_HELP_LINK "https://github.com/maxwell1500/OpenCK")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/maxwell1500/OpenCK")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_SOURCE_IGNORE_FILES
        "/build/;/out/;/\.git/;/translations/OpenCK_.*\\.ts$"
        ".*\\.log$;.*\\.obj$;.*\\.pdb$;.*\\.tlog$"
    )
    include(CPack)
endif()
