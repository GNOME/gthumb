i18n.merge_file(
  input: 'app.gthumb.gthumb.desktop.in',
  output: 'app.gthumb.gthumb.desktop',
  type: 'desktop',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: join_paths(datadir, 'applications')
)

subdir('schemas')
subdir('icons')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true
)
