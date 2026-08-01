# Localization build step.
#
# Generates OpenCK_<lang>.ts translation sources from all C++/UI sources via
# lupdate, compiles them to .qm via lrelease, and copies the .qm files next
# to the executable so QTranslator can load them at runtime.
#
# Targets:
#   openck_i18n_update   - refresh .ts files from sources (lupdate only)
#   openck_i18n_<lang>   - build one language's .qm file (lupdate + lrelease)
#
# Languages are declared in OPENCK_TRANSLATION_LANGS. If the list is empty
# only the update target is exposed so translators can seed the .ts files.

find_package(Qt6 QUIET COMPONENTS LinguistTools)

if(NOT Qt6LinguistTools_FOUND OR NOT TARGET Qt6::lupdate OR NOT TARGET Qt6::lrelease)
    message(STATUS "i18n: Qt6LinguistTools targets unavailable; translation build disabled")
    return()
endif()

set(OPENCK_TRANSLATION_LANGS "")

file(GLOB_RECURSE OPENCK_I18N_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)

if(OPENCK_TRANSLATION_LANGS)
    foreach(_lang IN LISTS OPENCK_TRANSLATION_LANGS)
        set(_ts "${CMAKE_CURRENT_SOURCE_DIR}/translations/OpenCK_${_lang}.ts")
        set(_qm "${CMAKE_CURRENT_SOURCE_DIR}/translations/OpenCK_${_lang}.qm")

        add_custom_command(
            OUTPUT "${_qm}"
            COMMAND $<TARGET_FILE:Qt6::lupdate>
                ${OPENCK_I18N_SOURCES}
                -silent
                -no-obsolete
                -source-language en_US
                -target-language ${_lang}
                -ts "${_ts}"
            COMMAND $<TARGET_FILE:Qt6::lrelease>
                "${_ts}"
                -qm "${_qm}"
            DEPENDS ${OPENCK_I18N_SOURCES} "${_ts}"
            COMMENT "Generating ${_lang} translation (.qm)"
        )

        add_custom_target(openck_i18n_${_lang} DEPENDS "${_qm}")

        # Copy .qm next to the built binary.
        if(WIN32)
            add_custom_command(TARGET openck_i18n_${_lang} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qm}" "$<TARGET_FILE_DIR:openck>"
                COMMENT "Deploying OpenCK_${_lang}.qm"
            )
        endif()
    endforeach()

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/translations/OpenCK_*.qm
        DESTINATION share/openck/translations
    )
else()
    add_custom_target(openck_i18n_update
        COMMAND $<TARGET_FILE:Qt6::lupdate>
            ${OPENCK_I18N_SOURCES}
            -source-language en_US
            -target-language en_US
            -ts ${CMAKE_CURRENT_SOURCE_DIR}/translations/OpenCK_en.ts
        COMMENT "Updating translation sources (translations/OpenCK_en.ts)"
    )
    message(STATUS "i18n: no languages enabled; 'openck_i18n_update' target available")
endif()
