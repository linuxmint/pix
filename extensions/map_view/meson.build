source_files = files(
  'gth-map-view.c',
  'main.c'
)

shared_module('map_view',
  sources : source_files,
  dependencies : [ extension_common_deps, libchamplain_deps ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'map_view.extension.desktop.in.in',
  output : 'map_view.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'map_view.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
