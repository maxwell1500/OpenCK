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
