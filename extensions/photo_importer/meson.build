source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-photo-importer.c',
  'dlg-photo-importer-preferences.c',
  'main.c'
)

gresource_files = gnome.compile_resources('photo-importer-resources', 'resources.xml', c_name : 'gth')

photo_importer_mod = shared_module('photo_importer',
  sources : [ source_files, gresource_files ],
  dependencies : [
    extension_common_deps,
    importer_dep
  ],
  include_directories : [
    config_inc,
    gthumb_inc,
    importer_inc
  ],
  c_args : c_args,
  link_with : [
    catalogs_mod,
    importer_mod,
    image_rotation_mod,
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'photo_importer.extension.desktop.in.in',
  output : 'photo_importer.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'photo_importer.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
