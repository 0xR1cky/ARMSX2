output = 'x86emitter'
# variable with all sources of this library
x86emittersources = [
  'bmi.cpp', 
  'cpudetect.cpp', 
  'fpu.cpp', 
  'groups.cpp', 
  'jmp.cpp', 
  'legacy.cpp', 
  'legacy_sse.cpp', 
  'movs.cpp', 
  'PrecompiledHeader.cpp', 
  'simd.cpp', 
  'x86emitter.cpp'
]
# variable with all headers of this library
x86emitterheaders = [
  '../../include/x86emitter/implement/dwshift.h', 
  '../../include/x86emitter/implement/group1.h', 
  '../../include/x86emitter/implement/group2.h', 
  '../../include/x86emitter/implement/group3.h', 
  '../../include/x86emitter/implement/helpers.h', 
  '../../include/x86emitter/implement/incdec.h', 
  '../../include/x86emitter/implement/jmpcall.h', 
  '../../include/x86emitter/implement/movs.h', 
  '../../include/x86emitter/implement/simd_arithmetic.h', 
  '../../include/x86emitter/implement/simd_comparisons.h', 
  '../../include/x86emitter/implement/simd_helpers.h', 
  '../../include/x86emitter/implement/simd_moremovs.h', 
  '../../include/x86emitter/implement/simd_shufflepack.h', 
  '../../include/x86emitter/implement/simd_templated_helpers', 
  '../../include/x86emitter/implement/test.h', 
  '../../include/x86emitter/implement/xchg.h', 
  '../../include/x86emitter/instructions.h', 
  '../../include/x86emitter/internal.h', 
  '../../include/x86emitter/legacy_instructions.h', 
  '../../include/x86emitter/legacy_internal.h', 
  '../../include/x86emitter/legacy_types.h', 
  '../../include/x86emitter/tools.h', 
  '../../include/x86emitter/x86emitter.h', 
  '../../include/x86emitter/x86types.h', 
  'PrecompiledHeader.h', 
  'cpudetect_internal.h'
]
if build_machine.system() == 'windows'
  x86emittersources += ['WinCpuDetect.cpp']
else
  x86emittersources += ['LnxCpuDetect.cpp']
endif
x86emitterfinalsources = [x86emittersources, x86emitterheaders]
x86emitterfinallibs = wxwidgets_libraries
# add_pcsx2_lib([output, '${x86emitterFinalSources}', '${x86emitterFinalLibs}', '${x86emitterFinalFlags}'])
# target_compile_features([output, 'PUBLIC', 'cxx_std_17'])
if 'COMMAND', 'target_precompile_headers'
  # target_precompile_headers([output, 'PRIVATE', 'PrecompiledHeader.h'])
endif
