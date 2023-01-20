source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-slideshow.c',
  'gth-slideshow-preferences.c',
  'gth-transition.c',
  'main.c',
  'preferences.c'
)

shared_module('slideshow',
  sources : source_files,
  dependencies : [
    extension_common_deps,
    use_clutter ? clutter_deps : [],
    use_gstreamer ? gstreamer_deps : []
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : [
    catalogs_mod,
    use_gstreamer ? gstreamer_utils_mod : []
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'slideshow.extension.desktop.in.in',
  output : 'slideshow.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'slideshow.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
