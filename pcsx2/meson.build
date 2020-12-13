if 'openSUSE'
  # openSUSE don't install wx in a standard library system
  # path. Let's bypass the dynamic linker and hardcode the path.
  cmake_install_rpath_use_link_path = 'ON'
endif
# Check that people use the good file
if 'NOT', 'TOP_CMAKE_WAS_SOURCED'
  # message(['FATAL_ERROR', '
    You did not 'cmake' the good CMakeLists.txt file. Use the one in the top dir.
    It is advice to delete all wrongly generated cmake stuff => CMakeFiles & CMakeCache.txt'])
endif
commonflags = ['-fno-strict-aliasing', '-Wstrict-aliasing', '-Wno-parentheses', '-Wno-missing-braces', '-DWX_PRECOMP']
# Largely turning off because LegacyPluginAPI_Common in Plugins.h spams really badly in gcc 8 due to the memzero initialization.
# Should probably be fixed properly, but for now this should work.
if 'GCC_VERSION', 'VERSION_EQUAL', '8.0', 'OR', 'GCC_VERSION', 'VERSION_GREATER', '8.0'
  commonflags = [commonflags, '-Wno-class-memaccess']
endif
if '${PGO}', 'STREQUAL', 'generate'
  pcsx2finalflags = '-fprofile-generate'
elif '${PGO}', 'STREQUAL', 'use'
  pcsx2finalflags = '-fprofile-use'
else
  pcsx2finalflags = ''
endif
pcsx2finalflags = [pcsx2finalflags, commonflags]

if 'PORTAUDIO_FOUND'
  pcsx2finalflags = [pcsx2finalflags, '-DSPU2X_PORTAUDIO']
endif
if 'PULSEAUDIO_FOUND'
  pcsx2finalflags = [pcsx2finalflags, '-DSPU2X_PULSEAUDIO']
endif
if 'XDG_STD'
  pcsx2finalflags = [pcsx2finalflags, '-DXDG_STD']
endif
if 'SDL_FOUND', 'OR', 'SDL2_FOUND'
  pcsx2finalflags = [pcsx2finalflags, '-DSDL_BUILD']
endif
output = 'PCSX2'

# Main pcsx2 source
pcsx2sources = [
  'Cache.cpp', 
  'COP0.cpp', 
  'COP2.cpp', 
  'Counters.cpp', 
  'GameDatabase.cpp', 
  'Dump.cpp', 
  'Elfheader.cpp', 
  'FW.cpp', 
  'FiFo.cpp', 
  'FPU.cpp', 
  'Gif.cpp', 
  'Gif_Logger.cpp', 
  'Gif_Unit.cpp', 
  'GS.cpp', 
  'GSState.cpp', 
  'Hw.cpp', 
  'HwRead.cpp', 
  'HwWrite.cpp', 
  'Interpreter.cpp', 
  'IopBios.cpp', 
  'IopCounters.cpp', 
  'IopDma.cpp', 
  'IopGte.cpp', 
  'IopHw.cpp', 
  'IopIrq.cpp', 
  'IopMem.cpp', 
  'IopSio2.cpp', 
  'IPC.cpp', 
  'Mdec.cpp', 
  'Memory.cpp', 
  'MMI.cpp', 
  'MTGS.cpp', 
  'MTVU.cpp', 
  'MultipartFileReader.cpp', 
  'OutputIsoFile.cpp', 
  'Patch.cpp', 
  'Patch_Memory.cpp', 
  'Pcsx2Config.cpp', 
  'PluginManager.cpp', 
  'PrecompiledHeader.cpp', 
  'R3000A.cpp', 
  'R3000AInterpreter.cpp', 
  'R3000AOpcodeTables.cpp', 
  'R5900.cpp', 
  'R5900OpcodeImpl.cpp', 
  'R5900OpcodeTables.cpp', 
  'SaveState.cpp', 
  'ShiftJisToUnicode.cpp', 
  'Sif.cpp', 
  'Sif0.cpp', 
  'Sif1.cpp', 
  'sif2.cpp', 
  'Sio.cpp', 
  'SourceLog.cpp', 
  'SPR.cpp', 
  'System.cpp', 
  'Vif0_Dma.cpp', 
  'Vif1_Dma.cpp', 
  'Vif1_MFIFO.cpp', 
  'Vif.cpp', 
  'Vif_Codes.cpp', 
  'Vif_Transfer.cpp', 
  'Vif_Unpack.cpp', 
  'vtlb.cpp', 
  'VU0.cpp', 
  'VUmicro.cpp', 
  'VU0micro.cpp', 
  'VU0microInterp.cpp', 
  'VU1micro.cpp', 
  'VU1microInterp.cpp', 
  'VUflags.cpp', 
  'VUmicroMem.cpp', 
  'VUops.cpp'
]
# Main pcsx2 header
pcsx2headers = [
  'AsyncFileReader.h', 
  'Cache.h', 
  'cheatscpp.h', 
  'Common.h', 
  'Config.h', 
  'COP0.h', 
  'Counters.h', 
  'Dmac.h', 
  'Dump.h', 
  'GameDatabase.h', 
  'Elfheader.h', 
  'FW.h', 
  'Gif.h', 
  'Gif_Unit.h', 
  'GS.h', 
  'Hardware.h', 
  'Hw.h', 
  'IopBios.h', 
  'IopCommon.h', 
  'IopCounters.h', 
  'IopDma.h', 
  'IopGte.h', 
  'IopHw.h', 
  'IopMem.h', 
  'IopSio2.h', 
  'IPC.h', 
  'Mdec.h', 
  'MTVU.h', 
  'Memory.h', 
  'MemoryTypes.h', 
  'Patch.h', 
  'PathDefs.h', 
  'Plugins.h', 
  'PrecompiledHeader.h', 
  'R3000A.h', 
  'R5900Exceptions.h', 
  'R5900.h', 
  'R5900OpcodeTables.h', 
  'SaveState.h', 
  'Sifcmd.h', 
  'Sif.h', 
  'Sio.h', 
  'sio_internal.h', 
  'SPR.h', 
  'SysForwardDefs.h', 
  'System.h', 
  'Vif_Dma.h', 
  'Vif.h', 
  'Vif_Unpack.h', 
  'vtlb.h', 
  'VUflags.h', 
  'VUmicro.h', 
  'VUops.h'
]
# CDVD sources
pcsx2cdvdsources = [
  'CDVD/BlockdumpFileReader.cpp', 
  'CDVD/CdRom.cpp', 
  'CDVD/CDVDaccess.cpp', 
  'CDVD/CDVD.cpp', 
  'CDVD/CDVDdiscReader.cpp', 
  'CDVD/CDVDisoReader.cpp', 
  'CDVD/CDVDdiscThread.cpp', 
  'CDVD/InputIsoFile.cpp', 
  'CDVD/OutputIsoFile.cpp', 
  'CDVD/ChunksCache.cpp', 
  'CDVD/CompressedFileReader.cpp', 
  'CDVD/CsoFileReader.cpp', 
  'CDVD/GzippedFileReader.cpp', 
  'CDVD/IsoFS/IsoFile.cpp', 
  'CDVD/IsoFS/IsoFSCDVD.cpp', 
  'CDVD/IsoFS/IsoFS.cpp'
]
# CDVD headers
pcsx2cdvdheaders = [
  'CDVD/CdRom.h', 
  'CDVD/CDVDaccess.h', 
  'CDVD/CDVD.h', 
  'CDVD/CDVD_internal.h', 
  'CDVD/CDVDdiscReader.h', 
  'CDVD/CDVDisoReader.h', 
  'CDVD/ChunksCache.h', 
  'CDVD/CompressedFileReader.h', 
  'CDVD/CompressedFileReaderUtils.h', 
  'CDVD/CsoFileReader.h', 
  'CDVD/GzippedFileReader.h', 
  'CDVD/IsoFileFormats.h', 
  'CDVD/IsoFS/IsoDirectory.h', 
  'CDVD/IsoFS/IsoFileDescriptor.h', 
  'CDVD/IsoFS/IsoFile.h', 
  'CDVD/IsoFS/IsoFSCDVD.h', 
  'CDVD/IsoFS/IsoFS.h', 
  'CDVD/IsoFS/SectorSource.h', 
  'CDVD/zlib_indexed.h'
]
# SPU2 sources
pcsx2spu2sources = [
  'SPU2/ADSR.cpp', 
  'SPU2/Debug.cpp', 
  'SPU2/DplIIdecoder.cpp', 
  'SPU2/Dma.cpp', 
  'SPU2/Lowpass.cpp', 
  'SPU2/Mixer.cpp', 
  'SPU2/spu2.cpp', 
  'SPU2/ReadInput.cpp', 
  'SPU2/RegLog.cpp', 
  'SPU2/RegTable.cpp', 
  'SPU2/Reverb.cpp', 
  'SPU2/SndOut.cpp', 
  'SPU2/SndOut_SDL.cpp', 
  'SPU2/spu2freeze.cpp', 
  'SPU2/spu2replay.cpp', 
  'SPU2/spu2sys.cpp', 
  'SPU2/Timestretcher.cpp', 
  'SPU2/Wavedump_wav.cpp', 
  'SPU2/WavFile.cpp', 
  'SPU2/Linux/Alsa.cpp', 
  'SPU2/Linux/CfgHelpers.cpp', 
  'SPU2/Linux/Config.cpp', 
  'SPU2/Linux/ConfigDebug.cpp', 
  'SPU2/Linux/ConfigSoundTouch.cpp', 
  'SPU2/Linux/Dialogs.cpp', 
  'SPU2/wx/wxConfig.cpp'
]


if 'PORTAUDIO_FOUND'
  pcsx2spu2sources = [pcsx2spu2sources, 'SPU2/SndOut_Portaudio.cpp']
endif
# SPU2 headers
pcsx2spu2headers = ['SPU2/Config.h', 'SPU2/Debug.h', 'SPU2/defs.h', 'SPU2/Dma.h', 'SPU2/Global.h', 'SPU2/Lowpass.h', 'SPU2/Mixer.h', 'SPU2/spu2.h', 'SPU2/regs.h', 'SPU2/SndOut.h', 'SPU2/spdif.h', 'SPU2/spu2replay.h', 'SPU2/WavFile.h', 'SPU2/Linux/Alsa.h', 'SPU2/Linux/Config.h', 'SPU2/Linux/Dialogs.h', 'SPU2/wx/wxConfig.h']
# DEV9 UI sources
# compile_gresources(['pcsx2DEV9UISources', 'pcsx2DEV9UIXML', 'TYPE', 'EMBED_C', 'RESOURCES', 'DEV9/Linux/dev9.ui', 'PREFIX', '/net/pcsx2/dev9', 'COMPRESS_ALL', 'STRIPBLANKS_ALL'])
# DEV9 UI headers
# compile_gresources(['pcsx2DEV9UIHeaders', 'pcsx2DEV9UIXML', 'TYPE', 'EMBED_H', 'RESOURCES', 'DEV9/Linux/dev9.ui', 'PREFIX', '/net/pcsx2/dev9', 'COMPRESS_ALL', 'STRIPBLANKS_ALL'])
# DEV9 sources
pcsx2dev9sources = ['DEV9/smap.cpp', 'DEV9/DEV9.cpp', 'DEV9/flash.cpp', 'DEV9/pcap_io.cpp', 'DEV9/Linux/Config.cpp', 'DEV9/Linux/Linux.cpp', 'DEV9/net.cpp', pcsx2dev9uisources]
# DEV9 headers
pcsx2dev9headers = ['DEV9/DEV9.h', 'DEV9/ata.h', 'DEV9/net.h', 'DEV9/pcap_io.h', 'DEV9/smap.h', pcsx2dev9uiheaders]
# USBNull
pcsx2usbnullsources = 'USB/USBNull.cpp'
pcsx2usbnullheaders = 'USB/USB.h'
# USB sources 
pcsx2usbsources = [
'USB/USB.cpp', 
'USB/deviceproxy.cpp', 
'USB/configuration.cpp', 
'USB/device_init.cpp', 
'USB/qemu-usb/glib.cpp', 
'USB/qemu-usb/vl.cpp', 
'USB/qemu-usb/iov.cpp', 
'USB/qemu-usb/desc.cpp', 
'USB/qemu-usb/core.cpp', 
'USB/qemu-usb/bus.cpp', 
'USB/qemu-usb/usb-ohci.cpp', 
'USB/qemu-usb/hid.cpp', 
'USB/qemu-usb/input-keymap-qcode-to-qnum.cpp', 
'USB/usb-msd/usb-msd.cpp', 
'USB/usb-pad/usb-pad.cpp', 
'USB/usb-pad/usb-pad-ff.cpp', 
'USB/usb-pad/lg/lg_ff.cpp', 
'USB/usb-pad/usb-seamic.cpp', 
'USB/usb-mic/usb-mic-singstar.cpp', 
'USB/usb-mic/usb-mic-logitech.cpp', 
'USB/usb-mic/usb-headset.cpp', 
'USB/usb-eyetoy/jpgd/jpgd.cpp', 
'USB/usb-eyetoy/jo_mpeg.cpp', 
'USB/usb-eyetoy/usb-eyetoy-webcam.cpp', 
'USB/usb-hid/usb-hid.cpp', 
'USB/shared/shared_usb.cpp', 
'USB/shared/inifile_usb.cpp', 
'USB/shared/ringbuffer.cpp', 
'USB/icon_buzz_24.cpp', 
'USB/usb-msd/usb-msd-gtk.cpp', 
'USB/usb-pad/joydev/joydev.cpp', 
'USB/usb-pad/joydev/joydev-gtk.cpp', 
'USB/usb-pad/evdev/shared-gtk.cpp', 
'USB/usb-pad/evdev/evdev-ff.cpp', 
'USB/usb-pad/evdev/evdev.cpp', 
'USB/usb-pad/evdev/evdev-gtk.cpp', 
'USB/usb-hid/evdev/evdev-gtk.cpp', 
'USB/usb-eyetoy/cam-linux.cpp', 
'USB/linux/config.cpp', 
'USB/linux/config-gtk.cpp', 
'USB/linux/util.cpp', 
'USB/qemu-usb/input-keymap-linux-to-qcode.cpp', 
'USB/usb-pad/api_init_linux.cpp', 
'USB/usb-eyetoy/api_init_linux.cpp', 
'USB/usb-hid/api_init_linux.cpp', 
'USB/usb-mic/api_init_linux.cpp']
# USB headers
pcsx2usbheaders = [
'USB/USB.h', 
'USB/proxybase.h', 
'USB/deviceproxy.h', 
'USB/configuration.h', 
'USB/platcompat.h', 
'USB/helpers.h', 
'USB/readerwriterqueue/readerwriterqueue.h', 
'USB/readerwriterqueue/atomicops.h', 
'USB/qemu-usb/glib.h', 
'USB/qemu-usb/vl.h', 
'USB/qemu-usb/qusb.h', 
'USB/qemu-usb/USBinternal.h', 
'USB/qemu-usb/desc.h', 
'USB/qemu-usb/iov.h', 
'USB/qemu-usb/queue.h', 
'USB/qemu-usb/hid.h', 
'USB/qemu-usb/input-keymap.h', 
'USB/usb-msd/usb-msd.h', 
'USB/usb-pad/usb-pad.h', 
'USB/usb-pad/padproxy.h', 
'USB/usb-pad/lg/lg_ff.h', 
'USB/usb-mic/audio.h', 
'USB/usb-mic/audiodev.h', 
'USB/usb-mic/audiodeviceproxy.h', 
'USB/usb-mic/usb-mic-singstar.h', 
'USB/usb-mic/usb-headset.h', 
'USB/usb-mic/audiodev-noop.h', 
'USB/usb-eyetoy/jpgd/jpgd.h', 
'USB/usb-eyetoy/jo_mpeg.h', 
'USB/usb-eyetoy/videodeviceproxy.h', 
'USB/usb-eyetoy/videodev.h', 
'USB/usb-eyetoy/usb-eyetoy-webcam.h', 
'USB/usb-eyetoy/ov519.h', 
'USB/usb-hid/hidproxy.h', 
'USB/usb-hid/usb-hid.h', 
'USB/usb-hid/noop.h', 
'USB/shared/shared_usb.h', 
'USB/shared/inifile_usb.h', 
'USB/shared/ringbuffer.h', 
'USB/linux/ini.h', 
'USB/linux/actualfile.h', 
'USB/linux/config.h', 
'USB/linux/util.h', 
'USB/gtk.h', 
'USB/icon_buzz_24.h', 
'USB/usb-pad/joydev/joydev.h', 
'USB/usb-pad/evdev/shared.h', 
'USB/usb-pad/evdev/evdev.h', 
'USB/usb-pad/evdev/evdev-ff.h', 
'USB/usb-hid/evdev/evdev.h', 
'USB/usb-hid/evdev/evdev.cpp', 
'USB/usb-eyetoy/cam-linux.h', 
'USB/qemu-usb/input-keymap-linux-to-qcode.h'
]
if 'PULSEAUDIO_FOUND'
  pcsx2usbsources = [pcsx2usbsources, 'USB/usb-mic/audiodev-pulse.cpp']
  pcsx2usbheaders = [pcsx2usbheaders, 'USB/usb-mic/audiodev-pulse.h']
endif
# PAD resources pre-compilation
padimgheader = '${CMAKE_BINARY_DIR}/pcsx2/PAD/Linux/ImgHeader'
padimg = '${CMAKE_SOURCE_DIR}/pcsx2/PAD/Linux/Img'
# include_directories('${CMAKE_BINARY_DIR}/pcsx2/PAD/Linux/')
# file(['MAKE_DIRECTORY', padimgheader])
# foreach(['result_file', 'IN', 'ITEMS', 'analog', 'circle', 'cross', 'dp_bottom', 'dp_left', 'dp_right', 'dp_up', 'dualshock2', 'joystick_cursor', 'l1', 'r1', 'l2', 'r2', 'l3', 'r3', 'select', 'square', 'start', 'triangle', 'arrow_up', 'arrow_bottom', 'arrow_left', 'arrow_right'])
# add_custom_command(['OUTPUT', '${PADImgHeader}/${result_file}.h', 'COMMAND', 'perl', cmake_source_dir, '/linux_various/hex2h.pl', '${PADImg}/${result_file}.png', '${PADImgHeader}/${result_file}'])
# endforeach()
# add_custom_command(['OUTPUT', cmake_binary_dir, '/pcsx2/PAD/Linux/resources_pad.h', 'COMMAND', 'glib-compile-resources', '--sourcedir', '${CMAKE_SOURCE_DIR}/pcsx2/PAD/Linux/res', '--generate-header', '--c-name', 'PAD_res', '${CMAKE_SOURCE_DIR}/pcsx2/PAD/Linux/res/PAD-res.xml', '--target=${CMAKE_BINARY_DIR}/pcsx2/PAD/Linux/resources_pad.h', 'DEPENDS', cmake_source_dir, '/pcsx2/PAD/Linux/res/PAD-res.xml', cmake_source_dir, '/pcsx2/PAD/Linux/res/game_controller_db.txt'])
# add_custom_command(['OUTPUT', cmake_binary_dir, '/pcsx2/PAD/Linux/resources_pad.cpp', 'COMMAND', 'glib-compile-resources', '--sourcedir', '${CMAKE_SOURCE_DIR}/pcsx2/PAD/Linux/res', '--generate-source', '--c-name', 'PAD_res', '${CMAKE_SOURCE_DIR}/pcsx2/PAD/Linux/res/PAD-res.xml', '--target=${CMAKE_BINARY_DIR}/pcsx2/PAD/Linux/resources_pad.cpp', 'DEPENDS', cmake_source_dir, '/pcsx2/PAD/Linux/res/PAD-res.xml', cmake_source_dir, '/pcsx2/PAD/Linux/res/game_controller_db.txt'])
# PAD/Linux sources 
pcsx2padsources = [
'PAD/Linux/controller.cpp', 
'PAD/Linux/GamePad.cpp', 
'PAD/Linux/SDL/joystick.cpp', 
'PAD/Linux/keyboard.cpp', 
'PAD/Linux/KeyStatus.cpp', 
'PAD/Linux/PAD.cpp', 
'PAD/Linux/state_management.cpp', 
'PAD/Linux/wx_dialog/dialog.cpp', 
'PAD/Linux/wx_dialog/opPanel.cpp', 
'PAD/Linux/wx_dialog/GamepadConfiguration.cpp', 
'PAD/Linux/wx_dialog/JoystickConfiguration.cpp', 
'PAD/Linux/ini.cpp', 
'PAD/Linux/linux.cpp', 
cmake_binary_dir, '/pcsx2/PAD/Linux/resources_pad.cpp'
]
# PAD/Linux headers
pcsx2padheaders = ['PAD/Linux/bitwise.h', 'PAD/Linux/controller.h', 'PAD/Linux/GamePad.h', 'PAD/Linux/SDL/joystick.h', 'PAD/Linux/keyboard.h', 'PAD/Linux/KeyStatus.h', 'PAD/Linux/PAD.h', 'PAD/Linux/state_management.h', 'PAD/Linux/wx_dialog/opPanel.h', 'PAD/Linux/wx_dialog/GamepadConfiguration.h', 'PAD/Linux/wx_dialog/JoystickConfiguration.h', cmake_binary_dir, '/pcsx2/PAD/Linux/resources_pad.h', padimgheader, '/analog.h', padimgheader, '/circle.h', padimgheader, '/cross.h', padimgheader, '/dp_bottom.h', padimgheader, '/dp_left.h', padimgheader, '/dp_right.h', padimgheader, '/dp_up.h', padimgheader, '/dualshock2.h', padimgheader, '/joystick_cursor.h', padimgheader, '/l1.h', padimgheader, '/r1.h', padimgheader, '/l2.h', padimgheader, '/r2.h', padimgheader, '/l3.h', padimgheader, '/r3.h', padimgheader, '/select.h', padimgheader, '/square.h', padimgheader, '/start.h', padimgheader, '/triangle.h', padimgheader, '/arrow_up.h', padimgheader, '/arrow_bottom.h', padimgheader, '/arrow_left.h', padimgheader, '/arrow_right.h']
# DebugTools sources
pcsx2debugtoolssources = ['DebugTools/DebugInterface.cpp', 'DebugTools/DisassemblyManager.cpp', 'DebugTools/ExpressionParser.cpp', 'DebugTools/MIPSAnalyst.cpp', 'DebugTools/MipsAssembler.cpp', 'DebugTools/MipsAssemblerTables.cpp', 'DebugTools/MipsStackWalk.cpp', 'DebugTools/Breakpoints.cpp', 'DebugTools/SymbolMap.cpp', 'DebugTools/DisR3000A.cpp', 'DebugTools/DisR5900asm.cpp', 'DebugTools/DisVU0Micro.cpp', 'DebugTools/DisVU1Micro.cpp', 'DebugTools/BiosDebugData.cpp']
# DebugTools headers
pcsx2debugtoolsheaders = ['DebugTools/DebugInterface.h', 'DebugTools/DisassemblyManager.h', 'DebugTools/ExpressionParser.h', 'DebugTools/MIPSAnalyst.h', 'DebugTools/MipsAssembler.h', 'DebugTools/MipsAssemblerTables.h', 'DebugTools/MipsStackWalk.h', 'DebugTools/Breakpoints.h', 'DebugTools/SymbolMap.h', 'DebugTools/Debug.h', 'DebugTools/DisASM.h', 'DebugTools/DisVUmicro.h', 'DebugTools/DisVUops.h', 'DebugTools/BiosDebugData.h']
# gui sources
pcsx2guisources = ['gui/AppAssert.cpp', 'gui/AppConfig.cpp', 'gui/AppCorePlugins.cpp', 'gui/AppCoreThread.cpp', 'gui/AppEventSources.cpp', 'gui/AppGameDatabase.cpp', 'gui/AppUserMode.cpp', 'gui/AppInit.cpp', 'gui/AppMain.cpp', 'gui/AppRes.cpp', 'gui/ConsoleLogger.cpp', 'gui/CpuUsageProvider.cpp', 'gui/Dialogs/AboutBoxDialog.cpp', 'gui/Dialogs/AssertionDialog.cpp', 'gui/Dialogs/BaseConfigurationDialog.cpp', 'gui/Dialogs/ConfirmationDialogs.cpp', 'gui/Dialogs/ConvertMemoryCardDialog.cpp', 'gui/Dialogs/CreateMemoryCardDialog.cpp', 'gui/Dialogs/FirstTimeWizard.cpp', 'gui/Dialogs/ImportSettingsDialog.cpp', 'gui/Dialogs/LogOptionsDialog.cpp', 'gui/Dialogs/McdConfigDialog.cpp', 'gui/Dialogs/PickUserModeDialog.cpp', 'gui/Dialogs/SysConfigDialog.cpp', 'gui/Debugger/BreakpointWindow.cpp', 'gui/Debugger/CtrlDisassemblyView.cpp', 'gui/Debugger/CtrlRegisterList.cpp', 'gui/Debugger/CtrlMemView.cpp', 'gui/Debugger/DebuggerLists.cpp', 'gui/Debugger/DisassemblyDialog.cpp', 'gui/Debugger/DebugEvents.cpp', 'gui/DriveList.cpp', 'gui/ExecutorThread.cpp', 'gui/FrameForGS.cpp', 'gui/GlobalCommands.cpp', 'gui/i18n.cpp', 'gui/IsoDropTarget.cpp', 'gui/MainFrame.cpp', 'gui/MainMenuClicks.cpp', 'gui/MemoryCardFile.cpp', 'gui/MemoryCardFolder.cpp', 'gui/MessageBoxes.cpp', 'gui/MSWstuff.cpp', 'gui/Panels/BaseApplicableConfigPanel.cpp', 'gui/Panels/BiosSelectorPanel.cpp', 'gui/Panels/CpuPanel.cpp', 'gui/Panels/DirPickerPanel.cpp', 'gui/Panels/GameFixesPanel.cpp', 'gui/Panels/GSWindowPanel.cpp', 'gui/Panels/LogOptionsPanels.cpp', 'gui/Panels/MemoryCardListPanel.cpp', 'gui/Panels/MemoryCardListView.cpp', 'gui/Panels/MiscPanelStuff.cpp', 'gui/Panels/PathsPanel.cpp', 'gui/Panels/PluginSelectorPanel.cpp', 'gui/Panels/SpeedhacksPanel.cpp', 'gui/Panels/VideoPanel.cpp', 'gui/RecentIsoList.cpp', 'gui/Saveslots.cpp', 'gui/SysState.cpp', 'gui/UpdateUI.cpp']
# gui headers
pcsx2guiheaders = ['gui/AppAccelerators.h', 'gui/AppCommon.h', 'gui/AppConfig.h', 'gui/AppCorePlugins.h', 'gui/AppCoreThread.h', 'gui/AppEventListeners.h', 'gui/AppForwardDefs.h', 'gui/AppGameDatabase.h', 'gui/App.h', 'gui/ApplyState.h', 'gui/AppSaveStates.h', 'gui/ConsoleLogger.h', 'gui/CpuUsageProvider.h', 'gui/Debugger/BreakpointWindow.h', 'gui/Debugger/CtrlDisassemblyView.h', 'gui/Debugger/CtrlMemView.h', 'gui/Debugger/CtrlRegisterList.h', 'gui/Debugger/DebugEvents.h', 'gui/Debugger/DebuggerLists.h', 'gui/Debugger/DisassemblyDialog.h', 'gui/Dialogs/BaseConfigurationDialog.inl', 'gui/Dialogs/ConfigurationDialog.h', 'gui/Dialogs/LogOptionsDialog.h', 'gui/Dialogs/ModalPopups.h', 'gui/DriveList.h', 'gui/GSFrame.h', 'gui/i18n.h', 'gui/IsoDropTarget.h', 'gui/MainFrame.h', 'gui/MemoryCardFile.h', 'gui/MemoryCardFolder.h', 'gui/MSWstuff.h', 'gui/Panels/ConfigurationPanels.h', 'gui/Panels/LogOptionsPanels.h', 'gui/Panels/MemoryCardPanels.h', 'gui/pxEventThread.h', 'gui/RecentIsoList.h', 'gui/Saveslots.h']
# Warning: the declaration of the .h are mandatory in case of resources files. It will ensure the creation
# from the bin2cpp tools at right good moment (ie .h must be created before the pcsx2 compilation)
# Gui resources headers
res_bin = '${CMAKE_BINARY_DIR}/pcsx2/gui/Resources'
res_src = '${CMAKE_SOURCE_DIR}/pcsx2/gui/Resources'
pcsx2guiresources = [res_bin, '/AppIcon16.h', res_bin, '/AppIcon32.h', res_bin, '/AppIcon64.h', res_bin, '/BackgroundLogo.h', res_bin, '/ButtonIcon_Camera.h', res_bin, '/ConfigIcon_Cpu.h', res_bin, '/ConfigIcon_Gamefixes.h', res_bin, '/ConfigIcon_MemoryCard.h', res_bin, '/ConfigIcon_Paths.h', res_bin, '/ConfigIcon_Plugins.h', res_bin, '/ConfigIcon_Speedhacks.h', res_bin, '/ConfigIcon_Video.h', res_bin, '/Logo.h', res_bin, '/Breakpoint_Active.h', res_bin, '/Breakpoint_Inactive.h']
# IPU sources
pcsx2ipusources = ['IPU/IPU.cpp', 'IPU/IPU_Fifo.cpp', 'IPU/IPUdither.cpp', 'IPU/IPUdma.cpp', 'IPU/mpeg2lib/Idct.cpp', 'IPU/mpeg2lib/Mpeg.cpp', 'IPU/yuv2rgb.cpp']
# IPU headers
pcsx2ipuheaders = ['IPU/IPUdma.h', 'IPU/IPU_Fifo.h', 'IPU/IPU.h', 'IPU/mpeg2lib/Mpeg.h', 'IPU/mpeg2lib/Vlc.h', 'IPU/yuv2rgb.h']
# Linux sources
pcsx2linuxsources = ['CDVD/Linux/DriveUtility.cpp', 'CDVD/Linux/IOCtlSrc.cpp', 'gui/CpuUsageProviderLnx.cpp', 'Linux/LnxConsolePipe.cpp', 'Linux/LnxKeyCodes.cpp', 'Linux/LnxFlatFileReader.cpp']
pcsx2osxsources = ['CDVD/Linux/DriveUtility.cpp', 'CDVD/Linux/IOCtlSrc.cpp', 'gui/CpuUsageProviderLnx.cpp', 'Linux/LnxConsolePipe.cpp', 'Darwin/DarwinFlatFileReader.cpp']
pcsx2freebsdsources = ['CDVD/Linux/DriveUtility.cpp', 'CDVD/Linux/IOCtlSrc.cpp', 'gui/CpuUsageProviderLnx.cpp', 'Linux/LnxConsolePipe.cpp', 'Linux/LnxKeyCodes.cpp', 'Darwin/DarwinFlatFileReader.cpp']
# Linux headers
pcsx2linuxheaders = 
# ps2 sources
pcsx2ps2sources = ['ps2/BiosTools.cpp', 'ps2/pgif.cpp', 'ps2/LegacyDmac.cpp', 'ps2/Iop/IopHwRead.cpp', 'ps2/Iop/IopHwWrite.cpp', 'ps2/Iop/PsxBios.cpp']
# ps2 headers
pcsx2ps2headers = ['ps2/BiosTools.h', 'ps2/eeHwTraceLog.inl', 'ps2/pgif.h', 'ps2/HwInternal.h', 'ps2/Iop/IopHw_Internal.h']
# RDebug sources
pcsx2rdebugsources = ['RDebug/deci2.cpp', 'RDebug/deci2_dbgp.cpp', 'RDebug/deci2_dcmp.cpp', 'RDebug/deci2_drfp.cpp', 'RDebug/deci2_iloadp.cpp', 'RDebug/deci2_netmp.cpp', 'RDebug/deci2_ttyp.cpp']
# RDebug headers
pcsx2rdebugheaders = ['RDebug/deci2_dbgp.h', 'RDebug/deci2_dcmp.h', 'RDebug/deci2_drfp.h', 'RDebug/deci2.h', 'RDebug/deci2_iloadp.h', 'RDebug/deci2_netmp.h', 'RDebug/deci2_ttyp.h']
# Recording sources
rec_src = 'Recording'
rec_vp_src = '${rec_src}/VirtualPad'
pcsx2recordingsources = [rec_src, '/InputRecording.cpp', rec_src, '/InputRecordingControls.cpp', rec_src, '/InputRecordingFile.cpp', rec_src, '/NewRecordingFrame.cpp', rec_src, '/PadData.cpp', rec_vp_src, '/VirtualPad.cpp', rec_vp_src, '/VirtualPadData.cpp', rec_vp_src, '/VirtualPadResources.cpp']
# Recording headers
pcsx2recordingheaders = [rec_src, '/InputRecording.h', rec_src, '/InputRecordingControls.h', rec_src, '/InputRecordingFile.h', rec_src, '/NewRecordingFrame.h', rec_src, '/PadData.h', rec_src, '/Utilities/InputRecordingLogger.h', rec_vp_src, '/VirtualPad.h', rec_vp_src, '/VirtualPadData.h', rec_vp_src, '/VirtualPadResources.h']
# Warning: the declaration of the .h are mandatory in case of resources files. It will ensure the creation
# from the bin2cpp tools at the right moment (ie .h must be created before the pcsx2 compilation)
# Recording - VirtualPad resources headers
res_rec_vp_src = '${CMAKE_SOURCE_DIR}/pcsx2/Recording/VirtualPad/img'
pcsx2recordingvirtualpadresources = [res_rec_vp_src, '/circlePressed.h', res_rec_vp_src, '/controllerFull.h', res_rec_vp_src, '/controllerHalf.h', res_rec_vp_src, '/controllerThreeQuarters.h', res_rec_vp_src, '/crossPressed.h', res_rec_vp_src, '/downPressed.h', res_rec_vp_src, '/l1Pressed.h', res_rec_vp_src, '/l2Pressed.h', res_rec_vp_src, '/l3Pressed.h', res_rec_vp_src, '/leftPressed.h', res_rec_vp_src, '/r1Pressed.h', res_rec_vp_src, '/r2Pressed.h', res_rec_vp_src, '/r3Pressed.h', res_rec_vp_src, '/rightPressed.h', res_rec_vp_src, '/selectPressed.h', res_rec_vp_src, '/squarePressed.h', res_rec_vp_src, '/startPressed.h', res_rec_vp_src, '/trianglePressed.h', res_rec_vp_src, '/upPressed.h']
# System sources
pcsx2systemsources = ['System/SysCoreThread.cpp', 'System/SysThreadBase.cpp']
# System headers
pcsx2systemheaders = ['System/RecTypes.h', 'System/SysThreads.h']
# Utilities sources
pcsx2utilitiessources = 'Utilities/FileUtils.cpp'
# Utilities headers
pcsx2utilitiesheaders = 'Utilities/AsciiFile.h'
# Zip tools utilies sources
pcsx2ziptoolssources = ['ZipTools/thread_gzip.cpp', 'ZipTools/thread_lzma.cpp']
# Zip tools utilies headers
pcsx2ziptoolsheaders = 'ZipTools/ThreadedZipTools.h'
# Windows sources
pcsx2windowssources = ['CDVD/Windows/DriveUtility.cpp', 'CDVD/Windows/IOCtlSrc.cpp', 'gui/CpuUsageProviderMSW.cpp', 'windows/FlatFileReaderWindows.cpp', 'windows/Optimus.cpp', 'windows/PatchBrowser.cpp', 'windows/VCprojects/IopSif.cpp', 'windows/WinCompressNTFS.cpp', 'windows/WinConsolePipe.cpp', 'windows/WinKeyCodes.cpp']
# Windows headers
pcsx2windowsheaders = ['windows/resource.h', 'windows/Win32.h']
# x86 sources
pcsx2x86sources = ['x86/BaseblockEx.cpp', 'x86/iCOP0.cpp', 'x86/iCore.cpp', 'x86/iFPU.cpp', 'x86/iFPUd.cpp', 'x86/iMisc.cpp', 'x86/iMMI.cpp', 'x86/iR3000A.cpp', 'x86/iR3000Atables.cpp', 'x86/iR5900Misc.cpp', 'x86/ir5900tables.cpp', 'x86/ix86-32/iCore-32.cpp', 'x86/ix86-32/iR5900-32.cpp', 'x86/ix86-32/iR5900Arit.cpp', 'x86/ix86-32/iR5900AritImm.cpp', 'x86/ix86-32/iR5900Branch.cpp', 'x86/ix86-32/iR5900Jump.cpp', 'x86/ix86-32/iR5900LoadStore.cpp', 'x86/ix86-32/iR5900Move.cpp', 'x86/ix86-32/iR5900MultDiv.cpp', 'x86/ix86-32/iR5900Shift.cpp', 'x86/ix86-32/iR5900Templates.cpp', 'x86/ix86-32/recVTLB.cpp', 'x86/newVif_Dynarec.cpp', 'x86/newVif_Unpack.cpp', 'x86/newVif_UnpackSSE.cpp']
# x86 headers
pcsx2x86headers = ['x86/BaseblockEx.h', 'x86/iCOP0.h', 'x86/iCore.h', 'x86/iFPU.h', 'x86/iMMI.h', 'x86/iR3000A.h', 'x86/iR5900Arit.h', 'x86/iR5900AritImm.h', 'x86/iR5900Branch.h', 'x86/iR5900.h', 'x86/iR5900Jump.h', 'x86/iR5900LoadStore.h', 'x86/iR5900Move.h', 'x86/iR5900MultDiv.h', 'x86/iR5900Shift.h', 'x86/microVU_Alloc.inl', 'x86/microVU_Analyze.inl', 'x86/microVU_Branch.inl', 'x86/microVU_Clamp.inl', 'x86/microVU_Compile.inl', 'x86/microVU.cpp', 'x86/microVU_Execute.inl', 'x86/microVU_Flags.inl', 'x86/microVU.h', 'x86/microVU_IR.h', 'x86/microVU_Log.inl', 'x86/microVU_Lower.inl', 'x86/microVU_Macro.inl', 'x86/microVU_Misc.h', 'x86/microVU_Misc.inl', 'x86/microVU_Profiler.h', 'x86/microVU_Tables.inl', 'x86/microVU_Upper.inl', 'x86/newVif.h', 'x86/newVif_HashBucket.h', 'x86/newVif_UnpackSSE.h', 'x86/R5900_Profiler.h']
# common Sources
common = [pcsx2sources, pcsx2headers, pcsx2cdvdsources, pcsx2cdvdheaders, pcsx2spu2sources, pcsx2spu2headers, pcsx2dev9sources, pcsx2dev9headers, pcsx2padheaders, pcsx2padsources, pcsx2debugtoolssources, pcsx2guisources, pcsx2guiresources, pcsx2guiheaders, pcsx2ipusources, pcsx2ipuheaders, pcsx2ps2sources, pcsx2ps2headers, pcsx2recordingsources, pcsx2recordingvirtualpadresources, pcsx2recordingheaders, pcsx2systemsources, pcsx2systemheaders, pcsx2utilitiessources, pcsx2utilitiesheaders, pcsx2x86sources, pcsx2x86headers, pcsx2ziptoolssources, pcsx2ziptoolsheaders]
# platform sources
# Linux
if 'Linux'
  platform = [pcsx2linuxsources, pcsx2linuxheaders, pcsx2usbsources, pcsx2usbheaders]
  platform_libs = [libudev_libraries, x11_libraries]
endif
# Windows
if 'Windows'
  platform = [pcsx2windowssources, pcsx2windowsheaders]
endif
# MacOSX
if 'APPLE'
  platform = [pcsx2osxsources, pcsx2linuxheaders, pcsx2usbnullsources, pcsx2usbnullheaders]
endif
if cmake_system_name, 'MATCHES', 'FreeBSD', 'OR', cmake_system_name, 'MATCHES', 'NetBSD'
  platform = [pcsx2freebsdsources, pcsx2linuxheaders]
endif
pcsx2finalsources = [common, platform]
pcsx2finallibs = ['Utilities', 'x86emitter', 'fmt::fmt', wxwidgets_libraries, gtk2_libraries, zlib_libraries, aio_libraries, gcov_libraries, alsa_libraries, soundtouch_libraries, sdl2_libraries, pcap_library, libsamplerate_libraries, libxml2_libraries, platform_libs]
if 'PORTAUDIO_FOUND'
  pcsx2finallibs = [pcsx2finallibs, portaudio_libraries]
endif
if 'PULSEAUDIO_FOUND'
  pcsx2finallibs = [pcsx2finallibs, pulseaudio_libraries]
endif
if 'BUILTIN_GS'
  pcsx2finallibs = '${pcsx2FinalLibs} GSdx'
endif
# additonal include directories
# include_directories(['gui', 'x86', cmake_binary_dir, '/pcsx2/gui'])
### Generate the resources files
# file(['MAKE_DIRECTORY', res_bin])
# foreach(['res_file', 'IN', 'ITEMS', 'AppIcon16', 'AppIcon32', 'AppIcon64', 'BackgroundLogo', 'Logo', 'ButtonIcon_Camera', 'ConfigIcon_Cpu', 'ConfigIcon_Gamefixes', 'ConfigIcon_MemoryCard', 'ConfigIcon_Paths', 'ConfigIcon_Plugins', 'ConfigIcon_Speedhacks', 'ConfigIcon_Video', 'Breakpoint_Active', 'Breakpoint_Inactive'])
# add_custom_command(['OUTPUT', '${res_bin}/${res_file}.h', 'COMMAND', 'perl', cmake_source_dir, '/linux_various/hex2h.pl', '${res_src}/${res_file}.png', '${res_bin}/${res_file}'])
# endforeach()
### Generate Recording resource files
### Drop them into the folder alongside the png files
# foreach(['res_file', 'IN', 'ITEMS', 'circlePressed', 'controllerFull', 'controllerHalf', 'controllerThreeQuarters', 'crossPressed', 'downPressed', 'l1Pressed', 'l2Pressed', 'l3Pressed', 'leftPressed', 'r1Pressed', 'r2Pressed', 'r3Pressed', 'rightPressed', 'selectPressed', 'squarePressed', 'startPressed', 'trianglePressed', 'upPressed'])
# add_custom_command(['OUTPUT', '${res_rec_vp_src}/${res_file}.h', 'COMMAND', 'perl', cmake_source_dir, '/linux_various/hex2h.pl', '${res_rec_vp_src}/${res_file}.png', '${res_rec_vp_src}/${res_file}'])
# endforeach()
if 'USE_VTUNE'
  pcsx2finallibs = [pcsx2finallibs, vtune_libraries]
endif
# add_pcsx2_executable([output, '${pcsx2FinalSources}', '${pcsx2FinalLibs}', '${pcsx2FinalFlags}'])
# target_compile_features([output, 'PRIVATE', 'cxx_std_17'])
if 'COMMAND', 'target_precompile_headers'
  # message('Using precompiled headers.')
  # target_precompile_headers([output, 'PRIVATE', 'PrecompiledHeader.h'])
endif
if 'APPLE'
  # MacOS defaults to having a maximum protection of the __DATA segment of rw (non-executable)
  # We have a bunch of page-sized arrays in bss that we use for jit
  # Obviously not being able to make those arrays executable would be a problem
  # target_link_options([output, 'PRIVATE', '-Wl,-segprot,__DATA,rwx,rw'])
  # set_target_properties([output, 'PROPERTIES', 'MACOSX_BUNDLE', 'true', 'MACOSX_BUNDLE_INFO_PLIST', '${CMAKE_CURRENT_SOURCE_DIR}/gui/Resources/Info.plist.in', 'OUTPUT_NAME', 'PCSX2'])
  # target_sources([output, 'PRIVATE', '${CMAKE_CURRENT_SOURCE_DIR}/gui/Resources/PCSX2.icns'])
  # set_source_files_properties(['${CMAKE_CURRENT_SOURCE_DIR}/gui/Resources/PCSX2.icns', 'PROPERTIES', 'MACOSX_PACKAGE_LOCATION', 'Resources'])
  # If they say to skip postprocess bundle, leave the target in but make it so they have
  # to manually run it
  if 'SKIP_POSTPROCESS_BUNDLE'
    postprocessbundletype = ''
  else
    postprocessbundletype = 'ALL'
  endif
  # Use custom_target and not custom_command so plugins can add themselves as dependencies
  # add_custom_target(['pcsx2-postprocess-bundle', postprocessbundletype, 'COMMAND', cmake_command, '-DPCSX2_BUNDLE_PATH=$<TARGET_FILE_DIR:${Output}>/../..', '-P', cmake_source_dir, '/cmake/Pcsx2PostprocessBundle.cmake'])
  # add_dependencies(['pcsx2-postprocess-bundle', output])
endif
if 'NOT', 'DISABLE_SETCAP'
  if 'PACKAGE_MODE'
    # install(['CODE', 'execute_process(COMMAND /bin/bash -c \"echo 'Enabling networking capability on Linux...';set -x; [ -f ${BIN_DIR}/${Output} ] && sudo setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' ${BIN_DIR}/${Output}; set +x\")'])
  else
    # install(['CODE', 'execute_process(COMMAND /bin/bash -c \"echo 'Enabling networking capability on Linux...';set -x; [ -f ${CMAKE_SOURCE_DIR}/bin/${Output} ] && sudo setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' ${CMAKE_SOURCE_DIR}/bin/${Output}; set +x\")'])
  endif
endif
