source_files = files(
  'gth-open-map.c',
  'main.c'
)

shared_library('open_map',
  sources : source_files,
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  override_options : [ 'b_lundef=false' ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'open_map.extension.desktop.in.in',
  output : 'open_map.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'open_map.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
