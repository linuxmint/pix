source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-find-duplicates.c',
  'gth-find-duplicates.c',
  'gth-folder-chooser-dialog.c',
  'main.c'
)

shared_library('find_duplicates',
  sources : source_files,
  dependencies : extension_common_deps,
  include_directories : [ config_inc, pix_inc ],
  c_args : c_args,
  link_with : catalogs_mod,
  override_options : [ 'b_lundef=false' ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'find_duplicates.extension.desktop.in.in',
  output : 'find_duplicates.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'find_duplicates.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
