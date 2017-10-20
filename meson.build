project('gthumb', ['c', 'cpp'],
  license : 'GPL2+',
  version : '3.5.3',
  meson_version : '>=0.43'
)

glib_version = '>=2.36.0'
gtk_version = '>=3.16'
exiv2_version = '>=0.21'
clutter_version = '>=1.12.0'
clutter_gtk_version = '>=1.0.0'
gstreamer_version = '>=1.0.0'
libraw_version = '>=0.14'
libsoup_version = '>=2.42'
libsecret_version = '>=0.11'
libbrasero_version = '>=3.2.0'
libchamplain_version = '>=0.12.0'
librsvg_version = '>=2.34.0'
libwebp_version = '>=0.2.0'
libjson_glib_version = '>=0.15.0'
webkit2_version = '>=1.10.0'
lcms2_version = '>=2.6'
colord_version = '>=1.3'

version_array = meson.project_version().split('.')
api_version = '.'.join([version_array[0], version_array[1]])

gnome = import('gnome')
i18n = import('i18n')
gettext_package = meson.project_name()
c_comp = meson.get_compiler('c')
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
pkgdatadir = join_paths(datadir, meson.project_name())
pkgincludedir = join_paths(prefix, 'include', meson.project_name())
pkglibdir = join_paths(prefix, 'lib', meson.project_name())
po_dir = join_paths(meson.source_root(), 'po')
extensions_install_dir = join_paths(pkglibdir, 'extensions')
ui_install_dir = join_paths(pkgdatadir, 'ui')
icons_install_dir = join_paths(pkgdatadir, 'icons')
if get_option('run-in-place')
  ui_data_dir = join_paths(meson.source_root(), 'data', 'ui')
  icons_data_dir = join_paths(meson.source_root(), 'data', 'icons')
  extensions_data_dir = join_paths(meson.build_root(), 'extensions')
else
  ui_data_dir = ui_install_dir
  icons_data_dir = icons_install_dir
  extensions_data_dir = extensions_install_dir  
endif

# Dependencies

if get_option('exiv2')
  exiv2_dep = dependency('exiv2', version : exiv2_version, required : false)
  use_exiv2 = exiv2_dep.found()
else
  use_exiv2 = false
endif

if get_option('clutter')
  clutter_dep = dependency('clutter-1.0', version : clutter_version, required : false)
  clutter_gtk_dep = dependency('clutter-gtk-1.0', version : clutter_gtk_version, required : false)
  clutter_deps = [ clutter_dep, clutter_gtk_dep ]
  use_clutter = clutter_dep.found() and clutter_gtk_dep.found()
else
  use_clutter = false
endif

if get_option('gstreamer')
  gstreamer_dep = dependency('gstreamer-1.0', version : gstreamer_version, required : false)
  gstreamer_plugins_base_dep = dependency('gstreamer-plugins-base-1.0', version : gstreamer_version, required : false)
  gstreamer_video_dep = dependency('gstreamer-video-1.0', version : gstreamer_version, required : false)
  gstreamer_deps = [ gstreamer_dep, gstreamer_plugins_base_dep, gstreamer_video_dep ]
  use_gstreamer = gstreamer_dep.found() and gstreamer_plugins_base_dep.found() and gstreamer_video_dep.found()
else
  use_gstreamer = false
endif

if get_option('libchamplain')
  libchamplain_dep = dependency('champlain-0.12', version : libchamplain_version, required : false)
  libchamplain_gtk_dep = dependency('champlain-gtk-0.12', version : libchamplain_version, required : false)
  libchamplain_deps = [ libchamplain_dep, libchamplain_gtk_dep ]
  use_libchamplain = libchamplain_dep.found() and libchamplain_gtk_dep.found()
else
  use_libchamplain = false
endif

if get_option('lcms2')
  lcms2_dep = dependency('lcms2', version : lcms2_version, required : false)
  use_lcms2 = lcms2_dep.found()
else
  use_lcms2 = false
endif

if get_option('colord')
  colord_dep = dependency('colord', version : colord_version, required : false)
  use_colord = colord_dep.found()
else
  use_colord = false
endif

# timezone

have_tm_gmtoff = c_comp.has_member('struct tm', 'tm_gmtoff', prefix : '#include<time.h>')
if not have_tm_gmtoff
  code = '''#include <time.h>
            int main() { timezone = 1; }'''
  have_timezone = c_comp.compiles(code, name : 'timezone variable is defined')
  if have_timezone
    code = '''#include <time.h>    
              int main() { altzone = 1; }'''
    have_altzone = c_comp.compiles(code, name : 'altzone variable is defined')    
  else
    error('unable to find a way to determine timezone')
  endif 
endif

# libjpeg

libz_dep = dependency('zlib')
libm_dep = c_comp.find_library('m')
libjpeg_dep = c_comp.find_library('jpeg')
if libjpeg_dep.found()
  jpeg_dep = [ libm_dep, libz_dep, libjpeg_dep ]
endif

code = '''#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif
int main(int c, char**v) { return 0; }'''
have_libjpeg_80 = c_comp.compiles(code, name : 'libjpeg version is 8 or greater', dependencies : jpeg_dep)

have_progressive_jpeg = c_comp.has_function('jpeg_simple_progression', prefix : '#include <stdio.h>\n#include<jpeglib.h>', dependencies : jpeg_dep)

# tiff

if get_option('libtiff')
  libtiff_dep = c_comp.find_library('tiff')
  use_libtiff = libtiff_dep.found()
  tiff_dep = [ libm_dep, libz_dep, libtiff_dep ]  
else
  use_libtiff = false
endif

if get_option('libwebp')
  libwebp_dep = dependency('libwebp', version : libwebp_version, required : false)
  use_libwebp = libwebp_dep.found()
else
  use_libwebp = false
endif

if get_option('libraw')
  libraw_dep = dependency('libraw', version : libraw_version, required : false)
  use_libraw = libraw_dep.found()
else
  use_libraw = false
endif

if get_option('librsvg')
  librsvg_dep = dependency('librsvg-2.0', version : librsvg_version, required : false)
  use_librsvg = librsvg_dep.found()
else
  use_librsvg = false
endif

use_libsecret = false
if get_option('webservices')
  libsoup_dep = dependency('libsoup-2.4', version : libsoup_version, required : false)
  libjson_glib_dep = dependency('json-glib-1.0', version : libjson_glib_version, required : false)
  webkit2_dep = dependency('webkit2gtk-4.0', version : webkit2_version, required : false)
  with_webservices = libsoup_dep.found() and libjson_glib_dep.found() and webkit2_dep.found()
  
  if with_webservices and get_option('libsecret')
    libsecret_dep = dependency('libsecret-1', version : libsecret_version, required : false)
    use_libsecret = libsecret_dep.found()
  endif  
else
  with_webservices = false
endif

if get_option('libbrasero')
  libbrasero_dep = dependency('libbrasero-burn3', version : libbrasero_version, required : false)
  use_libbrasero = libbrasero_dep.found()
else
  use_libbrasero = false
endif

# web albums

bison = find_program('bison', required : false)
flex = find_program('flex', required : false)
with_webalbums = bison.found() and flex.found()

vector_ops = (c_comp.get_id() == 'gcc') and (c_comp.version().version_compare('>=4.0.7'))

common_dep = [
  libm_dep,
  dependency('threads'),
  dependency('glib-2.0', version : glib_version),
  dependency('gthread-2.0'),
  dependency('gmodule-2.0'),
  dependency('gio-unix-2.0'),
  dependency('gtk+-3.0', version : gtk_version),
  dependency('libpng'),
  dependency('zlib'),
  jpeg_dep,
  dependency('gsettings-desktop-schemas')
]

# config.h

config_data = configuration_data()
config_data.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_data.set_quoted('PACKAGE_NAME', meson.project_name())
config_data.set_quoted('PACKAGE_VERSION', meson.project_version())
config_data.set_quoted('GTHUMB_API_VERSION', api_version)
config_data.set_quoted('GTHUMB_LOCALEDIR', join_paths(prefix, get_option('localedir')))
config_data.set_quoted('GTHUMB_UI_DIR', ui_data_dir)
config_data.set_quoted('GTHUMB_ICON_DIR', icons_data_dir)
config_data.set_quoted('GTHUMB_EXTENSIONS_UI_DIR', ui_data_dir)
config_data.set_quoted('GTHUMB_EXTENSIONS_DIR', extensions_data_dir)
if get_option('buildtype').contains('debug')
  config_data.set('DEBUG', 1)
endif
if get_option('run-in-place')
  config_data.set('RUN_IN_PLACE', 1)
endif
if use_exiv2
  config_data.set('HAVE_EXIV2_020', 1)
endif
if use_clutter
  config_data.set('HAVE_CLUTTER', 1)
endif
if use_gstreamer
  config_data.set('HAVE_GSTREAMER', 1)
endif
if use_libchamplain
  config_data.set('HAVE_LIBCHAMPLAIN', 1)
endif
if use_lcms2
  config_data.set('HAVE_LCMS2', 1)
endif
if use_colord
  config_data.set('HAVE_COLORD', 1)
endif
if have_tm_gmtoff
  config_data.set('HAVE_TM_GMTOFF', 1)
else
  if have_timezone
    config_data.set('HAVE_TIMEZONE', 1)
    if have_altzone
      config_data.set('HAVE_ALTZONE', 1)
    endif
  endif
endif
config_data.set('HAVE_LIBJPEG', 1)
if have_libjpeg_80
  config_data.set('HAVE_LIBJPEG_80', 1)
endif
if have_progressive_jpeg
  config_data.set('HAVE_PROGRESSIVE_JPEG', 1)
endif
if use_libtiff
  config_data.set('HAVE_LIBTIFF', 1)
endif
if use_libwebp
  config_data.set('HAVE_LIBWEBP', 1)  
  config_data.set('WEBP_IS_UNKNOWN_TO_GLIB', 1) # Define to 1 if webp images area not recognized by the glib functions 
endif
if use_libraw
  config_data.set('HAVE_LIBRAW', 1)
endif
if use_librsvg
  config_data.set('HAVE_LIBRSVG', 1)
endif
if use_libsecret
  config_data.set('HAVE_LIBSECRET', 1)
endif
if with_webservices
  config_data.set('WITH_WEB_SERVICES', 1)
endif
if use_libbrasero
  config_data.set('HAVE_LIBBRASERO', 1)
endif
config_file = configure_file(output : 'config.h', configuration : config_data)
config_inc = include_directories('.')

# C args

c_args = []
if get_option('buildtype').contains('debug')
  test_args = [ 
    '-Wall', 
    '-Wcast-align', 
    '-Wtype-limits', 
    '-Wclobbered', 
    '-Wempty-body', 
    '-Wignored-qualifiers', 
    '-Wmissing-prototypes',
    '-Wnested-externs', 
    '-Wpointer-arith', 
    '-Wno-sign-compare', 
    '-Wformat-security'
  ]
else
  c_args += [ '-Wno-deprecated-declarations' ]
  test_args = [ '-Wall' ]
endif
c_args += c_comp.get_supported_arguments(test_args)

# Subdirectories

#subdir('extensions')
subdir('gthumb')
subdir('data')
subdir('help')
subdir('po')

# Summary

summary = [
  'configuration summary:',
  '',
  '           project: @0@ @1@'.format(meson.project_name(), meson.project_version()),
  '               api: @0@'.format(api_version),
  '            prefix: @0@'.format(prefix),
  '      run in place: @0@'.format(get_option('run-in-place')),
  '             exiv2: @0@'.format(use_exiv2),
  '           clutter: @0@'.format(use_clutter),
  '         gstreamer: @0@'.format(use_gstreamer),
  '      libchamplain: @0@'.format(use_libchamplain),
  '             lcms2: @0@'.format(use_lcms2),
  '            colord: @0@'.format(use_colord),
  '       libjpeg >=8: @0@'.format(have_libjpeg_80),
  '  progressive jpeg: @0@'.format(have_progressive_jpeg),
  '              tiff: @0@'.format(use_libtiff),
  '              webp: @0@'.format(use_libwebp),
  '               raw: @0@'.format(use_libraw),
  '               svg: @0@'.format(use_librsvg),
  '      web services: @0@'.format(with_webservices),
  '         libsecret: @0@'.format(use_libsecret), 
  '        libbrasero: @0@'.format(use_libbrasero),  
  '        web albums: @0@'.format(with_webalbums),
  '        vector ops: @0@'.format(vector_ops),
  ''
]
message('\n'.join(summary))
