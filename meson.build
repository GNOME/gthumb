project('Thumb', 'vala', 'c')

dependencies = [
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gtk4'),
  dependency('libadwaita-1'),
  meson.get_compiler('c').find_library('m', required: false),
]

add_project_arguments([
  '--disable-warnings',
], language: 'vala')

gettext_package = 'Thumb'
log_domain = 'Thumb'

add_project_arguments([
  '-w',
  '-Wno-error=return-mismatch',
  '-Wno-error=implicit-function-declaration',
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DG_LOG_DOMAIN="@0@"'.format(log_domain),
], language: 'c')

config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config.set_quoted('DOMAIN', meson.project_name ())
config.set_quoted('G_LOG_DOMAIN', log_domain)
config.set_quoted('VERSION', meson.project_version())
config.set_quoted('PREFIX', get_option('prefix'))

config_file = configure_file(
  input: 'Config.vala.in',
  output : 'Config.vala',
  configuration : config
)

gnome = import('gnome')
resources_files = gnome.compile_resources('resources',
  'App.gresource.xml',
  c_name: 'resources',
  source_dir: [
    'data/icons/actions',
    'data/css',
    'data/ui',
  ],
)

source_files = files(
  'Application.vala',
  'DateTime.vala',
  'DefaultTests.vala',
  'Dialogs/PersonalizeFilters.vala',
  'Dialogs/FilterEditorPage.vala',
  'Dom.vala',
  'FileData.vala',
  'Files.vala',
  'Filter.vala',
  'FilterBar.vala',
  'FilterFile.vala',
  'ListModelIterator.vala',
  'Main.vala',
  'Metadata.vala',
  'Settings.vala',
  'SortFunc.vala',
  'Strings.vala',
  'TestChain.vala',
  'TestDate.vala',
  'TestSelector.vala',
  'TestSize.vala',
  'TestString.vala',
  'TestInt.vala',
  'Test.vala',
  'TimeSelector.vala',
  'Types.vala',
  'Uri.vala',
  'UserDir.vala',
  'Util.vala',
  'Widgets.vala',
  'Window.vala',
)

executable(
  'thumb',
  sources: [
    config_file,
    resources_files,
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
