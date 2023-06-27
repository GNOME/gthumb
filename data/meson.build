
install_man('gthumb.1')

# .desktop files

i18n.merge_file(
  input : 'org.gnome.gThumb.desktop.in',
  output : 'org.gnome.gThumb.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

i18n.merge_file(
  input : 'org.gnome.gThumb.Import.desktop.in',
  output : 'org.gnome.gThumb.Import.desktop',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'applications')
)

# .pc file

pkg = import('pkgconfig')
gthumb_pc = pkg.generate(
  name : 'gThumb',
  description : 'gThumb extensions',
  filebase : meson.project_name(),
  subdirs : meson.project_name(),
  requires : [ 'gtk+-3.0' ],
  version: meson.project_version(),
  variables : [
    '='.join(['extensionsdir', extensions_install_dir]),
    '='.join(['uidir', ui_install_dir]),
    '='.join(['apiversion', api_version])
  ]
)

# Subdirectories

if dependency('appstream', version : '>=0.14.6', required : false).found()
  subdir('appdata')
endif
subdir('gschemas')
subdir('icons')
subdir('ui')
