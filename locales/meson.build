gettext = find_program('gettext', required : true)
if cleaner.found()
  run_command(cleaner, '-i', src)
  run_command(cleaner, '-i', test_src)
endif
if (GETTEXT_FOUND)
    include(Translation)

    file (GLOB_RECURSE PO_ICO_FILES ${CMAKE_SOURCE_DIR}}/locales  pcsx2_Iconized.po)
    file (GLOB_RECURSE PO_MAIN_FILES ${CMAKE_SOURCE_DIR}/locales  pcsx2_Main.po)

    # Macro to compile po file and install them
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${CMAKE_SOURCE_DIR}/locales/templates/pcsx2_Iconized.pot ALL ${PO_ICO_FILES})
    GETTEXT_CREATE_TRANSLATIONS_PCSX2(${CMAKE_SOURCE_DIR}/locales/templates/pcsx2_Main.pot     ALL ${PO_MAIN_FILES})

endif (GETTEXT_FOUND)
