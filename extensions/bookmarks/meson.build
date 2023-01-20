source_files = files(
  'actions.c',
  'actions.h',
  'callbacks.c',
  'callbacks.h',
  'dlg-bookmarks.c',
  'dlg-bookmarks.h',
  'main.c'
)

gresource_files = gnome.compile_resources('bookmarks-resources', 'resources.xml', c_name : 'gth')

shared_module('bookmarks',
  sources : [ source_files, gresource_files ],
  dependencies : extension_common_deps,
  include_directories : [ config_inc, gthumb_inc ],
  c_args : c_args,
  install : true,
  install_dir : extensions_install_dir,
  install_rpath : extensions_install_dir
)

# .extension file

extension_in_file = configure_file(
  input : 'bookmarks.extension.desktop.in.in',
  output : 'bookmarks.extension.in',
  configuration : extension_data
)

i18n.merge_file(
  input : extension_in_file,
  output : 'bookmarks.extension',
  type : 'desktop',
  po_dir : po_dir,
  install : true,
  install_dir : extensions_install_dir
)
