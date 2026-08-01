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
