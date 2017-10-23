
source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-export-to-picasaweb.c',
  'dlg-import-from-picasaweb.c',
  'main.c',
  'picasa-album-properties-dialog.c',
  'picasa-web-album.c',
  'picasa-web-photo.c',
  'picasa-web-service.c'
)

shared_module('picasaweb', 
  sources : source_files,
  dependencies : [
    extension_common_deps,
    importer_dep,
    libsoup_dep,
    libjson_glib_dep,
    use_libsecret ? libsecret_dep : [],    
  ],
  include_directories : [ 
    config_inc, 
    gthumb_inc,
    importer_inc
  ],
  c_args : c_args,
  link_with : [
    importer_mod,
    export_tools_mod,
    oauth_mod,
  ],
  install : true,
  install_dir : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'picasaweb.extension.in.in',
  output : 'picasaweb.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'picasaweb.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
