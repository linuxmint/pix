source_files = files(
  'gstreamer-utils.c',
  'main.c'
)

gstreamer_utils_mod = shared_module('gstreamer_utils',
  sources : source_files,
  dependencies : [ extension_common_deps, gstreamer_deps ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'gstreamer_utils.extension.desktop.in.in',
  output : 'gstreamer_utils.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'gstreamer_utils.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
