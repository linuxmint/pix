source_files = files(
  'main.c'
)

shared_module('example',
  sources : [ source_files ],
  dependencies : extension_deps,
  include_directories : [ config_inc ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'example.extension.desktop.in.in',
  output : 'example.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'example.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
