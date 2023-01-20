header_files = [
  'actions.h',
  'callbacks.h',
  'dlg-contact-sheet.h',
  'dlg-image-wall.h',
  'gth-contact-sheet-creator.h',
  'gth-contact-sheet-theme.h',
  'gth-contact-sheet-theme-dialog.h',
  'preferences.h'
]

enum_files = gnome.mkenums_simple('contact-sheet-enum-types', sources: header_files)

source_files = files(
  'actions.c',
  'callbacks.c',
  'dlg-contact-sheet.c',
  'dlg-image-wall.c',
  'gth-contact-sheet-creator.c',
  'gth-contact-sheet-theme.c',
  'gth-contact-sheet-theme-dialog.c',
  'main.c'
)

shared_module('contact_sheet',
  sources : [ source_files, enum_files ],
  dependencies : [
    extension_common_deps,
    use_gstreamer ? gstreamer_deps : [],
  ],
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  link_with: [ image_rotation_mod, export_tools_mod ],
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'contact_sheet.extension.desktop.in.in',
  output : 'contact_sheet.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'contact_sheet.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)

# Subdirectories

subdir('data')
