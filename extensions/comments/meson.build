source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-comments-preferences.c',
  'gth-comment.c',
  'gth-import-metadata-task.c',
  'gth-metadata-provider-comment.c',
  'main.c'
)

shared_module('comments',
  sources : source_files,
  dependencies : [ extension_common_deps ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : use_exiv2 ? exiv2_tools_mod : [],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'comments.extension.desktop.in.in',
  output : 'comments.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'comments.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
