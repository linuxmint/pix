
source_files = files(
  'dlg-export-to-flickr.c',
  'dlg-import-from-flickr.c',
  'flickr-account.c',
  'flickr-consumer.c',
  'flickr-photo.c',
  'flickr-photoset.c',
  'flickr-service.c',
  'main.c'
)

flicker_utils_mod = shared_module('flicker_utils',
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
    oauth_mod,
    export_tools_mod,
    importer_mod
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'flicker_utils.extension.desktop.in.in',
  output : 'flicker_utils.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'flicker_utils.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
