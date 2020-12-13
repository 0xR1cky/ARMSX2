project('Pcsx2', 'cpp')


tag = vcs_tag(command: ['git', 'describe', '--always'], 
  input: 'common/include/svnrev.h', output: 'svnrev.h', fallback: 'Unknown')

#if linux
alsa = dependency('alsa', required : false)
x11 = dependency('x11', required : false)
gtk = dependency('gtk3', required : false)
sdl = dependency('sdl2', required : false)
udev = dependency('libudev', required : false)
aio = dependency('libudev', required : true) 
egl = dependency('egl', required : true) 
x11_xcb = dependency('x11-xcb', required : true) 
#endif

pcap = dependency('pcap', required : false)
libxml2 = dependency('libxml2', required : false)
freetype = dependency('freetype', required : false)
gettext_ = dependency('gettext', required : false) # included in i18n
git = dependency('git', required : false) # only necessary when .git exists? to check
lzma = dependency('liblzma', required : false)
harfbuzz = dependency('harfbuzz', required : false)
portaudio = dependency('portaudio', required : false)
soundtouch = dependency('soundtouch', required : false)


opengl = dependency('opengl', required : false)
png = dependency('png', required : false)
vtune = dependency('vtune', required : false)
zlib = dependency('zlib', required : false)
libc = dependency('libc', required : false)
libsamplerate = dependency('libsamplerate', required : false)


fmt = dependency('fmt', required : false)
yaml = dependency('yaml-cpp', required : false) # we should use bundled by default
zlib = dependency('zlib', required : false)

wx_dep = dependency(
  'wxwidgets', version : '>=3.0.0', modules : ['base', 'core', 'adv'],
) # we should probably add unicode and gtk3 by default here


# TODO: use bundled libs on windows/when not found

common_includes = include_directories('common/include', 'common/include/Utilities',
'common/include/x86emitter', tag)

#TODO: ensure wxwidgets is under SDL2 WX_vs_SDL()

subdir('locales')
subdir('common/src/Utilities')
subdir('common/src/x86emitter')
subdir('pcsx2')
subdir('plugins')

subdir('tests/ctest')
