project('Thumbnails',
  [ 'vala', 'c' ],
  license: 'GPL2+',
  version: '4',
  meson_version: '>=0.56'
)

adwaita_version = '>=1.7.3'
colord_version = '>=1.3'
glib_version = '>=2.84.2'
gtk_version = '>=4.18.5'
lcms2_version = '>=2.6'
png_version = '>=1.6'

c_comp = meson.get_compiler('c')
vala_comp = meson.get_compiler('vala')

if get_option('lcms2')
  lcms2_dep = dependency('lcms2',
    version: lcms2_version,
    required: false
  )
  use_lcms2 = lcms2_dep.found()
else
  use_lcms2 = false
endif

if get_option('colord')
  colord_dep = dependency('colord',
    version: colord_version,
    required: false
  )
  use_colord = colord_dep.found()
else
  use_colord = false
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
  use_lcms2 ? lcms2_dep : [],
  use_colord ? colord_dep : [],
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

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config.set_quoted('DOMAIN', meson.project_name ())
config.set_quoted('G_LOG_DOMAIN', log_domain)
config.set_quoted('VERSION', meson.project_version())
config.set_quoted('PREFIX', get_option('prefix'))

config_file = configure_file(
  output : 'config.h',
  configuration : config
)

gnome = import('gnome')
resources_files = gnome.compile_resources('resources',
  'App.gresource.xml',
  c_name: 'resources',
  source_dir: [
    'data/icons/private/actions',
    'data/icons/public/hicolor/scalable/apps',
    'data/css',
    'data/ui',
  ],
)

lib_files = files(
  'lib/gth-color-manager.c',
  'lib/gth-icc-profile.c',
  'lib/gth-image.c',
  'lib/gth-image-resize.c',
  'lib/pixel.c',
  'lib/util.c',
  'lib/io/load-png.c',
  'lib/jpeg/jpeg-info.c',
)

vapi_files = files(
  'vapi/ColorManager.vapi',
  'vapi/Config.vapi',
  'vapi/IccProfile.vapi',
  'vapi/IccTransform.vapi',
  'vapi/Image.vapi',
  'vapi/Loaders.vapi',
  'vapi/Types.vapi',
)

source_files = files(
  'Application.vala',
  'DateTime.vala',
  'DefaultTests.vala',
  'Dialogs/EditFiltersDialog.vala',
  'Dialogs/FilterEditorPage.vala',
  'Dialogs/FilterRow.vala',
  'Dialogs/SortFilesDialog.vala',
  'Dom.vala',
  'FileData.vala',
  'FileListItem.vala',
  'FileSource.vala',
  'FileSource.vala',
  'FileSourceVfs.vala',
  'FileSourceVfs.vala',
  'Files.vala',
  'FilterBar.vala',
  'FilterFile.vala',
  'Filter.vala',
  'GenericList.vala',
  'ImageLoader.vala',
  'Job.vala',
  'Job.vala',
  'Main.vala',
  'MetadataProviderFile.vala',
  'MetadataProvider.vala',
  'Metadata.vala',
  'Settings.vala',
  'SortFunc.vala',
  'Strings.vala',
  'TestChain.vala',
  'TestDate.vala',
  'TestInt.vala',
  'TestSelector.vala',
  'TestSize.vala',
  'TestString.vala',
  'Test.vala',
  'Thumbnailer.vala',
  'TimeSelector.vala',
  'Types.vala',
  'UserDir.vala',
  'Util.vala',
  'Widgets.vala',
  'Window.vala',
  'Status.vala',
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
