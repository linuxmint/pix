source_files = files(
  'actions.c',
  'callbacks.c',
  'gth-reset-orientation-task.c',
  'gth-transform-task.c',
  'main.c',
  'rotation-utils.c'
)

image_rotation_mod = shared_library('image_rotation',
  sources : source_files,
  dependencies : [ extension_common_deps, jpeg_deps ],
  include_directories : [ config_inc, pix_inc ],
  c_args : c_args,
  link_with : jpeg_utils_mod,
  override_options : [ 'b_lundef=false' ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'image_rotation.extension.desktop.in.in',
  output : 'image_rotation.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'image_rotation.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
