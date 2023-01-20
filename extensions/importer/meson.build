source_files = files(
  'gth-import-destination-button.c',
  'gth-import-preferences-dialog.c',
  'gth-import-task.c',
  'main.c',
  'utils.c'
)

header_files = [
  'gth-import-destination-button.h',
  'gth-import-preferences-dialog.h',
  'gth-import-task.h',
  'preferences.h',
  'utils.h'
]

enum_files = gnome.mkenums_simple('gth-import-enum-types', sources: header_files)

importer_mod = shared_module('importer',
  sources : [ source_files, enum_files ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with : [
    catalogs_mod,
    image_rotation_mod,
    use_exiv2 ? exiv2_tools_mod : []
  ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

importer_inc = include_directories('.')
importer_dep = declare_dependency(sources : enum_files)

# .extension file

extension_in_file = configure_file(
  input : 'importer.extension.desktop.in.in',
  output : 'importer.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'importer.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

subdir('data')
