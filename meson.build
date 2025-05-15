project('Thumb', 'vala', 'c')

dependencies = [
  dependency('glib-2.0'),
  dependency('gobject-2.0'),
  dependency('gtk4'),
  #dependency('libadwaita-1'),
  meson.get_compiler('c').find_library('m', required: false),
]

add_project_arguments([
  '--disable-warnings',
], language: 'vala')

add_project_arguments([
  '-w',
  '-Wno-error=return-mismatch',
  '-Wno-error=implicit-function-declaration',
], language: 'c')

sources = files(
  'Application.vala',
  'Main.vala',
  'Settings.vala',
  'Uri.vala',
  'Window.vala',
)

executable(
  'thumb',
  sources,
  dependencies: dependencies
)
