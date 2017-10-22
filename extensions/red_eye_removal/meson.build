source_files = files(
  'gth-file-tool-red-eye.c',
  'main.c'
)

shared_module('red_eye_removal', 
  sources : source_files,
  dependencies : [ extension_common_deps ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with: [ image_viewer_mod ],
  install : true,
  install_dir : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'red_eye_removal.extension.in.in',
  output : 'red_eye_removal.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'red_eye_removal.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
