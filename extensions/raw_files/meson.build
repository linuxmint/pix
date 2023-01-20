source_files = files(
  'main.c',
  'gth-metadata-provider-raw.c'
)

shared_module('raw_files',
  sources : source_files,
  dependencies : [ extension_common_deps, libraw_dep ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'raw_files.extension.desktop.in.in',
  output : 'raw_files.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'raw_files.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
