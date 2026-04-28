subdir('public')
if get_option('developer-mode')
  install_data(
    'org.gnome.gthumb-devel.svg',
    install_dir: join_paths(datadir, 'icons/hicolor/scalable/apps')
  )
else
  install_data(
    'org.gnome.gthumb.svg',
    install_dir: join_paths(datadir, 'icons/hicolor/scalable/apps')
  )
endif
