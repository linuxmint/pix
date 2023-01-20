
source_files = files(
  'actions.c',
  'callbacks.c',
  'main.c'
)

shared_module('flicker',
  sources : source_files,
  dependencies : [
    extension_common_deps,
    importer_dep,
    libsoup_dep,
    use_libsecret ? libsecret_dep : [],
  ],
  include_directories : [
    config_inc,
    gthumb_inc,
    importer_inc
  ],
  c_args : c_args,
  link_with : [
    flicker_utils_mod,
    photo_importer_mod,
    export_tools_mod,
    importer_mod
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'flicker.extension.desktop.in.in',
  output : 'flicker.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'flicker.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
