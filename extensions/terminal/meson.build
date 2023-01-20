source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-terminal-preferences.c',
  'main.c'
)

shared_module('terminal',
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
  input : 'terminal.extension.desktop.in.in',
  output : 'terminal.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'terminal.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
