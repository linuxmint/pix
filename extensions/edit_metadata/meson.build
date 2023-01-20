source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-edit-metadata.c',
  'gth-delete-metadata-task.c',
  'gth-edit-comment-dialog.c',
  'gth-edit-general-page.c',
  'gth-edit-metadata-dialog.c',
  'gth-edit-tags-dialog.c',
  'gth-tag-task.c',
  'main.c',
  'utils.c'
)

edit_metadata_mod = shared_module('edit_metadata',
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
  input : 'edit_metadata.extension.desktop.in.in',
  output : 'edit_metadata.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'edit_metadata.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
