
source_files = files(
  'actions.c',
  'callbacks.c',
  'main.c'
)

shared_module('23hq',
  sources : source_files,
  dependencies : [
    extension_common_deps,
    libsoup_dep,
    use_libsecret ? libsecret_dep : [],
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : [
    flicker_utils_mod,
    export_tools_mod,
    importer_mod
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : '23hq.extension.desktop.in.in',
  output : '23hq.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : '23hq.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
