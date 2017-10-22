
source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-export-to-photobucket.c',
  'main.c',
  'photobucket-account.c',
  'photobucket-album.c',
  'photobucket-album-properties-dialog.c',
  'photobucket-consumer.c',
  'photobucket-photo.c',
  'photobucket-service.c'
)

shared_module('photobucket', 
  sources : source_files,
  dependencies : [
    extension_common_deps,
    libsoup_dep,
    use_libsecret ? libsecret_dep : [],    
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : [
    oauth_mod,
    export_tools_mod
  ],
  install : true,
  install_dir : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'photobucket.extension.in.in',
  output : 'photobucket.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'photobucket.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
