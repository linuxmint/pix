source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-image-histogram.c',
  'gth-image-viewer-page.c',
  'gth-image-viewer-page-tool.c',
  'gth-image-viewer-task.c',
  'gth-metadata-provider-image.c',
  'main.c',
  'preferences.c'
)

gresource_files = gnome.compile_resources('image-viewer-resources', 'resources.xml', c_name : 'gth')

image_viewer_mod = shared_module('image_viewer',
  sources : [ source_files, gresource_files ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : jpeg_utils_mod,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'image_viewer.extension.desktop.in.in',
  output : 'image_viewer.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'image_viewer.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
