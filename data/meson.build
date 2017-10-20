
install_man('gthumb.1')

# .desktop file

desktop_data = configuration_data()
desktop_data.set('VERSION', meson.project_version())            
desktop_in_file = configure_file(
  input : 'org.gnome.gThumb.desktop.in.in',
  output : 'org.gnome.gThumb.desktop.in',
  configuration : desktop_data
)

i18n.merge_file(
  input : desktop_in_file,
  output : 'org.gnome.gThumb.desktop',
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
  filebase : 'gthumb-@0@'.format(api_version),
  subdirs : 'gthumb-@0@'.format(api_version),
  requires : [ 'gtk+-3.0' ],
  version: meson.project_version(),
  variables : [ 
    'extensionsdir=${libdir}/gthumb/extensions',
    'datarootdir=${prefix}/share',
    'datadir=${datarootdir}'
  ]
)

# Subdirectories

subdir('appdata')
subdir('gschemas')
subdir('icons')
subdir('ui')
