install_man('gthumb.1')

if get_option('developer-mode')
  desktop_file = i18n.merge_file(
    input: 'app.gthumb.gthumb-devel.desktop.in',
    output: 'app.gthumb.gthumb-devel.desktop',
    type: 'desktop',
    po_dir: join_paths(meson.project_source_root(), 'po'),
    install: true,
    install_dir: join_paths(datadir, 'applications')
  )
else
  desktop_file = i18n.merge_file(
    input: 'app.gthumb.gthumb.desktop.in',
    output: 'app.gthumb.gthumb.desktop',
    type: 'desktop',
    po_dir: join_paths(meson.project_source_root(), 'po'),
    install: true,
    install_dir: join_paths(datadir, 'applications')
  )
endif

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

subdir('schemas')
subdir('icons')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
