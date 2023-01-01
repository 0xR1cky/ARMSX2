#-------------------------------------------------------------------------------
#                       Search all libraries on the system
#-------------------------------------------------------------------------------
if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
	find_package(Git)
endif()
if (WIN32)
	# We bundle everything on Windows
	add_subdirectory(3rdparty/zlib EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/libpng EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/libjpeg EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/soundtouch EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/wil EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/xz EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/D3D12MemAlloc EXCLUDE_FROM_ALL)
	set(FFMPEG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include")
else()
	find_package(PCAP REQUIRED)
	find_package(LibLZMA REQUIRED)
	make_imported_target_if_missing(LibLZMA::LibLZMA LIBLZMA)

	# Using find_package OpenGL without either setting your opengl preference to GLVND or LEGACY
	# is deprecated as of cmake 3.11.
	if(USE_OPENGL)
		set(OpenGL_GL_PREFERENCE GLVND)
		find_package(OpenGL REQUIRED)
	endif()
	# On macOS, Mono.framework contains an ancient version of libpng.  We don't want that.
	# Avoid it by telling cmake to avoid finding frameworks while we search for libpng.
	set(FIND_FRAMEWORK_BACKUP ${CMAKE_FIND_FRAMEWORK})
	set(CMAKE_FIND_FRAMEWORK NEVER)
	find_package(PNG REQUIRED)
	set(CMAKE_FIND_FRAMEWORK ${FIND_FRAMEWORK_BACKUP})
	find_package(Vtune)

	# Use bundled ffmpeg v4.x.x headers if we can't locate it in the system.
	# We'll try to load it dynamically at runtime.
	find_package(FFMPEG COMPONENTS avcodec avformat avutil swresample swscale)
	if(NOT FFMPEG_FOUND)
		message(WARNING "FFmpeg not found, using bundled headers.")
		set(FFMPEG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include")
	endif()

	find_package(ZLIB REQUIRED)

	## Use pcsx2 package to find module
	include(FindLibc)

	## Use CheckLib package to find module
	include(CheckLib)

	if(UNIX AND NOT APPLE)
		if(USE_OPENGL)
			check_lib(EGL EGL EGL/egl.h)
		endif()
		if(X11_API)
			check_lib(X11_XCB X11-xcb X11/Xlib-xcb.h)
			check_lib(XCB xcb xcb/xcb.h)
			check_lib(XRANDR Xrandr X11/extensions/Xrandr.h)
		endif()

		if(Linux)
			check_lib(AIO aio libaio.h)
			# There are two udev pkg config files - udev.pc (wrong), libudev.pc (correct)
			# When cross compiling, pkg-config will be skipped so we have to look for
			# udev (it'll automatically be prefixed with lib). But when not cross
			# compiling, we have to look for libudev.pc. Argh. Hence the silliness below.
			if(CMAKE_CROSSCOMPILING)
				check_lib(LIBUDEV udev libudev.h)
			else()
				check_lib(LIBUDEV libudev libudev.h)
			endif()
		endif()
	endif()

	check_lib(SOUNDTOUCH SoundTouch SoundTouch.h PATH_SUFFIXES soundtouch)

	if(NOT QT_BUILD)
		find_optional_system_library(SDL2 3rdparty/sdl2 2.0.12)
	endif()

	if(UNIX AND NOT APPLE)
		find_package(X11 REQUIRED)
		make_imported_target_if_missing(X11::X11 X11)

		if(WAYLAND_API)
			find_package(Wayland REQUIRED)
		endif()
	endif()
endif(WIN32)

# Require threads on all OSes.
find_package(Threads REQUIRED)

set(ACTUALLY_ENABLE_TESTS ${ENABLE_TESTS})
if(ENABLE_TESTS)
	if(NOT EXISTS "${CMAKE_SOURCE_DIR}/3rdparty/gtest/CMakeLists.txt")
		message(WARNING "ENABLE_TESTS was on but gtest was not found, unit tests will not be enabled")
		set(ACTUALLY_ENABLE_TESTS Off)
	endif()
endif()

#----------------------------------------
# Check correctness of the parameter
# Note: wxWidgets_INCLUDE_DIRS must be defined
#----------------------------------------
include(ApiValidation)

# Blacklist bad GCC
if(GCC_VERSION VERSION_EQUAL "7.0" OR GCC_VERSION VERSION_EQUAL "7.1")
	GCC7_BUG()
endif()

if((GCC_VERSION VERSION_EQUAL "9.0" OR GCC_VERSION VERSION_GREATER "9.0") AND GCC_VERSION LESS "9.2")
	message(WARNING "
	It looks like you are compiling with 9.0.x or 9.1.x. Using these versions is not recommended,
	as there is a bug known to cause the compiler to segfault while compiling. See patch
	https://gitweb.gentoo.org/proj/gcc-patches.git/commit/?id=275ab714637a64672c6630cfd744af2c70957d5a
	Even with that patch, compiling with LTO may still segfault. Use at your own risk!
	This text being in a compile log in an open issue may cause it to be closed.")
endif()

find_optional_system_library(fmt 3rdparty/fmt/fmt 7.1.3)
find_optional_system_library(ryml 3rdparty/rapidyaml/rapidyaml 0.4.0)
find_optional_system_library(zstd 3rdparty/zstd 1.4.5)
if (${zstd_TYPE} STREQUAL System)
	alias_library(Zstd::Zstd zstd::libzstd_shared)
	alias_library(pcsx2-zstd zstd::libzstd_shared)
endif()
find_optional_system_library(libzip 3rdparty/libzip 1.8.0)

if(QT_BUILD)
	# Default to bundled Qt6 for Windows.
	if(WIN32 AND NOT DEFINED Qt6_DIR)
		set(Qt6_DIR ${CMAKE_SOURCE_DIR}/3rdparty/qt/6.4.0/msvc2022_64/lib/cmake/Qt6)
	endif()

	# Find the Qt components that we need.
	find_package(Qt6 COMPONENTS CoreTools Core GuiTools Gui WidgetsTools Widgets Network LinguistTools REQUIRED)

	if (APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET AND "${CMAKE_OSX_DEPLOYMENT_TARGET}" VERSION_LESS 10.15)
		get_target_property(QT_FEATURES Qt6::Core QT_ENABLED_PUBLIC_FEATURES)
		if (cxx17_filesystem IN_LIST QT_FEATURES)
			message("Qt compiled with std::filesystem support, requires macOS 10.15")
			set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
		endif()
	endif()

	# We use the bundled (latest) SDL version for Qt.
	find_optional_system_library(SDL2 3rdparty/sdl2 2.0.22)

	# rcheevos backend for RetroAchievements.
	if(USE_ACHIEVEMENTS)
		add_subdirectory(3rdparty/rcheevos EXCLUDE_FROM_ALL)
	endif()

	# Discord-RPC library for rich presence.
	if(USE_DISCORD_PRESENCE)
		add_subdirectory(3rdparty/rapidjson EXCLUDE_FROM_ALL)
		add_subdirectory(3rdparty/discord-rpc EXCLUDE_FROM_ALL)
	endif()

	# Demangler for the debugger
	add_subdirectory(3rdparty/demangler EXCLUDE_FROM_ALL)
endif()

if(NOT WIN32 AND QT_BUILD)
	find_package(CURL REQUIRED)
endif()

add_subdirectory(3rdparty/lzma EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/libchdr EXCLUDE_FROM_ALL)

# rapidyaml includes fast_float as a submodule, saves us pulling it in directly.
# Normally, we'd just pull in the cmake project, and link to it, but... it seems to enable
# permissive mode, which breaks other parts of PCSX2. So, we'll just create a target here
# for now.
#add_subdirectory(3rdparty/rapidyaml/rapidyaml/ext/c4core/src/c4/ext/fast_float EXCLUDE_FROM_ALL)
add_library(fast_float INTERFACE)
target_include_directories(fast_float INTERFACE 3rdparty/rapidyaml/rapidyaml/ext/c4core/src/c4/ext/fast_float/include)

add_subdirectory(3rdparty/jpgd EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/simpleini EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/imgui EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/cpuinfo EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/zydis EXCLUDE_FROM_ALL)

if(USE_OPENGL)
	add_subdirectory(3rdparty/glad EXCLUDE_FROM_ALL)
endif()

if(USE_VULKAN)
	add_subdirectory(3rdparty/glslang EXCLUDE_FROM_ALL)
	add_subdirectory(3rdparty/vulkan-headers EXCLUDE_FROM_ALL)
endif()

if(CUBEB_API)
	add_subdirectory(3rdparty/cubeb EXCLUDE_FROM_ALL)
	target_compile_options(cubeb PRIVATE "-w")
	target_compile_options(speex PRIVATE "-w")
endif()

