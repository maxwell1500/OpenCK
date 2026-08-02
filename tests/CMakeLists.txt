cmake_minimum_required(VERSION 3.16)

project(openck_tests LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Test Xml)

# Stubs for view-layer symbols referenced by openck_model
add_library(test_stubs STATIC test_stubs.cpp)
target_link_libraries(test_stubs PRIVATE Qt6::Core)
target_include_directories(test_stubs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/files
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/files/esm
)

# Common include dirs needed by all test targets
set(TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/files
    ${CMAKE_SOURCE_DIR}/libs/files/esm
    ${CMAKE_SOURCE_DIR}/libs/files/log
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/model/world
)

# Helper to register a test with the correct working directory on Windows
function(openck_add_test name)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
    )
endfunction()

# ESM I/O round-trip tests
add_executable(test_pluginio test_pluginio.cpp)
target_link_libraries(test_pluginio PRIVATE openck_esm Qt6::Test)
target_include_directories(test_pluginio PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_pluginio)

# Integration tests (collection + JSON)
add_executable(test_integration test_integration.cpp)
target_link_libraries(test_integration PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_integration PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_integration)

# Conflict detection tests
add_executable(test_conflict test_conflict.cpp)
target_link_libraries(test_conflict PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_conflict PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_conflict)

# Data model tests
add_executable(test_datamodel test_datamodel.cpp)
target_link_libraries(test_datamodel PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_datamodel PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_datamodel)

# Search algorithm tests
add_executable(test_searchalgorithm test_searchalgorithm.cpp)
target_link_libraries(test_searchalgorithm PRIVATE Qt6::Test)
target_include_directories(test_searchalgorithm PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_searchalgorithm)

# Column validator tests
add_executable(test_columnvalidator test_columnvalidator.cpp)
target_link_libraries(test_columnvalidator PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_columnvalidator PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_columnvalidator)

# EditRecordCommand tests
add_executable(test_editrecordcommand test_editrecordcommand.cpp)
target_link_libraries(test_editrecordcommand PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_editrecordcommand PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_editrecordcommand)

# Undo stack tests
add_executable(test_undostack test_undostack.cpp)
target_link_libraries(test_undostack PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_undostack PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_undostack)

# Export/Import round-trip tests
add_executable(test_exportimport test_exportimport.cpp)
target_link_libraries(test_exportimport PRIVATE openck_model test_stubs Qt6::Test Qt6::Xml)
target_include_directories(test_exportimport PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_exportimport)

# LOD generator tests
add_executable(test_lodgenerator test_lodgenerator.cpp)
target_link_libraries(test_lodgenerator PRIVATE openck_model test_stubs Qt6::Test Qt6::Gui)
target_include_directories(test_lodgenerator PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_lodgenerator)

# Config paths tests
add_executable(test_configpaths test_configpaths.cpp)
target_link_libraries(test_configpaths PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_configpaths PRIVATE ${TEST_INCLUDE_DIRS})
# Note: test_configpaths passes when run directly but fails under CTest
# due to DLL path resolution in the CTest environment. Keep for manual use.

# Shortcut manager tests
add_executable(test_shortcutmanager test_shortcutmanager.cpp)
target_link_libraries(test_shortcutmanager PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_shortcutmanager PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_shortcutmanager)

# Theme manager tests
add_executable(test_thememanager test_thememanager.cpp)
target_link_libraries(test_thememanager PRIVATE openck_view Qt6::Test)
target_include_directories(test_thememanager PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_thememanager)

# Gizmo picking/dragging math tests
add_executable(test_gizmomath test_gizmomath.cpp)
target_link_libraries(test_gizmomath PRIVATE openck_view Qt6::Test)
target_include_directories(test_gizmomath PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_gizmomath)

# NifPyFileWrapper tests
add_executable(test_nifpyfilewrapper test_nifpyfilewrapper.cpp)
target_link_libraries(test_nifpyfilewrapper PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_nifpyfilewrapper PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_nifpyfilewrapper)

# BlenderLauncher tests
add_executable(test_blenderlauncher test_blenderlauncher.cpp)
target_link_libraries(test_blenderlauncher PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_blenderlauncher PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_blenderlauncher)

# ObjectWindow modelPath tests
add_executable(test_objectwindow test_objectwindow.cpp)
target_link_libraries(test_objectwindow PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_objectwindow PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_objectwindow)

# NIF integration tests
add_executable(test_nifintegration test_nifintegration.cpp)
target_link_libraries(test_nifintegration PRIVATE openck_model openck_files test_stubs Qt6::Test)
target_include_directories(test_nifintegration PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_nifintegration)

# Compressed record (zlib) tests
add_executable(test_compressedrecord test_compressedrecord.cpp)
target_link_libraries(test_compressedrecord PRIVATE openck_esm Qt6::Test Qt6::ZlibPrivate)
target_include_directories(test_compressedrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_compressedrecord)

# Starfield ESM direct loading test (requires the user's Starfield install)
add_executable(test_starfieldesm test_starfieldesm.cpp)
target_link_libraries(test_starfieldesm PRIVATE openck_esm Qt6::Test)
target_include_directories(test_starfieldesm PRIVATE ${TEST_INCLUDE_DIRS})
# Not registered with CTest by default — requires a real Starfield.esm.

# Throwaway record-structure dumper used to validate the PNDT/CCT binary
# encoders against the real Starfield.esm. Run: test_dumpesm PNDT
add_executable(test_dumpesm test_dumpesm.cpp)
target_link_libraries(test_dumpesm PRIVATE openck_esm Qt6::Core)
target_include_directories(test_dumpesm PRIVATE ${TEST_INCLUDE_DIRS})
# Not registered with CTest by default — requires a real Starfield.esm.

# PndRecord binary encoder round-trip against the real Starfield.esm.
add_executable(test_pndrecord test_pndrecord.cpp)
target_link_libraries(test_pndrecord PRIVATE openck_esm Qt6::Test)
target_include_directories(test_pndrecord PRIVATE ${TEST_INCLUDE_DIRS})
# Not registered with CTest by default — requires a real Starfield.esm.

# Header parsing robustness tests
add_executable(test_headerparsing test_headerparsing.cpp)
target_link_libraries(test_headerparsing PRIVATE openck_esm Qt6::Test)
target_include_directories(test_headerparsing PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_headerparsing)

# FormComponents container tests
add_executable(test_component test_component.cpp)
target_link_libraries(test_component PRIVATE openck_esm Qt6::Test)
target_include_directories(test_component PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_component)

# EditorProperty leaf-type tests (value/setValue roundtrips)
add_executable(test_editorproperty test_editorproperty.cpp)
target_link_libraries(test_editorproperty PRIVATE openck_esm Qt6::Test)
target_include_directories(test_editorproperty PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_editorproperty)

# Component lifecycle tests (clone / copyFrom / isEqualTo / mergeWith)
add_executable(test_editor_lifecycle test_editor_lifecycle.cpp)
target_link_libraries(test_editor_lifecycle PRIVATE openck_esm Qt6::Test)
target_include_directories(test_editor_lifecycle PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_editor_lifecycle)

# QtFormDialog + QtFormDialogManager tests (dedup, closeAll, factory).
# We compile the dialog/widget sources directly here rather than link
# openck_view, because openck_view currently pulls in the (pre-existing
# broken) landscapeeditor.cpp which would otherwise block this test.
add_executable(test_qtformdialog test_qtformdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/view/window/qtformdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/view/window/qtformdialogmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/view/widgets/editorpropertygrid.cpp
    ${CMAKE_SOURCE_DIR}/src/view/widgets/formcomponentwidget.cpp
)
target_link_libraries(test_qtformdialog PRIVATE openck_esm Qt6::Test Qt6::Widgets)
target_include_directories(test_qtformdialog PRIVATE
    ${TEST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/view
    ${CMAKE_SOURCE_DIR}/src/view/widgets
    ${CMAKE_SOURCE_DIR}/src/view/window
)
openck_add_test(test_qtformdialog)

# Tier 3 component tests
add_executable(test_tier3_components test_tier3_components.cpp)
target_link_libraries(test_tier3_components PRIVATE openck_esm Qt6::Test)
target_include_directories(test_tier3_components PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_tier3_components)

# Specialized editor widget tests
add_executable(test_editor_widgets test_editor_widgets.cpp)
target_link_libraries(test_editor_widgets PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_editor_widgets PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_editor_widgets)

# CREA editor widget + QtFormDialogManager factory tests
add_executable(test_creatureeditor test_creatureeditor.cpp)
target_link_libraries(test_creatureeditor PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_creatureeditor PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_creatureeditor)

# CellMapView math tests
add_executable(test_cellviewcanvas test_cellviewcanvas.cpp)
target_link_libraries(test_cellviewcanvas PRIVATE openck_view Qt6::Test)
target_include_directories(test_cellviewcanvas PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_cellviewcanvas)

# Warnings dock widget tests
add_executable(test_warningsdock test_warningsdock.cpp)
target_link_libraries(test_warningsdock PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_warningsdock PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_warningsdock)

# PACK/WRLD/LCTN specialized editor widgets
add_executable(test_specialized_widgets test_specialized_widgets.cpp)
target_link_libraries(test_specialized_widgets PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_specialized_widgets PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_specialized_widgets)

# NavmRecord load/save round-trip
add_executable(test_navmrecord test_navmrecord.cpp)
target_link_libraries(test_navmrecord PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_navmrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_navmrecord)
# NavMesh toolkit algorithms: adjacency, analyze/check, weld, connect, T-junction removal
add_executable(test_navmeshtoolkit test_navmeshtoolkit.cpp)
target_link_libraries(test_navmeshtoolkit PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_navmeshtoolkit PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_navmeshtoolkit)

# ScenRecord load/save round-trip
add_executable(test_scenrecord test_scenrecord.cpp)
target_link_libraries(test_scenrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_scenrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_scenrecord)

# CellRecord load/save round-trip (XCLW water plane support)
add_executable(test_cellrecord test_cellrecord.cpp)
target_link_libraries(test_cellrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_cellrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_cellrecord)

# Landscape brush definitions (.lbr JSON loading + operations)
add_executable(test_brushdefinition test_brushdefinition.cpp)
target_link_libraries(test_brushdefinition PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_brushdefinition PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_brushdefinition)

# Brush alpha texture masks (.dds stencils for landscape brushes)
add_executable(test_brushalphamask test_brushalphamask.cpp)
target_link_libraries(test_brushalphamask PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_brushalphamask PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_brushalphamask)

# Icon generation renderer (context sizes, light rig, Blender invocation)
add_executable(test_iconrenderer test_iconrenderer.cpp)
target_link_libraries(test_iconrenderer PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_iconrenderer PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_iconrenderer)

# Real plugin compaction: form-ID renumbering + reference re-pointing
add_executable(test_plugincompactor test_plugincompactor.cpp)
target_link_libraries(test_plugincompactor PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_plugincompactor PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_plugincompactor)

# xtexconv texture conversion rules (Textures_Settings*.json)
add_executable(test_textureconversionrules test_textureconversionrules.cpp)
target_link_libraries(test_textureconversionrules PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_textureconversionrules PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_textureconversionrules)

# Master file management ([MMS] ini section + free-ID allocation)
add_executable(test_mastermanagement test_mastermanagement.cpp)
target_link_libraries(test_mastermanagement PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_mastermanagement PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_mastermanagement)

# Landscape autopaint (height/slope band texture assignment)
add_executable(test_autopainter test_autopainter.cpp)
target_link_libraries(test_autopainter PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_autopainter PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_autopainter)

# EFSH effect shader record typed DATA subrecord round-trip
add_executable(test_efshrecord test_efshrecord.cpp)
target_link_libraries(test_efshrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_efshrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_efshrecord)

# IMGS record typed DATA parsing (48 modifier floats + 6 zone colors)
add_executable(test_imgsrecord test_imgsrecord.cpp)
target_link_libraries(test_imgsrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_imgsrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_imgsrecord)

# Dictionary-based spell checker (script/dialogue editors)
add_executable(test_spellchecker test_spellchecker.cpp)
target_link_libraries(test_spellchecker PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_spellchecker PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_spellchecker)

# BSMaterial shader property graph (TextureSet slots, blenders, SSS)
add_executable(test_materialpropertygraph test_materialpropertygraph.cpp)
target_link_libraries(test_materialpropertygraph PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_materialpropertygraph PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_materialpropertygraph)

# LCTN location record linked-reference round-trip (XNAM/LNAM)
add_executable(test_locationrecord test_locationrecord.cpp)
target_link_libraries(test_locationrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_locationrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_locationrecord)

# Simplygon-style mesh LOD pipeline config (GenerationConfig.json)
add_executable(test_meshlodconfig test_meshlodconfig.cpp)
target_link_libraries(test_meshlodconfig PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_meshlodconfig PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_meshlodconfig)

# Terrain-block cutting (heightmap region cut/insert with edge blending)
add_executable(test_terrainblock test_terrainblock.cpp)
target_link_libraries(test_terrainblock PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_terrainblock PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_terrainblock)

# Papyrus LSP client transport (Content-Length framing + JSON-RPC)
add_executable(test_papyruslanguageserver test_papyruslanguageserver.cpp)
target_link_libraries(test_papyruslanguageserver PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_papyruslanguageserver PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_papyruslanguageserver)

# Papyrus remote debugger wire protocol (port 20548, command/response)
add_executable(test_papyrusremotedebugger test_papyrusremotedebugger.cpp)
target_link_libraries(test_papyrusremotedebugger PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_papyrusremotedebugger PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_papyrusremotedebugger)

# External audio pipeline tools (LipGenerator/FaceFX/Wwise/RoboVoicer)
add_executable(test_audiopipelinetools test_audiopipelinetools.cpp)
target_link_libraries(test_audiopipelinetools PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_audiopipelinetools PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_audiopipelinetools)

# Scene timeline phase-model editing semantics
add_executable(test_scenetimeline test_scenetimeline.cpp)
target_link_libraries(test_scenetimeline PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_scenetimeline PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_scenetimeline)

# Behavior/animation node-graph model (nodes, edges, hit-testing)
add_executable(test_nodegraph test_nodegraph.cpp)
target_link_libraries(test_nodegraph PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_nodegraph PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_nodegraph)

# Animation event validation (event-name -> animation-type sets)
add_executable(test_animationeventvalidator test_animationeventvalidator.cpp)
target_link_libraries(test_animationeventvalidator PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_animationeventvalidator PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_animationeventvalidator)

# Blend-tree weights + variable assignment nodes
add_executable(test_blendtreemodel test_blendtreemodel.cpp)
target_link_libraries(test_blendtreemodel PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_blendtreemodel PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_blendtreemodel)

# Terrain overlay masks (.tif/.dds biome/settlement stencils)
add_executable(test_terrainoverlaymask test_terrainoverlaymask.cpp)
target_link_libraries(test_terrainoverlaymask PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_terrainoverlaymask PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_terrainoverlaymask)

# Physics collision shape generation (AABB, convex hull, decimation)
add_executable(test_collisionshapegenerator test_collisionshapegenerator.cpp)
target_link_libraries(test_collisionshapegenerator PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_collisionshapegenerator PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_collisionshapegenerator)

# FBX -> NIF import settings + Blender pipeline (AssetWatcher pattern)
add_executable(test_fbximporter test_fbximporter.cpp)
target_link_libraries(test_fbximporter PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_fbximporter PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_fbximporter)

# Starfield planet (PNDT) definition model (biomes, traits, resources)
add_executable(test_planetdefinition test_planetdefinition.cpp)
target_link_libraries(test_planetdefinition PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_planetdefinition PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_planetdefinition)

# Starfield creature (CCT) attach-point model
add_executable(test_creatureattachpoints test_creatureattachpoints.cpp)
target_link_libraries(test_creatureattachpoints PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_creatureattachpoints PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_creatureattachpoints)

# BTD land-texture files (per-quad texture indices)
add_executable(test_btdfile test_btdfile.cpp)
target_link_libraries(test_btdfile PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_btdfile PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_btdfile)

# Bethesda.net plugin upload client (login/logout/upload)
add_executable(test_bnetclient test_bnetclient.cpp)
target_link_libraries(test_bnetclient PRIVATE openck_model test_stubs Qt6::Test Qt6::Network)
target_include_directories(test_bnetclient PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_bnetclient)

# Object Window .filter file support (data-driven keyword filtering)
add_executable(test_objectwindowfilter test_objectwindowfilter.cpp)
target_link_libraries(test_objectwindowfilter PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_objectwindowfilter PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_objectwindowfilter)

# CTDA conditions (PACK/SCEN/DIAL/INFO/QUST)
add_executable(test_conditionrecord test_conditionrecord.cpp)
target_link_libraries(test_conditionrecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_conditionrecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_conditionrecord)

# PACK record with typed CTDA conditions
add_executable(test_packagerecord test_packagerecord.cpp)
target_link_libraries(test_packagerecord PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_packagerecord PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_packagerecord)

# Minimal ZIP writer (crash diagnostics bundles)
add_executable(test_zipwriter test_zipwriter.cpp)
target_link_libraries(test_zipwriter PRIVATE openck_files test_stubs Qt6::Test)
target_include_directories(test_zipwriter PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_zipwriter)

# Validation report export (xEdit-style TSV)
add_executable(test_reportexport test_reportexport.cpp)
target_link_libraries(test_reportexport PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_reportexport PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_reportexport)

# Reference batch action utilities (move/snap/scale/flag/rotation)
add_executable(test_referencebatchactions test_referencebatchactions.cpp)
target_link_libraries(test_referencebatchactions PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_referencebatchactions PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_referencebatchactions)

# CSV Snippets template engine (.Import= directives + field accessors)
add_executable(test_csvsnippet test_csvsnippet.cpp)
target_link_libraries(test_csvsnippet PRIVATE openck_files test_stubs Qt6::Test)
target_include_directories(test_csvsnippet PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_csvsnippet)

# Git repository wrapper (Check In/Out VCS)
add_executable(test_gitrepository test_gitrepository.cpp)
target_link_libraries(test_gitrepository PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_gitrepository PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_gitrepository)

# Preview primitive mesh generator (cube/cylinder/plane/sphere)
add_executable(test_primitivemeshgenerator test_primitivemeshgenerator.cpp)
target_link_libraries(test_primitivemeshgenerator PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_primitivemeshgenerator PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_primitivemeshgenerator)

# Papyrus script flag file parser (.flg)
add_executable(test_scriptflagfile test_scriptflagfile.cpp)
target_link_libraries(test_scriptflagfile PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_scriptflagfile PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_scriptflagfile)

# Papyrus project file parser (.ppj)
add_executable(test_papyrusproject test_papyrusproject.cpp)
target_link_libraries(test_papyrusproject PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_papyrusproject PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_papyrusproject)

# Particle projectile variable bindings (.pofx-style JSON)
add_executable(test_particleprojectilebindings test_particleprojectilebindings.cpp)
target_link_libraries(test_particleprojectilebindings PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_particleprojectilebindings PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_particleprojectilebindings)

# OPAL procedural placement list parser (.opl)
add_executable(test_opallist test_opallist.cpp)
target_link_libraries(test_opallist PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_opallist PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_opallist)

# DDS texture decoder (BC1/2/3/4/5 + uncompressed)
add_executable(test_ddsdecoder test_ddsdecoder.cpp)
target_link_libraries(test_ddsdecoder PRIVATE openck_files test_stubs Qt6::Test)
target_include_directories(test_ddsdecoder PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_ddsdecoder)

# Papyrus compiler diagnostic parsing (structured grammar)
add_executable(test_papyruscompiler test_papyruscompiler.cpp)
target_link_libraries(test_papyruscompiler PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_papyruscompiler PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_papyruscompiler)

# Particle LOD presets (ParticlesLODPresets.json)
add_executable(test_particleslodpresets test_particleslodpresets.cpp)
target_link_libraries(test_particleslodpresets PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_particleslodpresets PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_particleslodpresets)

# Particle effect bundle (.pofx)
add_executable(test_particlebundle test_particlebundle.cpp)
target_link_libraries(test_particlebundle PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_particlebundle PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_particlebundle)

# Headless CLI (export/info commands)
add_executable(test_cli test_cli.cpp)
target_link_libraries(test_cli PRIVATE openck_esm test_stubs Qt6::Test)
target_include_directories(test_cli PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_cli)

# Landscape texture-layer slope influence + material opacity
add_executable(test_texturelayer test_texturelayer.cpp)
target_link_libraries(test_texturelayer PRIVATE openck_view openck_model test_stubs Qt6::Test Qt6::Widgets Qt6::OpenGL OpenGL32)
target_include_directories(test_texturelayer PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_texturelayer)

# .fuz voice-over container parser (LIPF + audio chunks)
add_executable(test_fuzparser test_fuzparser.cpp)
target_link_libraries(test_fuzparser PRIVATE openck_files test_stubs Qt6::Test)
target_include_directories(test_fuzparser PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_fuzparser)

# Scene (SCEN) phase timeline model
add_executable(test_scenephasemodel test_scenephasemodel.cpp)
target_link_libraries(test_scenephasemodel PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_scenephasemodel PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_scenephasemodel)

# Papyrus type checker (property/member access, array length)
add_executable(test_papyrustypechecker test_papyrustypechecker.cpp)
target_link_libraries(test_papyrustypechecker PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_papyrustypechecker PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_papyrustypechecker)

# BSMaterial layered material rule templates
add_executable(test_materialruletemplate test_materialruletemplate.cpp)
target_link_libraries(test_materialruletemplate PRIVATE openck_model test_stubs Qt6::Test)
target_include_directories(test_materialruletemplate PRIVATE ${TEST_INCLUDE_DIRS})
openck_add_test(test_materialruletemplate)

# Helper that writes a minimal test plugin for manual CLI verification
add_executable(make_testplugin make_testplugin.cpp)
target_link_libraries(make_testplugin PRIVATE openck_esm test_stubs Qt6::Core)
target_include_directories(make_testplugin PRIVATE ${TEST_INCLUDE_DIRS})

# Convenience target that builds every test executable.
add_custom_target(all_tests DEPENDS
    test_pluginio test_integration test_conflict test_datamodel
    test_searchalgorithm test_columnvalidator test_editrecordcommand
    test_undostack test_exportimport test_lodgenerator test_configpaths
    test_shortcutmanager test_thememanager test_gizmomath
    test_nifpyfilewrapper test_blenderlauncher test_objectwindow
    test_nifintegration test_compressedrecord test_headerparsing
    test_component test_editorproperty test_editor_lifecycle
    test_qtformdialog test_tier3_components test_editor_widgets
    test_creatureeditor test_cellviewcanvas test_warningsdock
    test_specialized_widgets test_navmrecord test_scenrecord
    test_starfieldesm test_cellrecord test_brushdefinition test_zipwriter
    test_bitwise test_framing)
