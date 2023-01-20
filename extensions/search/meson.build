source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-search.c',
  'gth-search-editor.c',
  'gth-search-editor-dialog.c',
  'gth-search-source.c',
  'gth-search-source-selector.c',
  'gth-search-task.c',
  'main.c'
)

shared_module('search',
  sources : [ source_files, gresource_files ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : catalogs_mod,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'search.extension.desktop.in.in',
  output : 'search.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'search.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
