
source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-burn-task.c',
  'main.c'
)

shared_module('burn_disc',
  sources : source_files,
  dependencies : [ extension_common_deps, libbrasero_dep ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : export_tools_mod,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'burn_disc.extension.desktop.in.in',
  output : 'burn_disc.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'burn_disc.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
