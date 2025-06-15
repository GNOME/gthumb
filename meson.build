project('Thumbnails',
  [ 'vala', 'c', 'cpp' ],
  license: 'GPL2+',
  version: '4',
  meson_version: '>=0.56'
)

adwaita_version = '>=1.7.3'
glib_version = '>=2.84.2'
gtk_version = '>=4.18.5'
lcms2_version = '>=2.6'
colord_version = '>=1.3'
png_version = '>=1.6'
libwebp_version = '>=0.2.0'
librsvg_version = '>=2.34.0'
gstreamer_version = '>=1.26.0'
exiv2_version = '>=0.28'

c_comp = meson.get_compiler('c')
cpp_comp = meson.get_compiler('cpp')
vala_comp = meson.get_compiler('vala')

# lcms2

if get_option('lcms2')
  lcms2_dep = dependency('lcms2',
    version: lcms2_version,
    required: false
  )
  use_lcms2 = lcms2_dep.found()
else
  use_lcms2 = false
endif

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

code = '''#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif
int main(int c, char**v) { return 0; }'''
have_libjpeg_80 = c_comp.compiles(
  code,
  name: 'libjpeg version is 8 or greater',
  dependencies: libjpeg_dep
)
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

if get_option('gstreamer')
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
  use_gstreamer = gstreamer_dep.found() and gstreamer_plugins_base_dep.found() and gstreamer_video_dep.found()
else
  use_gstreamer = false
endif

# exiv2

if get_option('exiv2')
  exiv2_dep = dependency('exiv2',
    version: exiv2_version,
    required: false
  )
  use_exiv2 = exiv2_dep.found()
else
  use_exiv2 = false
endif


dependencies = [
  dependency('threads'),
  c_comp.find_library('m', required: false),
  vala_comp.find_library('posix'),
  dependency('glib-2.0', version: glib_version),
  dependency('gobject-2.0'),
  dependency('gtk4', version: gtk_version),
  dependency('libadwaita-1', version: adwaita_version),
  dependency('libpng', version: png_version),
  libjpeg_dep,
  use_lcms2 ? lcms2_dep : [],
  use_colord ? colord_dep : [],
  use_libwebp ? libwebp_dep : [],
  use_librsvg ? librsvg_dep : [],
  use_gstreamer ? gstreamer_dep_list : [],
  use_exiv2 ? exiv2_dep : [],
]

gettext_package = 'thumbnails'
log_domain = 'Thumbnails'
prefix = get_option('prefix')

add_project_arguments([
  '--disable-warnings',
  '--vapidir', join_paths(meson.current_source_dir(), 'vapi'),
], language: 'vala')

add_project_arguments([
  '-w',
  '-Wno-error=return-mismatch',
  '-Wno-error=implicit-function-declaration',
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DG_LOG_DOMAIN="@0@"'.format(log_domain),
  '-DLOCALEDIR="@0@"'.format(join_paths(prefix, get_option('localedir'))),
], language: 'c')

vector_ops = (c_comp.get_id() == 'gcc') and (c_comp.version().version_compare('>=4.0.7'))

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config.set_quoted('DOMAIN', meson.project_name ())
config.set_quoted('G_LOG_DOMAIN', log_domain)
config.set_quoted('VERSION', meson.project_version())
config.set_quoted('PREFIX', get_option('prefix'))
if vector_ops
  config.set('HAVE_VECTOR_OPERATIONS', 1)
endif
if have_progressive_jpeg
  config.set('HAVE_PROGRESSIVE_JPEG', 1)
endif
if use_lcms2
  config.set('HAVE_LCMS2', 1)
endif
if use_colord
  config.set('HAVE_COLORD', 1)
endif
if use_libwebp
  config.set('HAVE_LIBWEBP', 1)
  add_project_arguments(['-D', 'HAVE_LIBWEBP'], language: 'vala')
endif
if use_librsvg
  config.set('HAVE_LIBRSVG', 1)
  add_project_arguments(['-D', 'HAVE_LIBRSVG'], language: 'vala')
endif
if use_gstreamer
  config.set('HAVE_GSTREAMER', 1)
  add_project_arguments(['-D', 'HAVE_GSTREAMER'], language: 'vala')
endif
if use_exiv2
  config.set('HAVE_EXIV2', 1)
  add_project_arguments(['-D', 'HAVE_EXIV2'], language: 'vala')
endif

config_file = configure_file(
  output : 'config.h',
  configuration : config
)

gnome = import('gnome')
resources_files = gnome.compile_resources('resources',
  'App.gresource.xml',
  c_name: 'resources',
  source_dir: [
    'data/icons/private/',
    'data/icons/private/actions',
    'data/icons/public/hicolor/scalable/apps',
    'data/css',
    'data/ui',
  ]
)

lib_files = files(
  'lib/exiv2-utils.cpp',
  'lib/gth-color-manager.c',
  'lib/gth-icc-profile.c',
  'lib/gth-image.c',
  'lib/gth-image-edit.c',
  'lib/gth-image-resize.c',
  'lib/gth-metadata.c',
  'lib/gth-string-list.c',
  'lib/io/load-jpeg.c',
  'lib/io/load-png.c',
  'lib/io/save-png.c',
  'lib/io/image-info.c',
  'lib/jpeg/jmemorysrc.c',
  'lib/jpeg/jpeg-info.c',
  'lib/pixel.c',
  'lib/util.c',
)
if use_libwebp
  lib_files += ('lib/io/load-webp.c')
endif
if use_librsvg
  lib_files += ('lib/io/load-svg.c')
endif

vapi_files = files(
  'vapi/ColorManager.vapi',
  'vapi/Config.vapi',
  'vapi/Exiv2.vapi',
  'vapi/IccProfile.vapi',
  'vapi/IccTransform.vapi',
  'vapi/Image.vapi',
  'vapi/Loaders.vapi',
  'vapi/Metadata.vapi',
  'vapi/Savers.vapi',
  'vapi/StringList.vapi',
  'vapi/Types.vapi',
)

source_files = files(
  'Application.vala',
  'CatalogSearch.vala',
  'Catalog.vala',
  'DateTime.vala',
  'DefaultTests.vala',
  'Dialogs/EditFiltersDialog.vala',
  'Dialogs/FilterEditorPage.vala',
  'Dialogs/FilterRow.vala',
  'Dialogs/SortFilesDialog.vala',
  'Dialogs/SortFoldersDialog.vala',
  'Dom.vala',
  'Factory.vala',
  'FileData.vala',
  'FileListItem.vala',
  'FilePropertyView.vala',
  'FileSourceCatalogs.vala',
  'FileSource.vala',
  'FileSource.vala',
  'FileSourceVfs.vala',
  'FileSourceVfs.vala',
  'Files.vala',
  'FilterBar.vala',
  'FilterFile.vala',
  'Filter.vala',
  'FolderTreeItem.vala',
  'GenericList.vala',
  'GenericTree.vala',
  'ImageLoader.vala',
  'ImageSaver.vala',
  'Job.vala',
  'Job.vala',
  'Main.vala',
  'MetadataProviderExiv2.vala',
  'MetadataProviderFile.vala',
  'MetadataProviderImage.vala',
  'MetadataProvider.vala',
  'PropertySidebar.vala',
  'PropertyView.vala',
  'Settings.vala',
  'SidebarResizer.vala',
  'SortFunc.vala',
  'Status.vala',
  'Strings.vala',
  'TestDate.vala',
  'TestExpr.vala',
  'TestInt.vala',
  'TestSelector.vala',
  'TestSize.vala',
  'TestString.vala',
  'Test.vala',
  'ThumbLoader.vala',
  'Thumbnailer.vala',
  'TimeSelector.vala',
  'Types.vala',
  'UserDir.vala',
  'Util.vala',
  'Widgets.vala',
  'WindowBookmarks.vala',
  'WindowHistory.vala',
  'Window.vala',
  'EmbeddedPropertyView.vala',
)

executable(
  'thumbnails',
  sources: [
    config_file,
    resources_files,
    lib_files,
    vapi_files,
    source_files,
  ],
  dependencies: dependencies
)

if get_option('build-tests')
  test('dom',
    executable('test-dom',
      sources: [
        'Tests/TestDom.vala',
        'Dom.vala',
        'Files.vala',
        'Strings.vala',
      ],
      dependencies: dependencies,
      vala_args: '--define=TEST_XML',
    )
  )
endif

subdir('po')

# Summary

summary(
  {
    'prefix': prefix,
    'libjpeg 8': have_libjpeg_80,
    'prog jpeg': have_progressive_jpeg,
    'vector ops': vector_ops,
    'lcms2': use_lcms2,
    'colord': use_colord,
    'libwebp': use_libwebp,
    'librsvg': use_librsvg,
    'gstreamer': use_gstreamer,
    'exiv2': use_exiv2,
  },
  section: 'Configuration summary:',
)
