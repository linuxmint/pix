source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-copy-task.c',
  'gth-duplicate-task.c',
  'gth-reorder-task.c',
  'main.c'
)

shared_library('file_manager',
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
  input : 'file_manager.extension.desktop.in.in',
  output : 'file_manager.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'file_manager.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
