
source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-convert-format.c',
  'main.c'
)

shared_library('convert_format',
  sources : source_files,
  dependencies : extension_common_deps,
  include_directories : [ config_inc, pix_inc ],
  c_args : c_args,
  override_options : [ 'b_lundef=false' ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'convert_format.extension.desktop.in.in',
  output : 'convert_format.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'convert_format.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
