source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-add-to-catalog.c',
  'dlg-catalog-properties.c',
  'dlg-organize-files.c',
  'gth-catalog.c',
  'gth-file-source-catalogs.c',
  'gth-organize-task.c',
  'main.c'
)

catalogs_mod = shared_module('catalogs',
  sources : source_files,
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'catalogs.extension.desktop.in.in',
  output : 'catalogs.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'catalogs.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
