source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-media-viewer-preferences.c',
  'gth-media-viewer-page.c',
  'gth-metadata-provider-gstreamer.c',
  'main.c'
)

shared_library('gstreamer_tools',
  sources : source_files,
  dependencies : [ extension_common_deps, gstreamer_deps ],
  include_directories : [ config_inc, pix_inc ],
  c_args : c_args,
  link_with: gstreamer_utils_mod,
  override_options : [ 'b_lundef=false' ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'gstreamer_tools.extension.desktop.in.in',
  output : 'gstreamer_tools.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'gstreamer_tools.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
