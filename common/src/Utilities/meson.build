output = 'Utilities'

utilities_sources = [
  'VirtualMemory.cpp', 
  'AlignedMalloc.cpp', 
  '../../include/Utilities/FixedPointTypes.inl', 
  '../../include/Utilities/EventSource.inl', 
  '../../include/Utilities/SafeArray.inl', 
  'CheckedStaticBox.cpp', 
  'Console.cpp', 
  'EventSource.cpp', 
  'Exceptions.cpp', 
  'FastFormatString.cpp', 
  'IniInterface.cpp', 
  'Linux/LnxHostSys.cpp', 
  'Mutex.cpp', 
  'PathUtils.cpp', 
  'PrecompiledHeader.cpp', 
  'Perf.cpp', 
  'pxCheckBox.cpp', 
  'pxRadioPanel.cpp', 
  'pxStaticText.cpp', 
  'pxStreams.cpp', 
  'pxTranslate.cpp', 
  'pxWindowTextWriter.cpp', 
  'RwMutex.cpp', 
  'StringHelpers.cpp', 
  'ThreadingDialogs.cpp', 
  'ThreadTools.cpp', 
  'wxAppWithHelpers.cpp', 
  'wxGuiTools.cpp', 
  'wxHelpers.cpp'
]

# variable with all headers of this library
utilities_headers = [
  '../../include/Utilities/Assertions.h', 
  '../../include/Utilities/boost_spsc_queue.hpp', 
  '../../include/Utilities/CheckedStaticBox.h', 
  '../../include/Utilities/Console.h', 
  '../../include/Utilities/Dependencies.h', 
  '../../include/Utilities/EventSource.h', 
  '../../include/Utilities/Exceptions.h', 
  '../../include/Utilities/FixedPointTypes.h', 
  '../../include/Utilities/General.h', 
  '../../include/Utilities/MakeUnique.h', 
  '../../include/Utilities/MemcpyFast.h', 
  '../../include/Utilities/MemsetFast.inl', 
  '../../include/Utilities/Path.h', 
  '../../include/Utilities/PageFaultSource.h', 
  '../../include/Utilities/pxCheckBox.h', 
  '../../include/Utilities/pxForwardDefs.h', 
  '../../include/Utilities/pxRadioPanel.h', 
  '../../include/Utilities/pxStaticText.h', 
  '../../include/Utilities/pxStreams.h', 
  '../../include/Utilities/RedtapeWindows.h', 
  '../../include/Utilities/RwMutex.h', 
  '../../include/Utilities/SafeArray.h', 
  '../../include/Utilities/ScopedAlloc.h', 
  '../../include/Utilities/ScopedPtrMT.h', 
  '../../include/Utilities/StringHelpers.h', 
  '../../include/Utilities/Threading.h', 
  '../../include/Utilities/ThreadingDialogs.h', 
  '../../include/Utilities/TraceLog.h', 
  '../../include/Utilities/wxAppWithHelpers.h', 
  '../../include/Utilities/wxBaseTools.h', 
  '../../include/Utilities/wxGuiTools.h', 
  'PrecompiledHeader.h', 
  'ThreadingInternal.h'
]
if build_machine.system() == 'darwin'
  utilities_sources += ['Darwin/DarwinThreads.cpp', 'Darwin/DarwinMisc.cpp', 'Darwin/DarwinSemaphore.cpp']
elif build_machine.system() == 'windows' 
  utilities_sources += ['x86/MemcpyFast.cpp', 'Windows/WinThreads.cpp', 'Windows/WinHostSys.cpp', 'Windows/WinMisc.cpp']
else
  utilities_sources += ['Linux/LnxThreads.cpp', 'Linux/LnxMisc.cpp', 'Semaphore.cpp']
endif
utilitiesfinalsources = [utilitiessources, utilitiesheaders]
utilitiesfinallibs = [libc_libraries, wxwidgets_libraries, 'yaml-cpp']
# add_pcsx2_lib([output, '${UtilitiesFinalSources}', '${UtilitiesFinalLibs}', '${UtilitiesFinalFlags}'])
# add_pcsx2_lib([output, '_NO_TLS', '${UtilitiesFinalSources}', '${UtilitiesFinalLibs}', '${UtilitiesFinalFlags} -DPCSX2_THREAD_LOCAL=0'])
# target_compile_features([output, 'PUBLIC', 'cxx_std_17'])
# target_compile_features([output, '_NO_TLS', 'PUBLIC', 'cxx_std_17'])
if 'COMMAND', 'target_precompile_headers'
  # target_precompile_headers([output, 'PRIVATE', 'PrecompiledHeader.h'])
  # target_precompile_headers([output, '_NO_TLS', 'PRIVATE', 'PrecompiledHeader.h'])
endif
