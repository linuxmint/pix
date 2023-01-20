source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-image-info.c',
  'gth-image-print-job.c',
  'gth-load-image-info-task.c',
  'preferences.c',
  'main.c'
)

shared_module('image_print',
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
  input : 'image_print.extension.desktop.in.in',
  output : 'image_print.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'image_print.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
