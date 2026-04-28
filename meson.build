project('gthumb',
  [ 'vala', 'c', 'cpp' ],
  version: '4.0.alpha',
  license: 'GPL2+',
  meson_version: '>=1.1.0'
)

glib_version = '2.84'
gtk_version = '>= 4.18.5'
libadwaita_version = '>= 1.8.0'
lcms2_version = '>= 2.6'
colord_version = '>= 1.3'
gstreamer_version = '>= 1.26.0'
exiv2_version = '>= 0.28'
libpng_version = '>= 1.6'
libwebp_version = '>= 0.2.0'
librsvg_version = '>= 2.34.0'
libheif_version = '>= 1.11'
libjxl_version = '>= 0.3.0'
libgif_version = '>= 5.2.0'
libportal_version = '>= 0.9'
libraw_version = '>= 0.22'

c_comp = meson.get_compiler('c')
cpp_comp = meson.get_compiler('cpp')
vala_comp = meson.get_compiler('vala')
gnome = import('gnome')
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
podir = join_paths(meson.project_source_root(), 'po')

# colord

if get_option('colord')
  colord_dep = dependency('colord',
    version: colord_version,
    required: false
  )
  use_colord = colord_dep.found()
else
  use_colord = false
endif

# libjpeg

libjpeg_dep = [
  dependency('zlib'),
  c_comp.find_library('m'),
  c_comp.find_library('jpeg')
]

have_progressive_jpeg = c_comp.has_function(
  'jpeg_simple_progression',
  prefix: '#include <stdio.h>\n#include<jpeglib.h>',
  dependencies: libjpeg_dep
)

# libwebp

if get_option('libwebp')
  libwebp_dep = [
    dependency('libwebp', version: libwebp_version, required: false),
    dependency('libwebpdemux', version: libwebp_version, required: false)
  ]
  use_libwebp = libwebp_dep[0].found()
else
  use_libwebp = false
endif

# librsvg

if get_option('librsvg')
  librsvg_dep = dependency('librsvg-2.0', version: librsvg_version, required: false)
  use_librsvg = librsvg_dep.found()
else
  use_librsvg = false
endif

# gstreamer

gstreamer_dep = dependency('gstreamer-1.0',
  version: gstreamer_version,
  required: false
)
gstreamer_plugins_base_dep = dependency('gstreamer-plugins-base-1.0',
  version: gstreamer_version,
  required: false
)
gstreamer_video_dep = dependency('gstreamer-video-1.0',
  version: gstreamer_version,
  required: false
)
gstreamer_dep_list = [
  gstreamer_dep,
  gstreamer_plugins_base_dep,
  gstreamer_video_dep
]

# libportal

libportal_deps = [
  dependency('libportal', version : libportal_version),
  dependency('libportal-gtk4', version : libportal_version)
]

# libheif

if get_option('libheif')
  libheif_dep = dependency('libheif', version : libheif_version, required : false)
  use_libheif = libheif_dep.found()
else
  use_libheif = false
endif

# libjxl

if get_option('libjxl')
  libjxl_deps = [
    dependency('libjxl', version : libjxl_version, required : false),
    dependency('libjxl_threads', version : libjxl_version, required : false)
  ]
  use_libjxl = libjxl_deps[0].found()
else
  use_libjxl = false
endif

# libtiff

if get_option('libtiff')
  libtiff_dep = c_comp.find_library('tiff', required: false)
  use_libtiff = libtiff_dep.found()
  libtiff_deps = [
    dependency('zlib'),
    c_comp.find_library('m'),
    libtiff_dep
  ]
else
  use_libtiff = false
endif

# libgif

if get_option('libgif')
  libgif_dep = c_comp.find_library('gif', required: false)
  use_libgif = libgif_dep.found()
else
  use_libgif = false
endif

# libraw

if get_option('libraw')
  libraw_dep = dependency('libraw', version: libraw_version, required: false)
  use_libraw = libraw_dep.found()
else
  use_libraw = false
endif

gettext_package = meson.project_name()
log_domain = meson.project_name()
prefix = get_option('prefix')
privexecdir = join_paths(prefix, get_option('libexecdir'), meson.project_name())

add_project_arguments([
  '--disable-warnings',
  '--enable-experimental',
  '--vapidir', join_paths(meson.current_source_dir(), 'src/vapi'),
  '--target-glib=@0@'.format(glib_version),
], language: 'vala')

add_project_arguments([
  '-w',
  '-Wno-error=return-mismatch',
  '-Wno-error=implicit-function-declaration',
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DG_LOG_DOMAIN="@0@"'.format(log_domain),
  '-DLOCALEDIR="@0@"'.format(join_paths(prefix, get_option('localedir'))),
], language: 'c')

subdir('src')
subdir('po')
subdir('data')

# Summary

summary(
  {
    'prefix': prefix,
    'prog jpeg': have_progressive_jpeg,
    'colord': use_colord,
    'libwebp': use_libwebp,
    'librsvg': use_librsvg,
    'libheif': use_libheif,
    'libjxl': use_libjxl,
    'libtiff': use_libtiff,
    'libgif': use_libgif,
    'libraw': use_libraw,
  },
  section: 'Configuration summary:',
)
