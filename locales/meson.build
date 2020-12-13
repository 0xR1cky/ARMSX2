# Check that people use the good file
if 'NOT', 'TOP_CMAKE_WAS_SOURCED'
  # message(['FATAL_ERROR', '
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt'])
endif
if 'GETTEXT_FOUND'
  # file(['GLOB_RECURSE', 'PO_ICO_FILES', cmake_source_dir, '}/locales', 'pcsx2_Iconized.po'])
  # file(['GLOB_RECURSE', 'PO_MAIN_FILES', cmake_source_dir, '/locales', 'pcsx2_Main.po'])
  # Macro to compile po file and install them
  # gettext_create_translations_pcsx2([cmake_source_dir, '/locales/templates/pcsx2_Iconized.pot', 'ALL', po_ico_files])
  # gettext_create_translations_pcsx2([cmake_source_dir, '/locales/templates/pcsx2_Main.pot', 'ALL', po_main_files])
endif
